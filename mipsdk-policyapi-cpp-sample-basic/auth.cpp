/**
*
* Copyright (c) Microsoft Corporation.
* All rights reserved.
*
* This code is licensed under the MIT License.
*
*/

#include "auth.h"
#include "mip_auth_abi.h"

#include <Windows.h>

#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <nethost.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using std::runtime_error;
using std::string;

namespace sample {
	namespace auth {
		namespace {
			constexpr uint32_t kBufferSize = 64 * 1024;
			constexpr size_t kMaximumErrorLength = 512;

			using acquire_token_fn = int(CORECLR_DELEGATE_CALLTYPE*)(void*, int32_t);

			struct ManagedHostState {
				HMODULE hostfxrModule = nullptr;
				acquire_token_fn acquireToken = nullptr;
				std::exception_ptr initializationError;
			};

			fs::path GetExecutableDirectory() {
				std::vector<wchar_t> buffer(1024);
				for (;;) {
					const DWORD length = GetModuleFileNameW(
						nullptr,
						buffer.data(),
						static_cast<DWORD>(buffer.size()));
					if (length == 0) {
						throw runtime_error("Authentication runtime is unavailable.");
					}
					if (length < buffer.size()) {
						return fs::path(buffer.data(), buffer.data() + length).parent_path();
					}
					if (buffer.size() > (std::numeric_limits<DWORD>::max)() / 2) {
						throw runtime_error("Authentication runtime is unavailable.");
					}
					buffer.resize(buffer.size() * 2);
				}
			}

			void HOSTFXR_CALLTYPE IgnoreHostError(const char_t*) {
			}

			void InitializeManagedHost(ManagedHostState& state) {
				std::vector<char_t> hostfxrPath(1024);
				size_t hostfxrPathSize = hostfxrPath.size();
				int result = get_hostfxr_path(hostfxrPath.data(), &hostfxrPathSize, nullptr);
				if (result != 0 && hostfxrPathSize > hostfxrPath.size()) {
					hostfxrPath.resize(hostfxrPathSize);
					result = get_hostfxr_path(hostfxrPath.data(), &hostfxrPathSize, nullptr);
				}
				if (result != 0) {
					throw runtime_error("Authentication runtime is unavailable.");
				}

				state.hostfxrModule = LoadLibraryExW(
					hostfxrPath.data(),
					nullptr,
					LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
				if (state.hostfxrModule == nullptr) {
					throw runtime_error("Authentication runtime is unavailable.");
				}

				const auto initialize = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
					GetProcAddress(state.hostfxrModule, "hostfxr_initialize_for_runtime_config"));
				const auto getDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
					GetProcAddress(state.hostfxrModule, "hostfxr_get_runtime_delegate"));
				const auto close = reinterpret_cast<hostfxr_close_fn>(
					GetProcAddress(state.hostfxrModule, "hostfxr_close"));
				const auto setErrorWriter = reinterpret_cast<hostfxr_set_error_writer_fn>(
					GetProcAddress(state.hostfxrModule, "hostfxr_set_error_writer"));
				if (initialize == nullptr || getDelegate == nullptr || close == nullptr) {
					throw runtime_error("Authentication runtime is unavailable.");
				}

				hostfxr_error_writer_fn previousErrorWriter = nullptr;
				if (setErrorWriter != nullptr) {
					previousErrorWriter = setErrorWriter(IgnoreHostError);
				}

				hostfxr_handle context = nullptr;
				try {
					const fs::path executableDirectory = GetExecutableDirectory();
					const fs::path runtimeConfig =
						executableDirectory / L"MipAuth.Managed.runtimeconfig.json";
					const fs::path assembly =
						executableDirectory / L"MipAuth.Managed.dll";

					result = initialize(runtimeConfig.c_str(), nullptr, &context);
					if (result != 0 || context == nullptr) {
						throw runtime_error("Authentication runtime is unavailable.");
					}

					load_assembly_and_get_function_pointer_fn loadAssembly = nullptr;
					result = getDelegate(
						context,
						hdt_load_assembly_and_get_function_pointer,
						reinterpret_cast<void**>(&loadAssembly));
					if (result != 0 || loadAssembly == nullptr) {
						throw runtime_error("Authentication runtime is unavailable.");
					}

					result = loadAssembly(
						assembly.c_str(),
						L"MipAuth.Managed.NativeEntryPoint, MipAuth.Managed",
						L"AcquireToken",
						UNMANAGEDCALLERSONLY_METHOD,
						nullptr,
						reinterpret_cast<void**>(&state.acquireToken));
					if (result != 0 || state.acquireToken == nullptr) {
						throw runtime_error("Authentication runtime is unavailable.");
					}
				}
				catch (...) {
					if (context != nullptr) {
						close(context);
					}
					if (setErrorWriter != nullptr) {
						setErrorWriter(previousErrorWriter);
					}
					throw;
				}

				close(context);
				if (setErrorWriter != nullptr) {
					setErrorWriter(previousErrorWriter);
				}
			}

			ManagedHostState& GetManagedHost() {
				static ManagedHostState state;
				static std::once_flag initializeOnce;
				std::call_once(initializeOnce, []() {
					try {
						InitializeManagedHost(state);
					}
					catch (...) {
						state.initializationError = std::current_exception();
					}
				});
				if (state.initializationError) {
					std::rethrow_exception(state.initializationError);
				}
				return state;
			}

			uint32_t CheckedLength(const string& value) {
				if (value.size() > (std::numeric_limits<uint32_t>::max)()) {
					throw runtime_error("Authentication request is invalid.");
				}
				return static_cast<uint32_t>(value.size());
			}

			string SanitizeError(const char* buffer, uint32_t length) {
				if (buffer == nullptr || length == 0) {
					return "Authentication failed.";
				}

				string sanitized;
				sanitized.reserve((std::min)(static_cast<size_t>(length), kMaximumErrorLength));
				for (uint32_t index = 0;
					index < length && sanitized.size() < kMaximumErrorLength;
					++index) {
					const unsigned char value = static_cast<unsigned char>(buffer[index]);
					sanitized.push_back(value >= 0x20 && value <= 0x7e
						? static_cast<char>(value)
						: '?');
				}
				return sanitized.empty() ? "Authentication failed." : sanitized;
			}

			void ValidateToken(const std::vector<char>& buffer, uint32_t length) {
				if (length == 0 || length >= buffer.size() || buffer[length] != '\0') {
					throw runtime_error("Authentication failed.");
				}
				for (uint32_t index = 0; index < length; ++index) {
					const unsigned char value = static_cast<unsigned char>(buffer[index]);
					if (value < 0x21 || value > 0x7e) {
						throw runtime_error("Authentication failed.");
					}
				}
			}
		}

		string AcquireToken(
			const string& username,
			const string& clientId,
			const string& resource,
			const string& authority,
			const string& claims) {
			std::vector<char> tokenBuffer(kBufferSize, '\0');
			std::vector<char> errorBuffer(kBufferSize, '\0');

			MipAuthRequestV1 request{};
			request.version = kMipAuthRequestVersion1;
			request.size = sizeof(request);
			request.username = username.data();
			request.usernameLength = CheckedLength(username);
			request.clientId = clientId.data();
			request.clientIdLength = CheckedLength(clientId);
			request.authority = authority.data();
			request.authorityLength = CheckedLength(authority);
			request.resource = resource.data();
			request.resourceLength = CheckedLength(resource);
			request.claims = claims.empty() ? nullptr : claims.data();
			request.claimsLength = CheckedLength(claims);
			request.tokenBuffer = tokenBuffer.data();
			request.tokenBufferSize = static_cast<uint32_t>(tokenBuffer.size());
			request.errorBuffer = errorBuffer.data();
			request.errorBufferSize = static_cast<uint32_t>(errorBuffer.size());

			ManagedHostState& host = GetManagedHost();
			const int result = host.acquireToken(&request, static_cast<int32_t>(sizeof(request)));
			if (result != kMipAuthSuccess) {
				throw runtime_error(SanitizeError(errorBuffer.data(), request.errorLength));
			}

			ValidateToken(tokenBuffer, request.tokenLength);
			return string(tokenBuffer.data(), request.tokenLength);
		}
	}
}
