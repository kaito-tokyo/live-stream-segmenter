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

	[[nodiscard]]
	std::chrono::system_clock::time_point expirationTimePoint() const
	{
		if (expires_at.has_value()) {
			return std::chrono::system_clock::time_point(std::chrono::seconds(expires_at.value()));
		} else {
			return {};
		}
	}

	[[nodiscard]]
	bool isAuthorized() const
	{
		return !refresh_token.empty();
	}

	[[nodiscard]]
	bool isAccessTokenFresh() const
	{
		if (access_token.empty() || !expires_at.has_value()) {
			return false;
		}

		auto now = std::chrono::system_clock::now();

		const auto timeMargin = std::chrono::seconds(60);

		return (now + timeMargin) < expirationTimePoint();
	}

	GoogleTokenState withUpdatedAuthResponse(const GoogleAuthResponse &response) const
	{
		GoogleTokenState updatedState = *this;

		updatedState.access_token = response.access_token;

		if (response.expires_in.has_value()) {
			auto now = std::chrono::duration_cast<std::chrono::seconds>(
					   std::chrono::system_clock::now().time_since_epoch())
					   .count();
			updatedState.expires_at = now + response.expires_in.value();
		}

		if (response.scope.has_value()) {
			updatedState.scope = response.scope.value();
		}

		if (response.refresh_token.has_value() && !response.refresh_token->empty()) {
			updatedState.refresh_token = response.refresh_token.value();
		}
		return updatedState;
	}
};

inline void from_json(const nlohmann::json &j, GoogleTokenState &p)
{
	if (auto it = j.find("ver"); it != j.end()) {
		it->get_to(p.ver);
	}

	j.at("access_token").get_to(p.access_token);
	j.at("refresh_token").get_to(p.refresh_token);
	j.at("email").get_to(p.email);
	j.at("scope").get_to(p.scope);

	if (auto it = j.find("expires_at"); it != j.end() && !it->is_null()) {
		it->get_to(p.expires_at.emplace());
	} else {
		p.expires_at = std::nullopt;
	}
}

inline void to_json(nlohmann::json &j, const GoogleTokenState &p)
{
	j = nlohmann::json{
		{"ver", p.ver},
		{"access_token", p.access_token},
		{"refresh_token", p.refresh_token},
		{"email", p.email},
		{"scope", p.scope},
	};

	if (p.expires_at.has_value())
		j["expires_at"] = *p.expires_at;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
