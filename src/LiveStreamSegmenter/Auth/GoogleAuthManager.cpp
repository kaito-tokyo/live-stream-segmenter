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

#include "GoogleAuthManager.hpp"

#include <stdexcept>
#include <vector>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <CurlVectorWriter.hpp>

#include "GoogleAuthResponse.hpp"

using json = nlohmann::json;

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

std::string GoogleAuthManager::getAccessToken()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!currentTokenData_.isAuthorized()) {
			throw std::runtime_error("Not logged in.");
		}
		// トークンが有効ならそのまま返す
		if (currentTokenData_.isAccessTokenFresh()) {
			return currentTokenData_.access_token;
		}
		// リフレッシュに必要な情報があるか確認
		if (clientId_.empty() || clientSecret_.empty()) {
			throw std::runtime_error("Client ID/Secret missing for refresh.");
		}
	}

	logger_->info("[GoogleAuthManager] Access token expired. Refreshing...");

	// 同期リフレッシュ実行
	performRefresh();

	// 更新後のトークンを返す
	std::lock_guard<std::mutex> lock(mutex_);
	return currentTokenData_.access_token;
}

void GoogleAuthManager::performRefresh()
{
	// 通信に必要な情報をコピー (通信中はロックを保持しない)
	std::string rtoken;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		rtoken = currentTokenData_.refresh_token;
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to init curl");
	}

	// URL Encoding helper
	auto urlEncode = [&](const std::string &s) -> std::string {
		char *output = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.length()));
		if (output) {
			std::string result(output);
			curl_free(output);
			return result;
		}
		return "";
	};

	std::string readBuffer;
	std::string postDataStr = "client_id=" + urlEncode(clientId_) + "&client_secret=" + urlEncode(clientSecret_) +
				  "&refresh_token=" + urlEncode(rtoken) + "&grant_type=refresh_token";

	curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postDataStr.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

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

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth