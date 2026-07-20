/**
*
* Copyright (c) Microsoft Corporation.
* All rights reserved.
*
* This code is licensed under the MIT License.
*
*/

#pragma once

#include <string>

namespace sample {
	namespace auth {
		std::string AcquireToken(
			const std::string& username,
			const std::string& clientId,
			const std::string& resource,
			const std::string& authority,
			const std::string& claims);
	}
}
