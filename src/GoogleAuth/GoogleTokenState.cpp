/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo GoogleAuth Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "GoogleTokenState.hpp"

#include <nlohmann/json.hpp>

namespace KaitoTokyo::GoogleAuth {

[[nodiscard]]
std::chrono::system_clock::time_point GoogleTokenState::expirationTimePoint() const
{
	if (expires_at.has_value()) {
		return std::chrono::system_clock::time_point(std::chrono::seconds(expires_at.value()));
	} else {
		return {};
	}
}

[[nodiscard]]
bool GoogleTokenState::isAuthorized() const
{
	return !refresh_token.empty();
}

[[nodiscard]]
bool GoogleTokenState::isAccessTokenFresh() const
{
	if (access_token.empty() || !expires_at.has_value()) {
		return false;
	}

	auto now = std::chrono::system_clock::now();

	const auto timeMargin = std::chrono::seconds(60);

	return (now + timeMargin) < expirationTimePoint();
}

void GoogleTokenState::loadAuthResponse(const GoogleAuthResponse &response)
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

void from_json(const nlohmann::json &j, GoogleTokenState &p)
{
	j.at("ver").get_to(p.ver);
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

void to_json(nlohmann::json &j, const GoogleTokenState &p)
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

} // namespace KaitoTokyo::GoogleAuth
