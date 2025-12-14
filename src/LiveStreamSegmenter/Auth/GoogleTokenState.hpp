/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "GoogleAuthResponse.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

using Timestamp = std::int64_t;

struct GoogleTokenState {
	std::string ver = "1.0";
	std::string access_token;
	std::string refresh_token;
	std::string email;
	std::string scope;

	std::optional<Timestamp> expires_at;

	std::chrono::system_clock::time_point expirationTimePoint() const
	{
		if (expires_at.has_value()) {
			return std::chrono::system_clock::time_point(std::chrono::seconds(expires_at.value()));
		} else {
			return {};
		}
	}

	bool isAuthorized() const { return !refresh_token.empty(); }

	bool isAccessTokenFresh() const
	{
		if (access_token.empty() || !expires_at.has_value()) {
			return false;
		}

		auto now = std::chrono::system_clock::now();

		const auto timeMargin = std::chrono::seconds(60);

		return (now + timeMargin) < expirationTimePoint();
	}

	void updateFromTokenResponse(const GoogleTokenResponse &response)
	{
		access_token = response.access_token;

		if (response.expires_in.has_value()) {
			auto now = std::chrono::duration_cast<std::chrono::seconds>(
					   std::chrono::system_clock::now().time_since_epoch())
					   .count();
			expires_at = now + response.expires_in.value();
		}

		if (response.scope.has_value()) {
			scope = response.scope.value();
		}

		if (response.refresh_token.has_value() && !response.refresh_token->empty()) {
			refresh_token = response.refresh_token.value();
		}
	}

	void clear()
	{
		access_token.clear();
		refresh_token.clear();
		email.clear();
		scope.clear();
		expires_at.reset();
	}

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(GoogleTokenState, ver, access_token, refresh_token, email, scope, expires_at)
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
