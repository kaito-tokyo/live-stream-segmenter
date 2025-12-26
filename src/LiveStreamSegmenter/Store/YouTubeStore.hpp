/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#pragma once

#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <ILogger.hpp>
#include <ObsUnique.hpp>
#include <YouTubeTypes.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class YouTubeStore {
public:
	explicit YouTubeStore(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~YouTubeStore() noexcept = default;

	YouTubeStore(const YouTubeStore &) = delete;
	YouTubeStore &operator=(const YouTubeStore &) = delete;
	YouTubeStore(YouTubeStore &&) = delete;
	YouTubeStore &operator=(YouTubeStore &&) = delete;

	void setStreamKeyA(const YouTubeApi::YouTubeStreamKey &streamKey)
	{
		std::scoped_lock lock(mutex_);
		streamKeyA_ = streamKey;
	}

	void setStreamKeyB(const YouTubeApi::YouTubeStreamKey &streamKey)
	{
		std::scoped_lock lock(mutex_);
		streamKeyB_ = streamKey;
	}

	YouTubeApi::YouTubeStreamKey getStreamKeyA() const
	{
		std::scoped_lock lock(mutex_);
		return streamKeyA_;
	}

	YouTubeApi::YouTubeStreamKey getStreamKeyB() const
	{
		std::scoped_lock lock(mutex_);
		return streamKeyB_;
	}

	bool saveYouTubeStore() noexcept
	try {
		std::filesystem::path configPath = getConfigPath();
		if (configPath.empty()) {
			return false;
		}

		std::scoped_lock lock(mutex_);

		nlohmann::json j{
			{"streamKeyA", streamKeyA_},
			{"streamKeyB", streamKeyB_},
		};

		std::ofstream ofs(configPath, std::ios::out | std::ios::trunc);
		ofs << j.dump();

		return true;
	} catch (const std::exception &e) {
		logger_->error("Error(YouTubeStore::saveYouTubeStore):{}", e.what());
		return false;
	} catch (...) {
		logger_->error("UnknownError(YouTubeStore::saveYouTubeStore)");
		return false;
	}

	void restoreYouTubeStore() noexcept
	try {
		std::filesystem::path configPath = getConfigPath();
		if (configPath.empty()) {
			return;
		}

		std::scoped_lock lock(mutex_);
		std::ifstream ifs(configPath, std::ios::in);
		if (ifs.is_open()) {
			nlohmann::json j;
			ifs >> j;

			streamKeyA_ = j.value("streamKeyA", YouTubeApi::YouTubeStreamKey{});
			streamKeyB_ = j.value("streamKeyB", YouTubeApi::YouTubeStreamKey{});
		}
	} catch (const std::exception &e) {
		logger_->error("Error(YouTubeStore::restoreYouTubeStore):{}", e.what());
		streamKeyA_ = YouTubeApi::YouTubeStreamKey{};
		streamKeyB_ = YouTubeApi::YouTubeStreamKey{};
	} catch (...) {
		logger_->error("UnknownError(YouTubeStore::restoreYouTubeStore)");
		streamKeyA_ = YouTubeApi::YouTubeStreamKey{};
		streamKeyB_ = YouTubeApi::YouTubeStreamKey{};
	}

private:
	std::filesystem::path getConfigPath() const
	{
		BridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
		if (!profilePathRaw) {
			logger_->error("ProfilePathError(YouTubeStore::getConfigPath)");
			return {};
		}

		std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
		return profilePath / "live-stream-segmenter_YouTubeStore.json";
	}

	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	YouTubeApi::YouTubeStreamKey streamKeyA_;
	YouTubeApi::YouTubeStreamKey streamKeyB_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
