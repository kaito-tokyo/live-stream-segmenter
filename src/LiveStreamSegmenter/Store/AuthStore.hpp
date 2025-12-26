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

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <ILogger.hpp>
#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class AuthStore : public std::enable_shared_from_this<AuthStore> {
public:
	explicit AuthStore(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {};
	~AuthStore() noexcept = default;

	AuthStore(const AuthStore &) = delete;
	AuthStore &operator=(const AuthStore &) = delete;
	AuthStore(AuthStore &&) = delete;
	AuthStore &operator=(AuthStore &&) = delete;

	void setGoogleOAuth2ClientCredentials(GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials)
	{
		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredentials_ = std::move(googleOAuth2ClientCredentials);
	}

	GoogleAuth::GoogleOAuth2ClientCredentials getGoogleOAuth2ClientCredentials() const
	{
		std::scoped_lock lock(mutex_);
		return googleOAuth2ClientCredentials_;
	}

	void setGoogleTokenState(const GoogleAuth::GoogleTokenState &tokenState)
	{
		std::scoped_lock lock(mutex_);
		googleTokenState_ = tokenState;
	}

	GoogleAuth::GoogleTokenState getGoogleTokenState() const
	{
		std::scoped_lock lock(mutex_);
		return googleTokenState_;
	}

	bool saveAuthStore() const noexcept
	try {
		std::filesystem::path configPath = getConfigPath();
		if (configPath.empty()) {
			return false;
		}

		std::scoped_lock lock(mutex_);

		nlohmann::json j{
			{"googleOAuth2ClientCredentials", googleOAuth2ClientCredentials_},
			{"googleTokenState", googleTokenState_},
		};

		std::ofstream ofs(configPath, std::ios::out | std::ios::trunc);
		ofs << j.dump();

		return true;
	} catch (const std::exception &e) {
		logger_->error("Error(AuthStore::saveAuthStore):{}", e.what());
		return false;
	} catch (...) {
		logger_->error("UnknownError(AuthStore::saveAuthStore)");
		return false;
	}

	void restoreAuthStore() noexcept
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

			googleOAuth2ClientCredentials_ =
				j.value("googleOAuth2ClientCredentials", GoogleAuth::GoogleOAuth2ClientCredentials{});
			googleTokenState_ = j.value("googleTokenState", GoogleAuth::GoogleTokenState{});
		}
	} catch (const std::exception &e) {
		logger_->error("Error(AuthStore::restoreAuthStore):{}", e.what());
		googleOAuth2ClientCredentials_ = GoogleAuth::GoogleOAuth2ClientCredentials{};
		googleTokenState_ = GoogleAuth::GoogleTokenState{};
	} catch (...) {
		logger_->error("UnknownError(AuthStore::restoreAuthStore)");
		googleOAuth2ClientCredentials_ = GoogleAuth::GoogleOAuth2ClientCredentials{};
		googleTokenState_ = GoogleAuth::GoogleTokenState{};
	}

private:
	std::filesystem::path getConfigPath() const
	{
		BridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
		if (!profilePathRaw) {
			logger_->error("ProfilePathError(AuthStore::getConfigPath)");
			return {};
		}

		std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
		return profilePath / "live-stream-segmenter_AuthStore.json";
	}

	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials_;
	GoogleAuth::GoogleTokenState googleTokenState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
