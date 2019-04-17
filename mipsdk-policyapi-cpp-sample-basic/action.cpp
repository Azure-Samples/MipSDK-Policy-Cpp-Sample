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

#include "mip/mip_init.h"
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

		// Constructor accepts mip::ApplicationInfo object and uses it to initialize AuthDelegateImpl.
		// Specifically, AuthDelegateInfo uses mAppInfo.ApplicationId for AAD client_id value.		
		Action::Action(const mip::ApplicationInfo appInfo,
			const std::string& username,
			const std::string& password,
			const bool generateAuditEvents)
			: mAppInfo(appInfo),
			mUsername(username),
			mPassword(password),
			mGenerateAuditEvents(generateAuditEvents) {
			mAuthDelegate = std::make_shared<sample::auth::AuthDelegateImpl>(mAppInfo, mUsername, mPassword);
		}

		Action::~Action()
		{
			mProfile = nullptr;
			mEngine = nullptr;
			mip::ReleaseAllResources();
		}

		// Method illustrates how to create a new mip::PolicyProfile using promise/future
		// Result is stored in private mProfile variable and referenced throughout lifetime of Action.
		void sample::policy::Action::AddNewProfile()
		{
			// Initialize the Profile::Settings Object. Example below stores state data in /file_sample/ directory 
			// and permits use license caching of protected content. Accepts AuthDelegate, new Profile::Observer, and ApplicationInfo object as last parameters.
			PolicyProfile::Settings profileSettings("mip_data", false, mAuthDelegate, std::make_shared<ProfileObserverImpl>(), mAppInfo);

			// Create promise and future for mip::PolicyProfile object.
			auto profilePromise = std::make_shared<std::promise<std::shared_ptr<PolicyProfile>>>();
			auto profileFuture = profilePromise->get_future();

			// Call static function LoadAsync providing the settings and promise. This will make the profile available to use.
			PolicyProfile::LoadAsync(profileSettings, profilePromise);

			// Get the future value and store in mProfile. mProfile is used throughout Action for profile operations.
			mProfile = profileFuture.get();
		}

		// Action::AddNewPolicyEngine adds an engine for a specific user. 		
		void Action::AddNewPolicyEngine()
		{
			// If mProfile hasn't been set, use AddNewProfile() to set it.
			if (!mProfile)
			{
				AddNewProfile();
			}

			// PolicyEngine requires a PolicyEngine::Settings object. The first parameter is the user identity or engine ID. 
			PolicyEngine::Settings engineSettings(mip::Identity(mUsername), "", "en-US", mGenerateAuditEvents);

			// Create promise and future for mip::PolicyEngine object
			auto enginePromise = std::make_shared<std::promise<std::shared_ptr<PolicyEngine>>>();
			auto engineFuture = enginePromise->get_future();

			// Engines are added to profiles. Call AddEngineAsync on mProfile, providing settings and promise
			// then get the future value and set in mEngine. mEngine will be used throughout Action for engine operations.
			mProfile->AddEngineAsync(engineSettings, enginePromise);
			mEngine = engineFuture.get();
		}

		// Function recursively lists all labels available for a user to	std::cout.
		void Action::ListLabels() {

			// If mEngine hasn't been set, call AddNewPolicyEngine() to load the engine.
			if (!mEngine) {
				AddNewPolicyEngine();
			}

			// Use mip::PolicyEngine to list all labels
			auto labels = mEngine->ListSensitivityLabels();

			// Iterate through each label, first listing details
			for (const auto& label : labels) {
				cout << label->GetName() << " : " << label->GetId() << endl;

				// get all children for mip::Label and list details
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

			// ExecutionStateImpl is derived from mip::ExecutionState
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

			// ExecutionStateImpl is derived from mip::ExecutionState
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

							// Iterate through list of metadata to add and add to execution state.
							for (const std::string oldMetadata : derivedAction->GetMetadataToRemove())
							{
								/******
								*
								* In this loop, your application should handle removing metadata from the file the user is labeling.
								*
								*******/

								options.metadata.clear();

								// Display metadata.
								cout << oldMetadata << endl;
							}
						}

						if (derivedAction->GetMetadataToAdd().size() > 0)
						{
							cout << "*** Action Type: Apply Metadata" << endl;

							// Iterate through list of metadata to add and add to execution state.
							for (const std::pair<std::string, std::string>& prop : derivedAction->GetMetadataToAdd())
							{
								/******
								*
								* In this loop, your application should handle adding metadata to the file the user is labeling.
								*
								*******/

								options.metadata[prop.first] = prop.second;

								// Display metadata.
								cout << prop.first << " : " << prop.second << endl;
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

						// Display Template ID.
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

						// Set template to empty.
						options.templateId.resize(0);
						break;
					}


					case mip::ActionType::JUSTIFY: {

						/******
						*
						* Here, your application would call display some prompt to the user to provide justification on downgrading.
						*
						*******/

						cout << "*** Action Type: Justification Required" << endl;

						// Enter some string for justification.
						cout << "Provide Justification: ";
						cin >> options.downgradeJustification;
						options.isDowngradeJustified = true;
						cout << endl;
						break;
					}

				    // Implement remaining case statements for all mip::ActionTypes

					default:
					{

					}

					}
				}

				// Compute actions based on new state information. 
				// Update state
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
