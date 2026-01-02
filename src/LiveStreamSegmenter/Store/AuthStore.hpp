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

#pragma once

#include <filesystem>
#include <memory>
#include <mutex>

#include <GoogleOAuth2ClientCredentials.hpp>
#include <GoogleTokenState.hpp>
#include <ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

class AuthStore {
public:
	AuthStore();

	~AuthStore() noexcept;

	AuthStore(const AuthStore &) = delete;
	AuthStore &operator=(const AuthStore &) = delete;
	AuthStore(AuthStore &&) = delete;
	AuthStore &operator=(AuthStore &&) = delete;

	static std::filesystem::path getConfigPath();

	void setLogger(std::shared_ptr<const Logger::ILogger> logger);

	void setGoogleOAuth2ClientCredentials(GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials);

	GoogleAuth::GoogleOAuth2ClientCredentials getGoogleOAuth2ClientCredentials() const;

	void setGoogleTokenState(const GoogleAuth::GoogleTokenState &tokenState);

	GoogleAuth::GoogleTokenState getGoogleTokenState() const;

	void save() const;

	void restore();

private:
	mutable std::mutex mutex_;
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials_;
	GoogleAuth::GoogleTokenState googleTokenState_;

	std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
