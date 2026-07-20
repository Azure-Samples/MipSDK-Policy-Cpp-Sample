/**
*
* Copyright (c) Microsoft Corporation.
* All rights reserved.
*
* This code is licensed under the MIT License.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

#include "action.h"

#include "mip/mip_context.h"
#include "mip/common_types.h"
#include "mip/upe/action.h"
#include "mip/upe/protect_by_template_action.h"
#include "mip/upe/execution_state.h"
#include "mip/upe/policy_engine.h"
#include "mip/upe/policy_profile.h"
#include "mip/upe/metadata_action.h"


#include "auth_delegate_impl.h"
#include "profile_observer_impl.h"
#include "utils.h"

#include <iostream>
#include <future>

using std::cout;
using std::cin;
using std::endl;

using mip::PolicyProfile;
using mip::PolicyEngine;

namespace sample {
	namespace policy {

		Action::Action(const mip::ApplicationInfo appInfo,
			const std::string& username,
			const bool generateAuditEvents)
			: mAppInfo(appInfo),
			mUsername(username),
			mGenerateAuditEvents(generateAuditEvents) {
			mAuthDelegate = std::make_shared<sample::auth::AuthDelegateImpl>(mAppInfo, mUsername);
		}

		Action::~Action()
		{			
			mEngine = nullptr;
			mProfile = nullptr;
			if (mMipContext) {
				mMipContext->ShutDown();
				mMipContext = nullptr;
			}
		}

		// Load a Policy profile and bridge the asynchronous callback with a future.
		void sample::policy::Action::AddNewProfile()
		{			

			std::shared_ptr<mip::MipConfiguration> mipConfiguration = std::make_shared<mip::MipConfiguration>(mAppInfo,
				"mip_data",
				mip::LogLevel::Trace,
				false,
				mip::CacheStorageType::OnDiskEncrypted);

			mMipContext = mip::MipContext::Create(mipConfiguration);

			// Persist the profile cache encrypted on disk and receive asynchronous
			// completion through the profile observer.
			PolicyProfile::Settings profileSettings(mMipContext, 
				mip::CacheStorageType::OnDiskEncrypted,  
				std::make_shared<ProfileObserverImpl>());

			auto profilePromise = std::make_shared<std::promise<std::shared_ptr<PolicyProfile>>>();
			auto profileFuture = profilePromise->get_future();

			PolicyProfile::LoadAsync(profileSettings, profilePromise);

			mProfile = profileFuture.get();
		}

		void Action::AddNewPolicyEngine()
		{
			// If mProfile hasn't been set, use AddNewProfile() to set it.
			if (!mProfile)
			{
				AddNewProfile();
			}

			// Configure a Policy engine for the requested user and authentication delegate.
			PolicyEngine::Settings engineSettings(mip::Identity(mUsername), mAuthDelegate, "", "en-US", mGenerateAuditEvents);

			auto enginePromise = std::make_shared<std::promise<std::shared_ptr<PolicyEngine>>>();
			auto engineFuture = enginePromise->get_future();

			// Bridge the asynchronous engine callback with a future.
			mProfile->AddEngineAsync(engineSettings, enginePromise);
			mEngine = engineFuture.get();
		}


		std::shared_ptr<mip::Label> Action::GetLabelById(const std::string& labelId)
		{
			if (!mEngine)
			{
				AddNewPolicyEngine();
			}

			return mEngine->GetLabelById(labelId);
		}

		// Display top-level labels and their immediate children.
		void Action::ListLabels() {

			// If mEngine hasn't been set, call AddNewPolicyEngine() to load the engine.
			if (!mEngine) {
				AddNewPolicyEngine();
			}

			auto labels = mEngine->ListSensitivityLabels();

			for (const auto& label : labels) {
				cout << label->GetName() << " : " << label->GetId() << endl;

				for (const auto& child : label->GetChildren()) {
					cout << "->  " << child->GetName() << " : " << child->GetId() << endl;
				}
			}
		}


		std::vector<std::shared_ptr<mip::Action>> Action::ComputeAction(const ExecutionStateOptions& options)
		{
			// If an engine hasn't been added, add it.
			if (!mEngine)
			{
				AddNewPolicyEngine();
			}

			std::unique_ptr<ExecutionStateImpl> state;

			state.reset(new ExecutionStateImpl(options));
			auto handler = mEngine->CreatePolicyHandler("");
			auto actions = handler->ComputeActions(*state);

			if (options.generateAuditEvent && actions.size() == 0)
			{
				handler->NotifyCommittedActions(*state);
			}
			
			return actions;
		}


		bool Action::ComputeActionLoop(ExecutionStateOptions& options)
		{
			// If an engine hasn't been added, add it.
			if (!mEngine)
			{
				AddNewPolicyEngine();
			}

			std::unique_ptr<ExecutionStateImpl> state;
			state.reset(new ExecutionStateImpl(options));
			
			auto handler = mEngine->CreatePolicyHandler("");
			auto actions = handler->ComputeActions(*state);

			while (actions.size() > 0)
			{
				cout << "Action Count: " << actions.size() << endl;

				// Iterate through actions returned from ComputeActions()
				for (const auto action : actions)
				{
					switch (action->GetType())
					{
					case mip::ActionType::METADATA: {

						auto derivedAction = static_cast<mip::MetadataAction*>(action.get());

						if (derivedAction->GetMetadataToRemove().size() > 0)
						{
							cout << "*** Action: Remove Metadata" << endl;

							// Process metadata keys that should be removed from the content.
							for (const std::string oldMetadata : derivedAction->GetMetadataToRemove())
							{
								/******
								*
								* In this loop, your application should remove the requested metadata from the content being labeled.
								*
								*******/

								options.metadata.erase(oldMetadata);

								cout << oldMetadata << endl;
							}
						}

						if (derivedAction->GetMetadataToAdd().size() > 0)
						{
							cout << "*** Action Type: Apply Metadata" << endl;

							// Add the requested metadata to the simulated content state.
							for (const mip::MetadataEntry& prop : derivedAction->GetMetadataToAdd())
							{
								/******
								*
								* In this loop, your application should add or update metadata on the content being labeled.
								*
								*******/								
								options.metadata.insert_or_assign(prop.GetKey(), prop.GetValue());


								cout << prop.GetKey() << " : " << prop.GetValue() << endl;
							}
						}
						break;
					}


					case mip::ActionType::PROTECT_BY_TEMPLATE: {

						/******
						*
						* Here, your application would call the protection API to apply protection to the data.
						*
						*******/

						auto derivedAction = static_cast<mip::ProtectByTemplateAction*>(action.get());
						options.templateId = derivedAction->GetTemplateId();

						cout << "*** Action Type: Protect By Template: " << options.templateId << endl;
						break;
					}

					case mip::ActionType::REMOVE_PROTECTION: {

						/******
						*
						* Here, your application would call the protection API to remove protection from the data.
						*
						*******/

						cout << "*** Action Type: Remove Protection." << endl;

						options.templateId.resize(0);
						break;
					}


					case mip::ActionType::JUSTIFY: {

						/******
						*
						* Here, your application would prompt the user to justify lowering the label.
						*
						*******/

						cout << "*** Action Type: Justification Required" << endl;

						cout << "Provide Justification: ";
						cin >> options.downgradeJustification;
						options.isDowngradeJustified = true;
						cout << endl;
						break;
					}

				    // A production integration should implement every action type it declares as supported.

					default:
					{

					}

					}
				}

				// Recompute actions using the state updated by the previous results.
				state.reset(new ExecutionStateImpl(options));

				actions = handler->ComputeActions(*state);
				
				cout << "*** Remaining Action Count: " << actions.size() << endl;			
			}

			if (options.generateAuditEvent && actions.size() == 0)
			{
				handler->NotifyCommittedActions(*state);
			}

			return true;
		}
	}

	
}
