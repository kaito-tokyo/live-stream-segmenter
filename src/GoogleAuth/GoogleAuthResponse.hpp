/*
 * KaitoTokyo GoogleAuth Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace KaitoTokyo::GoogleAuth {

struct GoogleAuthResponse {
	std::string ver = "1.0";
	std::string access_token;
	std::optional<int> expires_in;
	std::optional<std::string> token_type;
	std::optional<std::string> refresh_token;
	std::optional<std::string> scope;
};

inline void from_json(const nlohmann::json &j, GoogleAuthResponse &p)
{
	if (auto it = j.find("ver"); it != j.end()) {
		it->get_to(p.ver);
	}

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

inline void to_json(nlohmann::json &j, const GoogleAuthResponse &p)
{
	j = nlohmann::json{
		{"ver", p.ver},
		{"access_token", p.access_token},
	};

	if (p.expires_in.has_value())
		j["expires_in"] = *p.expires_in;

	if (p.token_type.has_value())
		j["token_type"] = *p.token_type;

	if (p.refresh_token.has_value())
		j["refresh_token"] = *p.refresh_token;

	if (p.scope.has_value())
		j["scope"] = *p.scope;
}

} // namespace KaitoTokyo::GoogleAuth
