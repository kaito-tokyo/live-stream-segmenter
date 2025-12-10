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

#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleTokenState.hpp"
#include "GoogleTokenStorage.hpp"

using namespace KaitoTokyo::CurlHelper;

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleAuthManager {
public:
	GoogleAuthManager(std::string clientId, std::string clientSecret, std::shared_ptr<const Logger::ILogger> logger,
			  std::unique_ptr<GoogleTokenStorage> storage = std::make_unique<GoogleTokenStorage>())
		: clientId_(std::move(clientId)),
		  clientSecret_(std::move(clientSecret)),
		  logger_(std::move(logger)),
		  storage_(std::move(storage))
	{
		if (auto savedTokenState = storage_->load()) {
			std::lock_guard<std::mutex> lock(mutex_);
			currentTokenState_ = *savedTokenState;
		}
	}

	~GoogleAuthManager() noexcept = default;

	GoogleAuthManager(const GoogleAuthManager &) = delete;
	GoogleAuthManager &operator=(const GoogleAuthManager &) = delete;
	GoogleAuthManager(GoogleAuthManager &&) = delete;
	GoogleAuthManager &operator=(GoogleAuthManager &&) = delete;

	bool isAuthenticated() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return currentTokenState_.isAuthorized();
	};

	void updateTokenState(const GoogleTokenState &tokenState)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			currentTokenState_ = tokenState;
		}
		storage_->save(tokenState);
	}

	void clear()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			currentTokenState_.clear();
		}
		storage_->clear();
	}

	std::string getAccessToken()
	{
		std::optional<std::string> refreshToken;

		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (!currentTokenState_.isAuthorized()) {
				throw std::runtime_error("NotAuthorizedError(getAccessToken)");
			}

			if (currentTokenState_.isAccessTokenFresh()) {
				return currentTokenState_.access_token;
			}

			refreshToken = currentTokenState_.refresh_token;
		}

		if (clientId_.empty() || clientSecret_.empty()) {
			throw std::runtime_error("CredentialMissingError(getAccessToken)");
		}

		if (refreshToken.has_value()) {
			GoogleTokenResponse tokenResponse = refreshTokenState(*refreshToken);

			GoogleTokenState tokenState;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				currentTokenState_.updateFromTokenResponse(tokenResponse);
				tokenState = currentTokenState_;
			}

			storage_->save(tokenState);

			return tokenState.access_token;
		} else {
			throw std::runtime_error("TokenRefreshError(getAccessToken)");
		}
	}

private:
	GoogleTokenResponse refreshTokenState(const std::string &refreshToken)
	{
		using nlohmann::json;

		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl) {
			throw std::runtime_error("InitError(refreshTokenState)");
		}

		CurlVectorWriterType readBuffer;

		CurlUrlSearchParams postParams(curl.get());
		postParams.append("client_id", clientId_);
		postParams.append("client_secret", clientSecret_);
		postParams.append("refresh_token", refreshToken);
		postParams.append("grant_type", "refresh_token");

		std::string postData = postParams.toString();

		curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);       // 60 second timeout
		curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
		curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 5L);      // Max 5 redirects

		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

		CURLcode res = curl_easy_perform(curl.get());

		if (res != CURLE_OK) {
			std::string err = curl_easy_strerror(res);
			throw std::runtime_error("NetworkError:" + err);
		}

		json j = json::parse(readBuffer.begin(), readBuffer.end());

		if (j.contains("error")) {
			throw std::runtime_error("APIError(refreshTokenState):" + j["error"].dump());
		}

		return j.get<GoogleTokenResponse>();
	}

	const std::string clientId_;
	const std::string clientSecret_;

	std::shared_ptr<const Logger::ILogger> logger_;
	std::unique_ptr<GoogleTokenStorage> storage_;

	mutable std::mutex mutex_;
	GoogleTokenState currentTokenState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
