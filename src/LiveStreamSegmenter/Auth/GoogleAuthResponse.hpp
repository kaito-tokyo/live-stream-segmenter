/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

struct GoogleTokenResponse {
	std::string access_token;
	std::optional<int> expires_in;
	std::optional<std::string> token_type;
	std::optional<std::string> refresh_token;
	std::optional<std::string> scope;
};

inline void from_json(const nlohmann::json &j, GoogleTokenResponse &p)
{
	j.at("access_token").get_to(p.access_token);

	const auto set_optional = [&j](const char *key, auto &field) {
		if (auto it = j.find(key); it != j.end() && !it->is_null()) {
			it->get_to(field.emplace());
		} else {
			field = std::nullopt;
		}
	};

	set_optional("expires_in", p.expires_in);
	set_optional("token_type", p.token_type);
	set_optional("refresh_token", p.refresh_token);
	set_optional("scope", p.scope);
}

inline void to_json(nlohmann::json &j, const GoogleTokenResponse &response)
{
	j = nlohmann::json{
		{"access_token", response.access_token},
	};

	if (response.expires_in.has_value()) {
		j["expires_in"] = *response.expires_in;
	}

	if (response.token_type.has_value()) {
		j["token_type"] = *response.token_type;
	}

	if (response.refresh_token.has_value()) {
		j["refresh_token"] = *response.refresh_token;
	}

	if (response.scope.has_value()) {
		j["scope"] = *response.scope;
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
