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

#include <cassert>

#include <nlohmann/json.hpp>

#include <KaitoTokyo/CurlHelper/CurlUrlSearchParams.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>

namespace KaitoTokyo::GoogleAuth {

GoogleAuthManager::GoogleAuthManager(std::shared_ptr<const Logger::ILogger> logger,
				     std::shared_ptr<CurlHelper::CurlHandle> curl,
				     std::shared_ptr<GoogleOAuth2ClientCredentials> clientCredentials)
	: logger_(logger ? std::move(logger) : Logger::NullLogger::instance()),
	  curl_(std::move(curl)),
	  clientCredentials_(std::move(clientCredentials))
{
	assert(logger_);
	if (!curl_) {
		logger_->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(GoogleAuthManager::GoogleAuthManager)");
	}
	if (!clientCredentials_) {
		logger_->error("ClientCredentialsIsNullError");
		throw std::invalid_argument("ClientCredentialsIsNullError(GoogleAuthManager::GoogleAuthManager)");
	}
}

GoogleAuthManager::~GoogleAuthManager() noexcept = default;

std::shared_ptr<GoogleAuthResponse> GoogleAuthManager::fetchFreshAuthResponse(Jthread::stop_token stoken,
									      const std::string &refreshToken) const
{
	if (stoken.stop_requested())
		throw std::runtime_error("OperationCancelled(GoogleAuthManager::fetchFreshAuthResponse)");

	CurlHelper::CurlUrlSearchParams postParams(curl_->getRaw());
	postParams.append("client_id", clientCredentials_->client_id);
	postParams.append("client_secret", clientCredentials_->client_secret);
	postParams.append("refresh_token", std::move(refreshToken));
	postParams.append("grant_type", "refresh_token");

	std::vector<char> readBuffer;
	std::string postData = postParams.toString();

	curl_easy_reset(curl_->getRaw());

	curl_easy_setopt(curl_->getRaw(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl_->getRaw(), CURLOPT_FOLLOWLOCATION, 2L);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_POSTFIELDS, postData.c_str());
	curl_easy_setopt(curl_->getRaw(), CURLOPT_POSTFIELDSIZE, static_cast<long>(postData.length()));

	curl_easy_setopt(curl_->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl_->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(
		curl_->getRaw(), CURLOPT_XFERINFOFUNCTION,
		+[](void *clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t) -> int {
			auto *stoken = static_cast<Jthread::stop_token *>(clientp);
			if (stoken && stoken->stop_requested()) {
				return 1;
			} else {
				return 0;
			}
		});
	curl_easy_setopt(curl_->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl_->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl_->getRaw());

	if (res == CURLE_ABORTED_BY_CALLBACK) {
		logger_->info("OperationCancelled");
		throw std::runtime_error("OperationCancelled(GoogleAuthManager::fetchFreshAuthResponse)");
	}

	if (res != CURLE_OK) {
		logger_->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("NetworkError(GoogleAuthManager::fetchFreshAuthResponse)");
	}

	nlohmann::json j = nlohmann::json::parse(readBuffer);
	if (j.contains("error")) {
		std::string errorJson = j["error"].dump();
		logger_->error("GoogleOAuth2Error", {{"error", errorJson}});
		throw std::runtime_error("APIError(GoogleAuthManager::fetchFreshAuthResponse)");
	}

	auto authResponse = std::make_shared<GoogleAuthResponse>();
	j.get_to(*authResponse);
	return authResponse;
}

} // namespace KaitoTokyo::GoogleAuth
