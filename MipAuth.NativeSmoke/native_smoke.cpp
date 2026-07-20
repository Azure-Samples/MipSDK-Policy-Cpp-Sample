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
		if (std::string(error.what()) == "Authentication request is invalid.") {
			return 0;
		}
	}
	return 1;
}
