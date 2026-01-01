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

#pragma once

#include "GoogleOAuth2Flow.hpp"

#include <stdexcept>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <CurlUrlHandle.hpp>
#include <CurlUrlSearchParams.hpp>
#include <CurlWriteCallback.hpp>

#include <Task.hpp>

namespace KaitoTokyo::GoogleAuth {

GoogleOAuth2Flow::GoogleOAuth2Flow(CURL *curl, GoogleOAuth2ClientCredentials clientCredentials, std::string scopes,
				   std::shared_ptr<const Logger::ILogger> logger)
	: curl_(curl ? curl : throw std::invalid_argument("CurlIsNullError(GoogleOAuth2Flow)")),
	  clientCredentials_(std::move(clientCredentials)),
	  scopes_(std::move(scopes)),
	  logger_(std::move(logger))
{
}

GoogleOAuth2Flow::~GoogleOAuth2Flow() = default;

std::string GoogleOAuth2Flow::getAuthorizationUrl(std::string redirectUri) const
{
	CurlHelper::CurlUrlSearchParams params(curl_);
	params.append("client_id", clientCredentials_.client_id);
	params.append("redirect_uri", std::move(redirectUri));
	params.append("response_type", "code");
	params.append("scope", scopes_);
	params.append("access_type", "offline");
	params.append("prompt", "consent");
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://accounts.google.com/o/oauth2/v2/auth");
	urlHandle.appendQuery(qs.c_str());

	auto url = urlHandle.c_str();
	return std::string(url.get());
}

std::optional<GoogleAuthResponse> GoogleOAuth2Flow::exchangeCodeForToken(const std::string &code,
									 const std::string &redirectUri)
{
	logger_->info("GoogleOAuth2FlowTokenExchanging");
	auto result = exchangeCode(code, redirectUri);
	logger_->info("GoogleOAuth2FlowTokenExchanged");
	return result;
}

GoogleAuthResponse GoogleOAuth2Flow::exchangeCode(std::string code, std::string redirectUri)
{
	CurlHelper::CurlUrlSearchParams params(curl_);

	params.append("client_id", clientCredentials_.client_id);
	params.append("client_secret", clientCredentials_.client_secret);
	params.append("code", std::move(code));
	params.append("grant_type", "authorization_code");
	params.append("redirect_uri", std::move(redirectUri));
	const std::string postData = params.toString();

	std::vector<char> readBuffer;
	curl_easy_setopt(curl_, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl_, CURLOPT_POST, 1L);
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postData.c_str());
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(postData.length()));

	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

	const CURLcode res = curl_easy_perform(curl_);

	if (res != CURLE_OK) {
		logger_->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(exchangeCode)");
	}

	const nlohmann::json j = nlohmann::json::parse(readBuffer.begin(), readBuffer.end());
	if (j.contains("error")) {
		logger_->error("APIError", {{"error", j["error"].dump()}});
		throw std::runtime_error("APIError(exchangeCode)");
	}

	return j.get<GoogleAuthResponse>();
}

} // namespace KaitoTokyo::GoogleAuth
