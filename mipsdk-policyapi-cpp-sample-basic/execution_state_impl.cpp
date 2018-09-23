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

#include "execution_state_impl.h"

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

namespace sample {
	namespace policy {
		vector<pair<string, string>> ExecutionStateImpl::GetNewLabelExtendedProperties() const {
			return vector<pair<string, string>>();
		}

		vector<pair<string, string>> ExecutionStateImpl::GetContentMetadata(
			const vector<string>& names,
			const vector<string>& namePrefixes) const {
			unordered_map<string, string> filteredMetadata;

			for (const string& namePrefix : namePrefixes) {
				for (const auto& prop : mOptions.metadata) {
					if (prop.first.compare(0, namePrefix.length(), namePrefix) == 0)
						filteredMetadata[prop.first] = prop.second;
				}
			}

			for (const string& name : names) {
				auto itName = mOptions.metadata.find(name);
				if (itName != mOptions.metadata.end())
					filteredMetadata[name] = itName->second;
			}

			vector<pair<string, string>> result;
			for (const auto& prop : filteredMetadata)
				result.emplace_back(prop.first, prop.second);

			return result;
		}

		mip::ActionType ExecutionStateImpl::GetSupportedActions() const {
			//  The UPE SDK will always notify client of 'JUSTIFY', 'METADATA', and 'REMOVE*' actions. However an application can
			//  choose not to support specific actions that may appear in a policy. (For instance, A policy may define a label to
			//  require both protection and a watermark, but the application could decide not to support watermarks by not
			//  including ADD_WATERMARK here. If that were the case, 'mip::PolicyEngine::ComputeActions' would never return
			//  AddWatermark actions.)
			return mip::ActionType::ADD_CONTENT_FOOTER |
				mip::ActionType::ADD_CONTENT_HEADER |
				mip::ActionType::ADD_WATERMARK |
				mip::ActionType::CUSTOM |
				mip::ActionType::PROTECT_ADHOC |
				mip::ActionType::PROTECT_BY_TEMPLATE |
				mip::ActionType::PROTECT_DO_NOT_FORWARD;
		}

	} //  namespace sample
} //  namespace upe
