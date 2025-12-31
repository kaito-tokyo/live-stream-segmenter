/*
 * Live Stream Segmenter - Store Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <NullLogger.hpp>
#include <ObsUnique.hpp>
#include <YouTubeTypes.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class YouTubeStore {
public:
	YouTubeStore() = default;
	~YouTubeStore() noexcept = default;

	YouTubeStore(const YouTubeStore &) = delete;
	YouTubeStore &operator=(const YouTubeStore &) = delete;
	YouTubeStore(YouTubeStore &&) = delete;
	YouTubeStore &operator=(YouTubeStore &&) = delete;

	void setLogger(std::shared_ptr<const Logger::ILogger> logger) { logger_ = std::move(logger); }

	void setStreamKeyA(const YouTubeApi::YouTubeLiveStream &streamKey)
	{
		std::scoped_lock lock(mutex_);
		streamKeyA_ = streamKey;
	}

	void setStreamKeyB(const YouTubeApi::YouTubeLiveStream &streamKey)
	{
		std::scoped_lock lock(mutex_);
		streamKeyB_ = streamKey;
	}

	YouTubeApi::YouTubeLiveStream getStreamKeyA() const
	{
		std::scoped_lock lock(mutex_);
		return streamKeyA_;
	}

	YouTubeApi::YouTubeLiveStream getStreamKeyB() const
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
		if (!ofs.is_open()) {
			logger_->error("FileOpenError", {{"path", configPath.string()}});
			return false;
		}

		ofs << j.dump();

		return true;
	} catch (const std::exception &e) {
		logger_->error("Error", {{"exception", e.what()}});
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

			j["streamKeyA"].get_to(streamKeyA_);
			j["streamKeyB"].get_to(streamKeyB_);
		}
	} catch (const std::exception &e) {
		logger_->error("Error", {{"exception", e.what()}});
		streamKeyA_ = YouTubeApi::YouTubeLiveStream{};
		streamKeyB_ = YouTubeApi::YouTubeLiveStream{};
	}

private:
	std::filesystem::path getConfigPath() const
	{
		BridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
		if (!profilePathRaw) {
			logger_->error("ProfilePathError");
			return {};
		}

		std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
		return profilePath / "live-stream-segmenter_YouTubeStore.json";
	}

	mutable std::mutex mutex_;
	YouTubeApi::YouTubeLiveStream streamKeyA_;
	YouTubeApi::YouTubeLiveStream streamKeyB_;

	std::shared_ptr<const Logger::ILogger> logger_{Logger::NullLogger::instance()};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
