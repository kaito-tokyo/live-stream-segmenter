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

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <ILogger.hpp>
#include <NullLogger.hpp>
#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class AuthStore {
public:
	AuthStore() = default;
	~AuthStore() noexcept = default;

	AuthStore(const AuthStore &) = delete;
	AuthStore &operator=(const AuthStore &) = delete;
	AuthStore(AuthStore &&) = delete;
	AuthStore &operator=(AuthStore &&) = delete;

	void setLogger(std::shared_ptr<const Logger::ILogger> logger)
	{
		logger_ = std::move(logger);
	}

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
		logger_->error("Error", {{"exception", e.what()}});
		googleOAuth2ClientCredentials_ = GoogleAuth::GoogleOAuth2ClientCredentials{};
		googleTokenState_ = GoogleAuth::GoogleTokenState{};
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
		return profilePath / "live-stream-segmenter_AuthStore.json";
	}

	mutable std::mutex mutex_;
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials_;
	GoogleAuth::GoogleTokenState googleTokenState_;

	std::shared_ptr<const Logger::ILogger> logger_{std::make_shared<Logger::NullLogger>()};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
