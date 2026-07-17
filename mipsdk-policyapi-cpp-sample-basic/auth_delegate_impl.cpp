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


#include "auth_delegate_impl.h"
#include "auth.h"

#include <iostream>
#include <stdexcept>

using std::runtime_error;
using std::string;

namespace sample {
	namespace auth {

		AuthDelegateImpl::AuthDelegateImpl(
			const mip::ApplicationInfo& applicationInfo,
			const std::string& username)
			: mApplicationInfo(applicationInfo),
			  mUserName(username) {
		}
			
		bool AuthDelegateImpl::AcquireOAuth2Token(
			const mip::Identity& /*identity*/,
			const OAuth2Challenge& challenge,
			OAuth2Token& token) {
			
			try {
				string accessToken = sample::auth::AcquireToken(
					mUserName,
					mApplicationInfo.applicationId,
					challenge.GetResource(),
					challenge.GetAuthority(),
					challenge.GetClaims());
				token.SetAccessToken(accessToken);
				return true;
			}
			catch (const std::exception& error) {
				std::cerr << "Authentication failed: " << error.what() << std::endl;
				return false;
			}
			catch (...) {
				std::cerr << "Authentication failed." << std::endl;
				return false;
			}
		}

	} //  namespace sample
} //  namespace auth