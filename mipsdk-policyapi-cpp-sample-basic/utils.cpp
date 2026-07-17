/**
*
* Copyright (c) Microsoft Corporation.
* All rights reserved.
*
* This code is licensed under the MIT License.
*
*/

#include "utils.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using std::ifstream;
using std::string;
using std::vector;

static const char kPathSeparatorWindows = '\\';
static const char kPathSeparatorUnix = '/';
static const char kExtensionSeparator = '.';
static const char kPathSeparatorsAll[] = { kPathSeparatorWindows, kPathSeparatorUnix, '\0' };

namespace sample {
	namespace utils {
		vector<string> SplitString(const string& str, char delim) {
			vector<string> output;
			std::stringstream stream(str);
			string value;

			while (stream.good()) {
				std::getline(stream, value, delim);
				output.emplace_back(std::move(value));
			}

			return output;
		}

		bool FileExists(const char* path) {
			ifstream file(path);
			return file.good();
		}

		string GetFileExtension(const string& filePath) {
			string fileName = GetFileName(filePath);
			const auto index = fileName.rfind(kExtensionSeparator);
			if (index == string::npos) return "";
			return fileName.substr(index);
		}

		string GetFileName(const string& filePath) {
			const auto index = filePath.find_last_of(kPathSeparatorsAll);
			if (index == string::npos) return filePath;
			return filePath.substr(index + 1);
		}

		string GetOutputFileNameModified(const string& input, const string& modification) {
			auto result = input;
			const auto fileExtension = GetFileExtension(result);
			const auto resultWithoutExtension =
				result.substr(0, result.length() - fileExtension.length());
			return resultWithoutExtension + modification + fileExtension;
		}
	}
}
