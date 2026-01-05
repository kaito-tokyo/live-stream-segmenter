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

#include <cassert>
#include <stdexcept>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <KaitoTokyo/CurlHelper/CurlHandle.hpp>
#include <KaitoTokyo/CurlHelper/CurlUrlHandle.hpp>
#include <KaitoTokyo/CurlHelper/CurlUrlSearchParams.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>

namespace KaitoTokyo::GoogleAuth {

GoogleOAuth2Flow::GoogleOAuth2Flow(std::shared_ptr<const Logger::ILogger> logger,
				   std::shared_ptr<CurlHelper::CurlHandle> curl,
				   std::shared_ptr<GoogleOAuth2ClientCredentials> clientCredentials, std::string scopes)
	: logger_(std::move(logger)),
	  curl_(std::move(curl)),
	  clientCredentials_(std::move(clientCredentials)),
	  scopes_(std::move(scopes))
{
	assert(logger_);
	if (!curl_) {
		logger_->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(GoogleOAuth2Flow)");
	}
	if (!clientCredentials_) {
		logger_->error("ClientCredentialsIsNullError");
		throw std::invalid_argument("ClientCredentialsIsNullError(GoogleOAuth2Flow)");
	}
	if (scopes_.empty()) {
		logger_->error("ScopesIsEmptyError");
		throw std::invalid_argument("ScopesIsEmptyError(GoogleOAuth2Flow)");
	}
}

GoogleOAuth2Flow::~GoogleOAuth2Flow() noexcept = default;

std::string GoogleOAuth2Flow::getAuthorizationUrl(const std::string &redirectUri) const
{
	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("client_id", clientCredentials_->client_id);
	params.append("redirect_uri", redirectUri);
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

GoogleAuthResponse GoogleOAuth2Flow::exchangeCode(Jthread::stop_token stoken, const std::string &code,
						  const std::string &redirectUri)
{
	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());

	params.append("client_id", clientCredentials_->client_id);
	params.append("client_secret", clientCredentials_->client_secret);
	params.append("code", code);
	params.append("grant_type", "authorization_code");
	params.append("redirect_uri", redirectUri);
	const std::string postData = params.toString();

	std::vector<char> readBuffer;

	curl_easy_reset(curl_->getRaw());

	curl_easy_setopt(curl_->getRaw(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
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

	const CURLcode res = curl_easy_perform(curl_->getRaw());
	if (res != CURLE_OK) {
		logger_->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(exchangeCode)");
	}

	curl_easy_setopt(curl_->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl_->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	const nlohmann::json j = nlohmann::json::parse(readBuffer);
	if (j.contains("error")) {
		logger_->error("APIError", {{"error", j["error"].dump()}});
		throw std::runtime_error("APIError(exchangeCode)");
	}

	return j.get<GoogleAuthResponse>();
}

} // namespace KaitoTokyo::GoogleAuth
