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

#include "GoogleOAuth2Flow.hpp"

#include <utility>

namespace KaitoTokyo::GoogleAuth {

GoogleOAuth2Flow::GoogleOAuth2Flow(std::shared_ptr<CurlHelper::CurlHandle> curl,
				   GoogleOAuth2ClientCredentials clientCredentials, std::string scopes,
				   std::shared_ptr<const Logger::ILogger> logger)
	: curl_(std::move(curl)),
	  clientCredentials_(std::move(clientCredentials)),
	  scopes_(std::move(scopes)),
	  logger_(std::move(logger))
{
}

GoogleOAuth2Flow::~GoogleOAuth2Flow() noexcept = default;

std::string GoogleOAuth2Flow::getAuthorizationUrl(std::string redirectUri) const
{
	return "https://mocked.example.com/oauth2/auth?redirect_uri=" + redirectUri;
}

GoogleAuthResponse GoogleOAuth2Flow::exchangeCode([[maybe_unused]] std::string code,
						  [[maybe_unused]] std::string redirectUri)
{
	GoogleAuthResponse resp;
	resp.access_token = "mocked_access_token";
	resp.expires_in = 3600;
	resp.token_type = "Bearer";
	resp.refresh_token = "mocked_refresh_token";
	resp.scope = scopes_;
	return resp;
}

} // namespace KaitoTokyo::GoogleAuth
