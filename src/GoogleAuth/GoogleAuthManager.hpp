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

class GoogleAuthManager {
public:
	GoogleAuthManager(GoogleOAuth2ClientCredentials clientCredentials,
			  std::shared_ptr<const Logger::ILogger> logger)
		: clientCredentials_(std::move(clientCredentials)),
		  logger_(std::move(logger))
	{
		if (clientCredentials_.client_id.empty() || clientCredentials_.client_secret.empty()) {
			throw std::runtime_error("CredentialsMissingError(GoogleAuthManager::GoogleAuthManager)");
		}
	}
	~GoogleAuthManager() noexcept = default;

	GoogleAuthManager(const GoogleAuthManager &) = delete;
	GoogleAuthManager &operator=(const GoogleAuthManager &) = delete;
	GoogleAuthManager(GoogleAuthManager &&) = delete;
	GoogleAuthManager &operator=(GoogleAuthManager &&) = delete;

	GoogleAuthResponse fetchFreshAuthResponse(const std::string &refreshToken) const
	{
		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl) {
			throw std::runtime_error("InitError(GoogleAuthManager::fetchFreshAuthResponse)");
		}

		CurlHelper::CurlVectorWriterBuffer readBuffer;

		CurlHelper::CurlUrlSearchParams postParams(curl.get());
		postParams.append("client_id", clientCredentials_.client_id);
		postParams.append("client_secret", clientCredentials_.client_secret);
		postParams.append("refresh_token", refreshToken);
		postParams.append("grant_type", "refresh_token");

		std::string postData = postParams.toString();

		curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
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

		nlohmann::json j = nlohmann::json::parse(readBuffer.begin(), readBuffer.end());
		if (j.contains("error")) {
			throw std::runtime_error("APIError(GoogleAuthManager::fetchFreshAuthResponse):" +
						 j["error"].dump());
		}

		return j.get<GoogleAuthResponse>();
	}

	const GoogleOAuth2ClientCredentials clientCredentials_;
	const std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::GoogleAuth
