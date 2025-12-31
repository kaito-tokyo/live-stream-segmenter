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

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <curl/curl.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include <Task.hpp>

#include "GoogleAuthResponse.hpp"
#include "GoogleOAuth2ClientCredentials.hpp"

namespace KaitoTokyo::GoogleAuth {

class GoogleOAuth2Flow {
public:
	GoogleOAuth2Flow(GoogleOAuth2ClientCredentials clientCredentials, std::string scopes,
			 std::shared_ptr<const Logger::ILogger> logger)
		: clientCredentials_(std::move(clientCredentials)),
		  scopes_(std::move(scopes)),
		  logger_(std::move(logger))
	{
	}

	~GoogleOAuth2Flow() = default;

	GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
	GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

	std::string getAuthorizationUrl(const std::string &redirectUri) const
	{
		const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl)
			throw std::runtime_error("InitError(GoogleOAuth2Flow::getAuthorizationUrl)");

		CurlHelper::CurlUrlSearchParams qp(curl.get());
		qp.append("client_id", clientCredentials_.client_id);
		qp.append("redirect_uri", redirectUri);
		qp.append("response_type", "code");
		qp.append("scope", scopes_);
		qp.append("access_type", "offline");
		qp.append("prompt", "consent");

		return fmt::format("https://accounts.google.com/o/oauth2/v2/auth?{}", qp.toString());
	}

	std::optional<GoogleAuthResponse> exchangeCodeForToken(const std::string &code, const std::string &redirectUri)
	{
		try {
			logger_->info("CodeReceived");
			auto result = exchangeCode(clientCredentials_, code, redirectUri);
			logger_->info("TokenExchanged");
			return result;
		} catch (const std::exception &e) {
			logger_->error("GoogleOAuth2FlowExchangeFailed", {{"exception", e.what()}});
			throw;
		}
	}

private:
	[[nodiscard]]
	static GoogleAuthResponse exchangeCode(const GoogleOAuth2ClientCredentials &clientCredentials,
					       const std::string &code, const std::string &redirectUri)
	{
		using nlohmann::json;
		using namespace CurlHelper;

		const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl)
			throw std::runtime_error("InitError(GoogleOAuth2Flow::exchangeCode)");

		CurlVectorWriterBuffer readBuffer;
		CurlUrlSearchParams params(curl.get());

		params.append("client_id", clientCredentials.client_id);
		params.append("client_secret", clientCredentials.client_secret);
		params.append("code", code);
		params.append("grant_type", "authorization_code");
		params.append("redirect_uri", redirectUri);

		const std::string postData = params.toString();

		curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

		const CURLcode res = curl_easy_perform(curl.get());

		if (res != CURLE_OK) {
			throw std::runtime_error(fmt::format("NetworkError(GoogleOAuth2Flow::exchangeCode): {}",
							     curl_easy_strerror(res)));
		}

		const json j = json::parse(readBuffer.begin(), readBuffer.end());
		if (j.contains("error")) {
			throw std::runtime_error(fmt::format("APIError(GoogleOAuth2Flow::exchangeCode): {}", j.dump()));
		}

		return j.get<GoogleAuthResponse>();
	}

	const GoogleOAuth2ClientCredentials clientCredentials_;
	const std::string scopes_;
	const std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::GoogleAuth
