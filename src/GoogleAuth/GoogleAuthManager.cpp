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

#include "GoogleAuthManager.hpp"

#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlWriteCallback.hpp>

namespace KaitoTokyo::GoogleAuth {

GoogleAuthManager::GoogleAuthManager(CURL *curl, GoogleOAuth2ClientCredentials clientCredentials,
				     std::shared_ptr<const Logger::ILogger> logger)
	: curl_(curl ? curl : throw std::invalid_argument("CurlIsNullError(GoogleAuthManager)")),
	  clientCredentials_((clientCredentials_.client_id.empty() || clientCredentials_.client_secret.empty())
				     ? std::move(clientCredentials)
				     : throw std::runtime_error("CredentialsMissingError(GoogleAuthManager)")),
	  logger_(std::move(logger))
{
}

GoogleAuthManager::~GoogleAuthManager() noexcept = default;

GoogleAuthResponse GoogleAuthManager::fetchFreshAuthResponse(std::string refreshToken) const
{
	curl_easy_reset(curl_);

	CurlHelper::CurlUrlSearchParams postParams(curl_);
	postParams.append("client_id", clientCredentials_.client_id);
	postParams.append("client_secret", clientCredentials_.client_secret);
	postParams.append("refresh_token", std::move(refreshToken));
	postParams.append("grant_type", "refresh_token");

	std::vector<char> readBuffer;
	std::string postData = postParams.toString();

	curl_easy_reset(curl_);

	curl_easy_setopt(curl_, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 2L);
	curl_easy_setopt(curl_, CURLOPT_POST, 1L);
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, postData.c_str());
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(postData.length()));

	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl_);

	if (res != CURLE_OK) {
		logger_->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("NetworkError(fetchFreshAuthResponse)");
	}

	nlohmann::json j = nlohmann::json::parse(readBuffer);
	if (j.contains("error")) {
		std::string errorJson = j["error"].dump();
		logger_->error("GoogleOAuth2Error", {{"error", errorJson}});
		throw std::runtime_error("APIError(fetchFreshAuthResponse)");
	}

	return j.get<GoogleAuthResponse>();
}

} // namespace KaitoTokyo::GoogleAuth
