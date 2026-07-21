/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#include "auth.h"

#include <stdexcept>
#include <string>

int main() {
	try {
		(void)sample::auth::AcquireToken(
			"invalid-user",
			"00000000-0000-0000-0000-000000000001",
			"https://api.aadrm.com",
			"https://login.microsoftonline.com/common",
			"");
	}
	catch (const std::runtime_error& error) {
		const std::string message = error.what();
		if (!message.empty() &&
			message.find("tenant domain") != std::string::npos &&
			message.find_first_of("\r\n") == std::string::npos) {
			return 0;
		}
	}
	return 1;
}
