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

#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include "action.h"
#include "mip/common_types.h"
#include "utils.h"
#include "execution_state_impl.h"
#include "mip/upe/metadata_action.h"
#include "mip/upe/protect_by_template_action.h"
#include "mip/upe/justify_action.h"


using std::make_shared;
using std::shared_ptr;
using std::string;

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

using sample::policy::Action;

namespace {
	void PauseForUser() {
		std::cout << "Press Enter to continue...";
		std::string line;
		std::getline(std::cin, line);
	}
}

int RunSample()
{
	std::string newLabelId;
	std::string currentLabelId;
		
	// Client ID should be the client ID registered in Microsoft Entra ID for your custom application.
	std::string clientId = "<YOUR APPLICATION ID>";

	// The username is a login hint for cache lookup and interactive MSAL authentication.
	std::string userName = "<YOUR USER UPN>";

	// Identify this application to the MIP SDK and Microsoft Entra ID.
	mip::ApplicationInfo appInfo{ clientId, "MIP SDK Policy Sample for C++", "1.18.0" };

	// Action coordinates the Policy profile, user-specific engine, authentication,
	// and action computation. The final argument enables audit event generation.
	Action action = Action(appInfo, userName, true);

	// Display top-level labels and their immediate children, then pause.
	action.ListLabels();
	PauseForUser();

	// Select the label currently applied to the simulated content.
	cout << endl << endl << "Enter a label ID: ";
	cin >> currentLabelId;

	// Select the label to apply.
	cout << endl << "Enter a new label ID: ";
	cin >> newLabelId;

	// Describe the simulated content and labeling operation.
	sample::policy::ExecutionStateOptions options;

	// Compute the state produced by the current label so it can be used as the
	// starting point for the label change.
	options.newLabel = action.GetLabelById(currentLabelId);
	options.actionSource = mip::ActionSource::MANUAL;
	options.assignmentMethod = mip::AssignmentMethod::STANDARD;
	options.contentFormat = mip::GetEmailContentFormat();	
	options.contentIdentifier = "MyTestFile.pptx";
	options.dataState = mip::DataState::USE;	
	options.isDowngradeJustified = false;
	options.generateAuditEvent = true;
		
	auto initialActions = action.ComputeAction(options);

	// Capture metadata and template protection produced by the current label.
	for (const auto action : initialActions)
	{
		switch (action->GetType())
		{
		case mip::ActionType::METADATA:
		{
			options.metadata.clear();
			auto derivedAction = static_cast<mip::MetadataAction*>(action.get());
			for (const mip::MetadataEntry& prop : derivedAction->GetMetadataToAdd())
			{
				options.metadata[prop.GetKey()] = prop.GetValue();
			}
			break;
		}

		case mip::ActionType::PROTECT_BY_TEMPLATE: {
			auto derivedAction = static_cast<mip::ProtectByTemplateAction*>(action.get());
			options.templateId = derivedAction->GetTemplateId();			
			break;
		}
		default:
		{

		}
		}
	}

	// Update the desired state to the new label.
	options.newLabel = action.GetLabelById(newLabelId);
	
	// Compute and process actions until the desired state is satisfied.
	auto result = action.ComputeActionLoop(options);	
	
	cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	PauseForUser();

	return 0;
}

int main()
{
	try
	{
		return RunSample();
	}
	catch (const std::exception& error)
	{
		std::cerr << "Sample failed: " << error.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Sample failed." << std::endl;
		return 1;
	}
}
