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
#ifndef SAMPLES_UPE_PROTECTION_DESCRIPTOR_IMPL_H_
#define SAMPLES_UPE_PROTECTION_DESCRIPTOR_IMPL_H_

#include <chrono> 
#include <map>
#include <string>
#include <vector>

#include "mip/protection_descriptor.h"

	namespace sample {
	namespace policy {
		/**
		 * @brief This class implements the ProtectionDescriptor class in a simple manner for the sample app.
		 * The protection SDK should be used to generate a similar class. If the use case is UPE only then this class
		 * should hold state according to the protection specifications.
		 */
		class ProtectionDescriptorImpl final : public mip::ProtectionDescriptor {
		public:
			ProtectionDescriptorImpl(const std::string& templateId) :
				mType(mip::ProtectionType::TemplateBased),
				mTemplateId(templateId),
				mContentValidUntil(std::chrono::system_clock::now() + std::chrono::hours(24 * 30)) {}
			mip::ProtectionType GetProtectionType() const override { return mType; }
			std::string GetName() const override { return mName; }
			std::string GetOwner() const override { return mOwner; }
			std::string GetDescription() const override { return mDescription; }
			std::string GetTemplateId() const override { return mTemplateId; }
			std::string GetLabelId() const override { return mLabelId; }
			std::string GetContentId() const override { return mContentId;  }
			std::vector<mip::UserRights> GetUserRights() const override { return mUserRights; };
			std::vector<mip::UserRoles> GetUserRoles() const override { return mUserRoles; };
			bool DoesContentExpire() const override { return mContentValidUntil.time_since_epoch().count() != 0; }
			std::chrono::time_point<std::chrono::system_clock> GetContentValidUntil() const override {
				return mContentValidUntil;
			}
			bool DoesAllowOfflineAccess() const override { return mDoesAllowOfflineAccess; }
			std::string GetReferrer() const override { return mReferrer; }
			std::map<std::string, std::string> GetEncryptedAppData() const override { return mEncryptedAppData; }
			std::map<std::string, std::string> GetSignedAppData() const override { return mSignedAppData; }
            std::string GetDoubleKeyUrl() const override { return mDoubleKeyUrl; }

        private:
            mip::ProtectionType mType;
            std::string mName;
            std::string mOwner;
            std::string mDescription;
            std::string mTemplateId;
            std::string mLabelId;
            std::string mContentId;
            std::vector<mip::UserRights> mUserRights;
            std::vector<mip::UserRoles> mUserRoles;
            std::chrono::time_point<std::chrono::system_clock> mContentValidUntil;
            bool mDoesAllowOfflineAccess = true;
            std::string mReferrer;
            std::map<std::string, std::string> mEncryptedAppData;
            std::map<std::string, std::string> mSignedAppData;
            std::string mDoubleKeyUrl;
        };
	}
	//  namespace sample
} //  namespace upe

#endif //  SAMPLES_UPE_PROTECTION_DESCRIPTOR_IMPL_H_