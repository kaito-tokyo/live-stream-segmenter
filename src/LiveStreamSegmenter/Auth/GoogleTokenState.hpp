/*
Live Stream Segmenter

MIT License

Copyright (c) 2025 Kaito Udagawa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GoogleTokenState, access_token, refresh_token, email, scope,
						    expires_at)
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
