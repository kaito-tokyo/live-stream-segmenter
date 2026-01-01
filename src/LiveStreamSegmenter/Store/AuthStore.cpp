/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * Live Stream Segmenter - Store Module
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

#include "AuthStore.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <NullLogger.hpp>
#include <ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

AuthStore::AuthStore() : logger_{Logger::NullLogger::instance()} {}

AuthStore::~AuthStore() noexcept = default;

std::filesystem::path AuthStore::getConfigPath()
{
	ObsBridgeUtils::unique_bfree_char_t profilePathRaw(obs_frontend_get_current_profile_path());
	if (!profilePathRaw) {
		throw std::runtime_error("GetCurrentProfilePathFailed(getConfigPath)");
	}

	std::filesystem::path profilePath(reinterpret_cast<const char8_t *>(profilePathRaw.get()));
	return profilePath / "live-stream-segmenter_AuthStore.json";
}

void AuthStore::setLogger(std::shared_ptr<const Logger::ILogger> logger)
{
	std::scoped_lock lock(mutex_);
	logger_ = std::move(logger);
}

void AuthStore::setGoogleOAuth2ClientCredentials(GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials)
{
	std::scoped_lock lock(mutex_);
	googleOAuth2ClientCredentials_ = std::move(googleOAuth2ClientCredentials);
}

GoogleAuth::GoogleOAuth2ClientCredentials AuthStore::getGoogleOAuth2ClientCredentials() const
{
	std::scoped_lock lock(mutex_);
	return googleOAuth2ClientCredentials_;
}

void AuthStore::setGoogleTokenState(const GoogleAuth::GoogleTokenState &tokenState)
{
	std::scoped_lock lock(mutex_);
	googleTokenState_ = tokenState;
}

GoogleAuth::GoogleTokenState AuthStore::getGoogleTokenState() const
{
	std::scoped_lock lock(mutex_);
	return googleTokenState_;
}

void AuthStore::save() const
{
	const std::filesystem::path configPath = getConfigPath();

	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials;
	GoogleAuth::GoogleTokenState googleTokenState;
	{
		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredentials = googleOAuth2ClientCredentials_;
		googleTokenState = googleTokenState_;
	}

	nlohmann::json j{
		{"googleOAuth2ClientCredentials", googleOAuth2ClientCredentials},
		{"googleTokenState", googleTokenState},
	};

	std::filesystem::path tmpConfigPath = configPath;
	tmpConfigPath += ".tmp";
	std::ofstream ofs(tmpConfigPath, std::ios::out);
	if (!ofs.is_open()) {
		logger_->error("FileOpenError", {{"path", configPath.string()}});
		throw std::runtime_error("FileOpenError(save)");
	}

	ofs << j;

	ofs.close();

	std::filesystem::path bakConfigPath = configPath;
	bakConfigPath += ".bak";

	std::filesystem::rename(configPath, bakConfigPath);
	std::filesystem::rename(tmpConfigPath, configPath);
}

void AuthStore::restore()
{
	const std::filesystem::path configPath = getConfigPath();
	if (!std::filesystem::is_regular_file(configPath)) {
		logger_->info("AuthStoreConfigFileNotExist", {{"path", configPath.string()}});
		return;
	}

	std::ifstream ifs(configPath, std::ios::in);
	if (!ifs.is_open()) {
		logger_->error("FileOpenError", {{"path", configPath.string()}});
		throw std::runtime_error("FileOpenError(restore)");
	}

	nlohmann::json j;
	ifs >> j;

	std::scoped_lock lock(mutex_);
	try {
		if (j.contains("googleOAuth2ClientCredentials")) {
			j.get_to(googleOAuth2ClientCredentials_);
			logger_->info("RestoredGoogleOAuth2ClientCredentials");
		}
		if (j.contains("googleTokenState")) {
			j.get_to(googleTokenState_);
			logger_->info("RestoredGoogleTokenState");
		}
	} catch (...) {
		googleOAuth2ClientCredentials_ = {};
		googleTokenState_ = {};
		throw;
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
