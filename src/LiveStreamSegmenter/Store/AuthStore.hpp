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

class AuthStore : public std::enable_shared_from_this<AuthStore> {
public:
	explicit AuthStore(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~AuthStore() noexcept = default;

	AuthStore(const AuthStore &) = delete;
	AuthStore &operator=(const AuthStore &) = delete;
	AuthStore(AuthStore &&) = delete;
	AuthStore &operator=(AuthStore &&) = delete;

	void setGoogleOAuth2ClientCredentials(Auth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials)
	{
		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredentials_ = std::move(googleOAuth2ClientCredentials);
	}

	Auth::GoogleOAuth2ClientCredentials getGoogleOAuth2ClientCredentials() const
	{
		std::scoped_lock lock(mutex_);
		return googleOAuth2ClientCredentials_;
	}

	void setGoogleTokenState(const Auth::GoogleTokenState &tokenState)
	{
		std::scoped_lock lock(mutex_);
		googleTokenState_ = tokenState;
	}

	Auth::GoogleTokenState getGoogleTokenState() const
	{
		std::scoped_lock lock(mutex_);
		return googleTokenState_;
	}

	void clearGoogleTokenState()
	{
		std::scoped_lock lock(mutex_);
		googleTokenState_ = {};
	}

	void saveAuthStore() noexcept
	{
		std::scoped_lock lock(mutex_);
		saveToConfig("googleOAuth2ClientCredentials", googleOAuth2ClientCredentials_);
		saveToConfig("googleTokenState", googleTokenState_);
	}

	void restoreAuthStore() noexcept
	{
		std::scoped_lock lock(mutex_);
		restoreFromConfig("googleOAuth2ClientCredentials", googleOAuth2ClientCredentials_);
		restoreFromConfig("googleTokenState", googleTokenState_);
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
			logger_->logException(e, fmt::format("StoreError(saveToConfig):{}", key));
		} catch (...) {
			logger_->error("UnknownError(saveToConfig):{}", key);
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
				logger_->logException(e, fmt::format("RestoreError(restoreFromConfig):{}", key));
			} catch (...) {
				logger_->error("UnknownError(RestoreError):{}", key);
			}
		}
	}

	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	Auth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials_;
	Auth::GoogleTokenState googleTokenState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
