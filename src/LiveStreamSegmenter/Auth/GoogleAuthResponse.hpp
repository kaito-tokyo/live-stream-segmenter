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

#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

struct GoogleTokenResponse {
	std::string access_token;
	std::optional<int> expires_in;
	std::optional<std::string> token_type;
	std::optional<std::string> refresh_token;
	std::optional<std::string> scope;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GoogleTokenResponse, access_token, expires_in, token_type,
						    refresh_token, scope)
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth