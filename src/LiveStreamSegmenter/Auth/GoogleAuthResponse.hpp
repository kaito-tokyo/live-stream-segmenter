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

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(GoogleTokenResponse, access_token, expires_in, token_type, refresh_token, scope)
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
