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

#include <memory>
#include <mutex>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>
#include <obs.h>
#include <util/config-file.h>

#include <ILogger.hpp>

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class YouTubeStore {
public:
	explicit YouTubeStore(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~YouTubeStore() noexcept = default;

	YouTubeStore(const YouTubeStore &) = delete;
	YouTubeStore &operator=(const YouTubeStore &) = delete;
	YouTubeStore(YouTubeStore &&) = delete;
	YouTubeStore &operator=(YouTubeStore &&) = delete;

	void setStreamKeyA(const std::string &streamKeyId)
	{
		std::scoped_lock lock(mutex_);
		streamKeyA_ = streamKeyId;
	}

	void setStreamKeyB(const std::string &streamKeyId)
	{
		std::scoped_lock lock(mutex_);
		streamKeyB_ = streamKeyId;
	}


	std::string getStreamKeyA() const
	{
		std::scoped_lock lock(mutex_);
		return streamKeyA_;
	}

	std::string getStreamKeyB() const
	{
		std::scoped_lock lock(mutex_);
		return streamKeyB_;
	}

	void saveYouTubeStore() noexcept
	{
		std::scoped_lock lock(mutex_);
		saveToConfig("YouTubeStore_streamKeyA", streamKeyA_);
		saveToConfig("YouTubeStore_streamKeyB", streamKeyB_);
	}

	void restoreYouTubeStore() noexcept
	{
		std::scoped_lock lock(mutex_);
		restoreFromConfig("YouTubeStore_streamKeyA", streamKeyA_);
		restoreFromConfig("YouTubeStore_streamKeyB", streamKeyB_);
	}

private:
	void saveToConfig(const char *key, const nlohmann::json &json) noexcept
	{
		config_t *config = obs_frontend_get_profile_config();
		try {
			std::string jsonString = json.dump();
			config_set_string(config, PLUGIN_NAME, key, jsonString.c_str());
			config_save(config);
		} catch (const std::exception &e) {
			logger_->logException(e, "StoreError(YouTubeStore::saveToConfig):" + std::string(key));
		} catch (...) {
			logger_->error("UnknownError(YouTubeStore::saveToConfig):{}", key);
		}
	}

	template<typename T> void restoreFromConfig(const char *key, T &target) noexcept
	{
		config_t *config = obs_frontend_get_profile_config();
		const char *jsonCstr = config_get_string(config, PLUGIN_NAME, key);

		if (jsonCstr == nullptr || jsonCstr[0] == '\0') {
			logger_->info("{} not found in config.", key);
		} else {
			try {
				nlohmann::json j = nlohmann::json::parse(jsonCstr);
				j.get_to(target);
				logger_->info("{} restored successfully.", key);
			} catch (const std::exception &e) {
				logger_->logException(e, "RestoreError(YouTubeStore::restoreFromConfig):" +
								 std::string(key));
			} catch (...) {
				logger_->error("UnknownError(YouTubeStore::restoreFromConfig):{}", key);
			}
		}
	}

	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	std::string streamKeyA_;
	std::string streamKeyB_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
