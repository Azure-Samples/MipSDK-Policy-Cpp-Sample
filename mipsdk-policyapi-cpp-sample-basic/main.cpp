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
#include <memory>
#include <sstream>


#include "action.h"
#include "mip/common_types.h"
#include "utils.h"
#include "execution_state_impl.h"

using std::make_shared;
using std::shared_ptr;
using std::string;

using std::cout;
using std::cin;
using std::endl;

using sample::policy::Action;

int main()
{			
	std::string newLabelId;

	// Create the mip::ApplicationInfo object. 
	// Client ID should be the client ID registered in Azure AD for your custom application.
	// Friendly Name should be the name of the application as it should appear in reports.
	mip::ApplicationInfo appInfo{ "YOUR CLIENT ID", "YOUR APP NAME", "YOUR APP VERSION" };
	
	// All actions for this tutorial project are implemented in samples::file::Action
	// Source files are Action.h/cpp.
	// "File" was chosen because this example is specifically for the MIP SDK File API. 
	// Action's constructor takes in the mip::ApplicationInfo object and uses the client ID for auth.
	// Username and password are required in this sample as the oauth2 token is obtained via Python script and basic auth.
	Action action = Action(appInfo, "YOUR TEST USER NAME","YOUR TEST USER PASSWORD", true);

	// Call action.ListLabels() to display all available labels, then pause.
	action.ListLabels();	
	system("pause");

	cout << endl << endl << "Enter a label ID: ";
	cin >> newLabelId;

	// Set execution state options and provide to ComputeActions. 
	sample::policy::ExecutionStateOptions options;
	options.newLabelId = newLabelId;
	action.ComputeAction(options);

	system("pause");
				
	return 0;
}


