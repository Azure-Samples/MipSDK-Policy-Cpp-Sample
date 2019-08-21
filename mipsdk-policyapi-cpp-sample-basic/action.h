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


#ifndef SAMPLES_BASICLABELING_ACTION_H_
#define SAMPLES_BASICLABELING_ACTION_H_

#include <memory>
#include <string>

#include "mip/common_types.h"
#include "mip/upe/policy_profile.h"
#include "mip/upe/policy_engine.h"

#include "auth_delegate_impl.h"
#include "profile_observer_impl.h"
#include "execution_state_impl.h"

namespace sample {
	namespace policy {		
		class Action {
		public:
			
			Action(const mip::ApplicationInfo appInfo,
				const std::string& username,
				const std::string& password,
				const bool generateAuditEvents);
			
			~Action();
					
			void ListLabels();							// List all labels associated engine loaded for user			
			std::vector<std::shared_ptr<mip::Action>> ComputeAction(const ExecutionStateOptions& options); // Calculate actions for new label			
			bool ComputeActionLoop(ExecutionStateOptions& options); // Loop on provided execution state options, updating each iteration until zero actions are needed. 
			std::shared_ptr<mip::Label> GetLabelById(const std::string& labelId);

		private:
			void AddNewProfile();					// Private function for adding and loading mip::FileProfile
			void AddNewPolicyEngine();					// Private function for adding/loading mip::FileEngine for specified user
			
			std::shared_ptr<sample::auth::AuthDelegateImpl> mAuthDelegate;			// AuthDelegateImpl object that will be used throughout the sample to store auth details.
			std::shared_ptr<mip::MipContext> mMipContext;
			std::shared_ptr<mip::PolicyProfile> mProfile;								// mip::FileProfile object to store/load state information 
			std::shared_ptr<mip::PolicyEngine> mEngine;								// mip::FileEngine object to handle user-specific actions. 			
			mip::ApplicationInfo mAppInfo;											// mip::ApplicationInfo object for storing client_id and friendlyname
			std::shared_ptr<ProfileObserverImpl> mProfileObserver;
			bool mGenerateAuditEvents;												// Set if application should submit audit events to AIP Analytics


			std::string mUsername; // store username to pass to auth delegate and to generate Identity
			std::string mPassword; // store password to pass to auth delegate
		};

	}
}


#endif
