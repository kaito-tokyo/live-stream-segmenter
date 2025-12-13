/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
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
	explicit GoogleTokenStorage(std::filesystem::path configFilePath) noexcept
		: configFilePath_(std::move(configFilePath))
	{
	}

	virtual ~GoogleTokenStorage() noexcept = default;

	GoogleTokenStorage(const GoogleTokenStorage &) = delete;
	GoogleTokenStorage &operator=(const GoogleTokenStorage &) = delete;
	GoogleTokenStorage(GoogleTokenStorage &&) = delete;
	GoogleTokenStorage &operator=(GoogleTokenStorage &&) = delete;

	virtual std::optional<GoogleTokenState> load()
	{
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

	virtual void save(const GoogleTokenState &tokenState)
	{
		if (auto parentDirectory = configFilePath_.parent_path(); !parentDirectory.empty()) {
			std::filesystem::create_directories(parentDirectory);
		}

		std::ofstream file(configFilePath_);
		if (file.is_open()) {
			nlohmann::json j = tokenState;
			file << j.dump();
		}
	}

	virtual void clear() { std::filesystem::remove(configFilePath_); }

private:
	const std::filesystem::path configFilePath_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
