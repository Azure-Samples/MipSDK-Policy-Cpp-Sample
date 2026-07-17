/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

#pragma once

#include <cstdint>

namespace sample {
	namespace auth {
		constexpr uint32_t kMipAuthRequestVersion1 = 1;

		enum MipAuthResult : int {
			kMipAuthSuccess = 0,
			kMipAuthInvalidRequest = 1,
			kMipAuthValidationFailed = 2,
			kMipAuthAuthenticationFailed = 3,
			kMipAuthBufferTooSmall = 4,
			kMipAuthInternalError = 5
		};

		struct MipAuthRequestV1 {
			uint32_t version;
			uint32_t size;
			const char* username;
			uint32_t usernameLength;
			const char* clientId;
			uint32_t clientIdLength;
			const char* authority;
			uint32_t authorityLength;
			const char* resource;
			uint32_t resourceLength;
			const char* claims;
			uint32_t claimsLength;
			char* tokenBuffer;
			uint32_t tokenBufferSize;
			uint32_t tokenLength;
			char* errorBuffer;
			uint32_t errorBufferSize;
			uint32_t errorLength;
		};

		static_assert(sizeof(void*) != 8 || sizeof(MipAuthRequestV1) == 120,
			"MipAuthRequestV1 layout must match the managed ABI.");
	}
}
