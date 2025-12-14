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

struct GoogleOAuth2ClientCredential {
	std::string ver = "1.0";
	std::string client_id;
	std::string client_secret;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GoogleOAuth2ClientCredential, ver, client_id, client_secret)
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
