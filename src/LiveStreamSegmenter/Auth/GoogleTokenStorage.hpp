/*
Live Stream Segmenter

MIT License

Copyright (c) 2025 Kaito Udagawa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "GoogleTokenState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleTokenStorage {
public:
	explicit GoogleTokenStorage(std::filesystem::path configFilePath) noexcept : configFilePath_(std::move(configFilePath)) {}

	virtual ~GoogleTokenStorage() noexcept = default;

	GoogleTokenStorage(const GoogleTokenStorage &) = delete;
	GoogleTokenStorage &operator=(const GoogleTokenStorage &) = delete;
	GoogleTokenStorage(GoogleTokenStorage &&) = delete;
	GoogleTokenStorage &operator=(GoogleTokenStorage &&) = delete;

	virtual std::optional<GoogleTokenState> load() {
		std::ifstream file(configFilePath_);
		if (!file.is_open()) {
			return std::nullopt;
		}

		try {
			nlohmann::json j;
			file >> j;
			return j.get<GoogleTokenState>();
		} catch (...) {
			return std::nullopt;
		}
	}

	virtual void save(const GoogleTokenState &tokenState) {
		if (auto parentDirectory = configFilePath_.parent_path(); !parentDirectory.empty()) {
			std::filesystem::create_directories(parentDirectory);
		}

		std::ofstream file(configFilePath_);
		if (file.is_open()) {
			nlohmann::json j = tokenState;
			file << j.dump();
		}
	}

	virtual void clear() {
		std::filesystem::remove(configFilePath_);
	}

private:
	const std::filesystem::path configFilePath_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
