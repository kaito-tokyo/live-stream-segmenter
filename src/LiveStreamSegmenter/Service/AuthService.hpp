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

#include <nlohmann/json.hpp>

#include <obs.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <ILogger.hpp>

#include <GoogleAuthManager.hpp>
#include <GoogleOAuth2ClientCredentials.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Service {

class AuthService : public std::enable_shared_from_this<AuthService> {
public:
	explicit AuthService(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~AuthService() noexcept = default;

	AuthService(const AuthService &) = delete;
	AuthService &operator=(const AuthService &) = delete;
	AuthService(AuthService &&) = delete;
	AuthService &operator=(AuthService &&) = delete;

	void setGoogleOAuth2ClientCredentials(Auth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials)
	{
		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredentials_ = std::move(googleOAuth2ClientCredentials);

		Auth::GoogleAuthManagerCallback googleAuthManagerCallback;
		googleAuthManagerCallback.onTokenStore = [logger = logger_](Auth::GoogleTokenState tokenState) {
			config_t *config = obs_frontend_get_profile_config();
			nlohmann::json j = tokenState;
			try {
				std::string dump = j.dump();
				config_set_string(config, PLUGIN_NAME, "googleTokenState", dump.c_str());
				config_save(config);
			} catch (const std::exception &e) {
				logger->logException(e, "StoreError(onTokenStore)");
			} catch (...) {
				logger->error("UnknownError(onTokenStore)");
			}
		};
		googleAuthManagerCallback.onTokenInvalidate = [logger = logger_]() {
			config_t *config = obs_frontend_get_profile_config();
			config_remove_value(config, PLUGIN_NAME, "googleTokenState");
		};

		config_t *config = obs_frontend_get_profile_config();
		try {
			nlohmann::json j =
				nlohmann::json::parse(config_get_string(config, PLUGIN_NAME, "googleTokenState"));
			googleAuthManager_ = std::make_shared<Auth::GoogleAuthManager>(googleOAuth2ClientCredentials,
										       googleAuthManagerCallback,
										       logger_,
										       j.get<Auth::GoogleTokenState>());
			return;
		} catch (const std::exception &e) {
			logger_->logException(e, "RestoreError(setGoogleOAuth2ClientCredentials)");
		} catch (...) {
			logger_->error("UnknownError(setGoogleOAuth2ClientCredentials)");
		}

		googleAuthManager_ = std::make_shared<Auth::GoogleAuthManager>(googleOAuth2ClientCredentials,
									       googleAuthManagerCallback, logger_);
	}

	Auth::GoogleOAuth2ClientCredentials getGoogleOAuth2ClientCredentials() const
	{
		std::scoped_lock lock(mutex_);
		return googleOAuth2ClientCredentials_;
	}

	void storeAuthService() noexcept { storeGoogleOAuth2ClientCredentials(); }

	void restoreAuthService() noexcept { restoreGoogleOAuth2ClientCredentials(); }

	std::shared_ptr<Auth::GoogleAuthManager> getGoogleAuthManager() const
	{
		std::scoped_lock lock(mutex_);
		return googleAuthManager_;
	}

private:
	void storeGoogleOAuth2ClientCredentials() noexcept
	{
		std::scoped_lock lock(mutex_);

		config_t *config = obs_frontend_get_profile_config();
		try {
			nlohmann::json j = googleOAuth2ClientCredentials_;
			std::string jsonString = j.dump();
			config_set_string(config, PLUGIN_NAME, "googleOAuth2ClientCredentials", jsonString.c_str());
			config_save(config);
			logger_->info("Google OAuth2 client credentials stored successfully.");
		} catch (const std::exception &e) {
			logger_->logException(e, "StoreError(saveGoogleOAuth2ClientCredentials)");
		} catch (...) {
			logger_->error("UnknownError(saveGoogleOAuth2ClientCredentials)");
		}
	}

	void restoreGoogleOAuth2ClientCredentials() noexcept
	{
		std::scoped_lock lock(mutex_);

		config_t *config = obs_frontend_get_profile_config();
		const char *jsonCstr = config_get_string(config, PLUGIN_NAME, "googleOAuth2ClientCredentials");
		if (jsonCstr == nullptr || jsonCstr[0] == '\0') {
			logger_->info("No Google OAuth2 client credential found in config.");
		} else {
			try {
				nlohmann::json j = nlohmann::json::parse(jsonCstr);
				setGoogleOAuth2ClientCredentials(j.get<Auth::GoogleOAuth2ClientCredentials>());
				logger_->info("Google OAuth2 client credentials restored successfully.");
			} catch (const std::exception &e) {
				logger_->logException(e, "RestoreError(restoreGoogleOAuth2ClientCredential)");
			} catch (...) {
				logger_->error("UnknownError(restoreGoogleOAuth2ClientCredential)");
			}
		}
	}

	const std::shared_ptr<const Logger::ILogger> logger_;

	Auth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials_;

	mutable std::recursive_mutex mutex_;
	std::shared_ptr<Auth::GoogleAuthManager> googleAuthManager_ = nullptr;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Service
