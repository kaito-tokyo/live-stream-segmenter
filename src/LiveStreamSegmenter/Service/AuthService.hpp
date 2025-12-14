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
#include <GoogleOAuth2ClientCrendential.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Service {

class AuthService : public std::enable_shared_from_this<AuthService> {
public:
	explicit AuthService(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~AuthService() noexcept = default;

	AuthService(const AuthService &) = delete;
	AuthService &operator=(const AuthService &) = delete;
	AuthService(AuthService &&) = delete;
	AuthService &operator=(AuthService &&) = delete;

	void setGoogleOAuth2ClientCredential(Auth::GoogleOAuth2ClientCredential googleOAuth2ClientCredential)
	{
		using nlohmann::json;
		using namespace Auth;

		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredential_ = std::move(googleOAuth2ClientCredential);

		GoogleAuthManagerCallback googleAuthManagerCallback;
		googleAuthManagerCallback.onTokenStore = [logger = logger_](GoogleTokenState tokenState) {
			config_t *config = obs_frontend_get_profile_config();
			json j = tokenState;
			try {
				std::string dump = j.dump();
				config_set_string(config, PLUGIN_NAME, "googleTokenState", dump.c_str());
			} catch (const std::exception &e) {
				logger->logException(e, "StoreError(onTokenStore)");
			} catch (...) {
				logger->error("UnknownError(onTokenStore)");
			}
		};
		googleAuthManagerCallback.onTokenRestore = [logger = logger_]() -> std::optional<GoogleTokenState> {
			config_t *config = obs_frontend_get_profile_config();
			try {
				json j = json::parse(config_get_string(config, PLUGIN_NAME, "googleTokenState"));
				return j.get<GoogleTokenState>();
			} catch (const std::exception &e) {
				logger->logException(e, "RestoreError(onTokenRestore)");
				return std::nullopt;
			} catch (...) {
				logger->error("UnknownError(onTokenRestore)");
				return std::nullopt;
			}
		};
		googleAuthManagerCallback.onTokenInvalidate = [logger = logger_]() {
			config_t *config = obs_frontend_get_profile_config();
			config_remove_value(config, PLUGIN_NAME, "googleTokenState");
		};

		googleAuthManager_ = std::make_shared<GoogleAuthManager>(googleOAuth2ClientCredential,
									 googleAuthManagerCallback, logger_);
	}

	void restoreGoogleOAuth2ClientCredential() noexcept
	{
		using nlohmann::json;
		using namespace Auth;

		std::scoped_lock lock(mutex_);

		config_t *config = obs_frontend_get_profile_config();
		const char *jsonCstr = config_get_string(config, PLUGIN_NAME, "googleOAuth2ClientCredential");
		if (jsonCstr == nullptr || jsonCstr[0] == '\0') {
			logger_->info("No Google OAuth2 client credential found in config.");
		} else {
			try {
				json j = json::parse(jsonCstr);
				setGoogleOAuth2ClientCredential(j.get<GoogleOAuth2ClientCredential>());
			} catch (const std::exception &e) {
				logger_->logException(e, "RestoreError(restoreGoogleOAuth2ClientCredential)");
			} catch (...) {
				logger_->error("UnknownError(restoreGoogleOAuth2ClientCredential)");
			}
		}
	}

	void saveGoogleOAuth2ClientCredential() noexcept
	{
		using nlohmann::json;

		std::scoped_lock lock(mutex_);

		config_t *config = obs_frontend_get_profile_config();
		try {
			json j = googleOAuth2ClientCredential_;
			std::string jsonString = j.dump();
			config_set_string(config, PLUGIN_NAME, "googleOAuth2ClientCredential", jsonString.c_str());
		} catch (const std::exception &e) {
			logger_->logException(e, "StoreError(saveGoogleOAuth2ClientCredential)");
		} catch (...) {
			logger_->error("UnknownError(saveGoogleOAuth2ClientCredential)");
		}
	}

	std::shared_ptr<Auth::GoogleAuthManager> getGoogleAuthManager() const
	{
		std::scoped_lock lock(mutex_);
		return googleAuthManager_;
	}

private:
	const std::shared_ptr<const Logger::ILogger> logger_;

	Auth::GoogleOAuth2ClientCredential googleOAuth2ClientCredential_;

	mutable std::recursive_mutex mutex_;
	std::shared_ptr<Auth::GoogleAuthManager> googleAuthManager_ = nullptr;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Service
