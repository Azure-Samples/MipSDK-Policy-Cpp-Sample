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
#include <vector>

namespace sample {
	namespace utils {
		bool FileExists(const char* path);
		std::vector<std::string> SplitString(const std::string& str, char delim);
		std::string GetFileName(const std::string& filePath);
		std::string GetFileExtension(const std::string& filePath);
		std::string GetOutputFileNameModified(
			const std::string& input,
			const std::string& modification);
	}
}
