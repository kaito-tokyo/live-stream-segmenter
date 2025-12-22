/*
 * KaitoTokyo GoogleAuth Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <optional>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleOAuth2ClientCredentials.hpp"
#include "GoogleTokenState.hpp"

namespace KaitoTokyo::GoogleAuth {

struct GoogleAuthManagerCallback {
	std::function<void(GoogleTokenState)> onTokenStore;
	std::function<void()> onTokenInvalidate;
};

class GoogleAuthManager {
public:
	GoogleAuthManager(GoogleOAuth2ClientCredentials clientCredentials, GoogleAuthManagerCallback callback,
			  std::shared_ptr<const Logger::ILogger> logger,
			  std::optional<GoogleTokenState> storedTokenState = std::nullopt)
		: clientCredentials_(std::move(clientCredentials)),
		  callback_(std::move(callback)),
		  logger_(std::move(logger))
	{
		if (storedTokenState.has_value()) {
			currentTokenState_ = storedTokenState.value();
		}
	}

	~GoogleAuthManager() noexcept = default;

	GoogleAuthManager(const GoogleAuthManager &) = delete;
	GoogleAuthManager &operator=(const GoogleAuthManager &) = delete;
	GoogleAuthManager(GoogleAuthManager &&) = delete;
	GoogleAuthManager &operator=(GoogleAuthManager &&) = delete;

	bool isAuthenticated() const
	{
		std::scoped_lock lock(mutex_);
		return currentTokenState_.isAuthorized();
	};

	void updateTokenState(const GoogleTokenState &tokenState)
	{
		{
			std::scoped_lock lock(mutex_);
			currentTokenState_ = tokenState;
		}
		callback_.onTokenStore(tokenState);
	}

	void clear()
	{
		{
			std::scoped_lock lock(mutex_);
			currentTokenState_.clear();
		}
		callback_.onTokenInvalidate();
	}

	std::string getAccessToken()
	{
		std::optional<std::string> refreshToken;

		{
			std::scoped_lock lock(mutex_);

			if (!currentTokenState_.isAuthorized()) {
				throw std::runtime_error("NotAuthorizedError(getAccessToken)");
			}

			if (currentTokenState_.isAccessTokenFresh()) {
				return currentTokenState_.access_token;
			}

			refreshToken = currentTokenState_.refresh_token;
		}

		if (clientCredentials_.client_id.empty() || clientCredentials_.client_secret.empty()) {
			throw std::runtime_error("CredentialsMissingError(getAccessToken)");
		}

		if (refreshToken.has_value()) {
			GoogleAuthResponse tokenResponse = refreshTokenState(*refreshToken);

			GoogleTokenState tokenState;
			{
				std::scoped_lock lock(mutex_);
				currentTokenState_ = currentTokenState_.withUpdatedAuthResponse(tokenResponse);
				tokenState = currentTokenState_;
			}

			callback_.onTokenStore(tokenState);

			return tokenState.access_token;
		} else {
			throw std::runtime_error("TokenRefreshError(getAccessToken)");
		}
	}

private:
	GoogleAuthResponse refreshTokenState(const std::string &refreshToken)
	{
		using nlohmann::json;
		using namespace CurlHelper;

		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl) {
			throw std::runtime_error("InitError(refreshTokenState)");
		}

		CurlVectorWriterBuffer readBuffer;

		CurlUrlSearchParams postParams(curl.get());
		postParams.append("client_id", clientCredentials_.client_id);
		postParams.append("client_secret", clientCredentials_.client_secret);
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

		return j.get<GoogleAuthResponse>();
	}

	const GoogleOAuth2ClientCredentials clientCredentials_;
	GoogleAuthManagerCallback callback_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	mutable std::mutex mutex_;
	GoogleTokenState currentTokenState_;
};

} // namespace KaitoTokyo::GoogleAuth
