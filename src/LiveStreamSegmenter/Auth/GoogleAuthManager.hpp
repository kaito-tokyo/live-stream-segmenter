/*
Live Stream Segmenter
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <mutex>
#include <memory>

#include <curl/curl.h>

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
			throw std::runtime_error("Client ID/Secret missing for refresh.");
		}

		logger_->info("Access token expired. Refreshing...");

		if (refreshToken.has_value()) {
			refreshTokenState(*refreshToken);
		}

		std::lock_guard<std::mutex> lock(mutex_);
		return currentTokenState_.access_token;
	}

private:
	void refreshTokenState(const std::string &refreshToken)
	{
		std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl) {
			throw std::runtime_error("InitError(refreshTokenState)");
		}

		CurlVectorWriterType readBuffer;

		CurlUrlSearchParams postData(curl.get());
		postData.append("client_id", clientId_);
		postData.append("client_secret", clientSecret_);
		postData.append("refresh_token", refreshToken);
		postData.append("grant_type", "refresh_token");

		curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.toString().c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);       // 60 second timeout
		curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
		curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 5L);      // Max 5 redirects

		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			std::string err = curl_easy_strerror(res);
			curl_easy_cleanup(curl);
			throw std::runtime_error("Network error during refresh: " + err);
		}
		curl_easy_cleanup(curl);

		// JSONパース
		json j = json::parse(readBuffer, nullptr, false);
		if (j.is_discarded()) {
			throw std::runtime_error("JSON parse error during refresh");
		}

		// エラーチェック
		if (j.contains("error")) {
			std::string err = j.value("error_description", "Unknown error");
			logger_->error("[GoogleAuthManager] Refresh failed: {}", err);

			// リフレッシュトークンが無効化されていたら強制ログアウト
			if (j.value("error", "") == "invalid_grant") {
				clear();
				throw std::runtime_error("Session expired (Revoked). Please login again.");
			}
			throw std::runtime_error("API Error: " + err);
		}

		// 成功: DTO経由で更新
		auto response = j.get<GoogleTokenResponse>();

		{
			std::lock_guard<std::mutex> lock(mutex_);
			currentTokenData_.updateFromTokenResponse(response);
			// 整合性のためロック内でデータをコピーしてから保存へ
			storage_->save(currentTokenData_);
		}

		logger_->info("[GoogleAuthManager] Token refreshed successfully.");
	}

	const std::string clientId_;
	const std::string clientSecret_;

	std::shared_ptr<const Logger::ILogger> logger_;
	std::unique_ptr<GoogleTokenStorage> storage_;

	mutable std::mutex mutex_;
	GoogleTokenState currentTokenState_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
