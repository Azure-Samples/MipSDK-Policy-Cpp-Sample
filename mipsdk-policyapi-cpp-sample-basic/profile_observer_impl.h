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


#ifndef SAMPLES_PROFILE_OBSERVER_IMPL_H_
#define SAMPLES_PROFILE_OBSERVER_IMPL_H_

#include <functional>
#include <memory>


#include "mip/upe/policy_profile.h"


class ProfileObserverImpl final : public mip::PolicyProfile::Observer {
public:
	// ProfileObserverImpl(std::function<void(const std::string&)>&& policyChangedHandler) { }
	ProfileObserverImpl() {}
	//  Observer implementation
	virtual void OnLoadSuccess(const std::shared_ptr<mip::PolicyProfile>& profile, const std::shared_ptr<void>& context) override;
	virtual void OnLoadFailure(const std::exception_ptr& Failure,	const std::shared_ptr<void>& context) override;

	virtual void OnListEnginesSuccess(const std::vector<std::string>& engineIds,const std::shared_ptr<void>& context) override;
	virtual void OnListEnginesFailure(const std::exception_ptr& Failure, const std::shared_ptr<void>& context) override;

	virtual void OnUnloadEngineSuccess(const std::shared_ptr<void>& context) override;
	virtual void OnUnloadEngineFailure(const std::exception_ptr& Failure, const std::shared_ptr<void>& context) override;

	virtual void OnAddEngineSuccess(const std::shared_ptr<mip::PolicyEngine>& engine, const std::shared_ptr<void>& context) override;
	virtual void OnAddEngineFailure(const std::exception_ptr& Failure, const std::shared_ptr<void>& context) override;

	virtual void OnDeleteEngineSuccess(const std::shared_ptr<void>& context) override;
	virtual void OnDeleteEngineFailure(const std::exception_ptr& Failure, const std::shared_ptr<void>& context) override;

	virtual void OnPolicyChanged(const std::string& engineId) override;

private:
	std::function<void(const std::string&)> mPolicyChangedHandler;

};

#endif