/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <mutex>
#include <memory>

#include <ILogger.hpp>

#include "GoogleTokenState.hpp"
#include "GoogleTokenStorage.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleTokenManager {
public:
	GoogleTokenManager(std::string clientId, std::string clientSecret, const Logger::ILogger &logger,
			   const GoogleTokenStorage &storage = GoogleTokenStorage())
		: clientId_(std::move(clientId)),
		  clientSecret_(std::move(clientSecret)),
		  logger_(logger),
		  storage_(storage)
	{
	}

	~GoogleTokenManager() = default;

	void setCredentials(const std::string &clientId, const std::string &clientSecret);

	bool isAuthenticated() const;
	std::string currentChannelName() const;

	void updateAuthState(const GoogleTokenState &state);
	void clear();

	std::string getAccessToken();

private:
	void performRefresh();

	std::string clientId_;
	std::string clientSecret_;

	const GoogleTokenStorage &storage_;
	const Logger::ILogger &logger_;

	mutable std::mutex mutex_;
	GoogleTokenState currentTokenState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
