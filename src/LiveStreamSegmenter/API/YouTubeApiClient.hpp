/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <CurlUrlHandler.hpp>

#include "YouTubeTypes.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::API {

class YouTubeApiClient {
public:
	using AccessTokenFunc = std::function<std::future<std::string>()>;

	YouTubeApiClient(AccessTokenFunc accessTokenFunc) : accessTokenFunc_(std::move(accessTokenFunc)) {}
	~YouTubeApiClient() = default;

	std::vector<YouTubeStreamKey> listStreamKeys();

private:
	std::string doGet(const char *url, const std::string &accessToken)
	{
		using namespace KaitoTokyo::CurlHelper;

		if (!url || url[0] == '\0') {
			throw std::invalid_argument("URLEmptyError(doGet)");
		}

		CURL *curl = curl_easy_init();
		if (!curl) {
			throw std::runtime_error("InitError(doGet)");
		}

		CurlVectorWriterBuffer readBuffer;
		struct curl_slist *headers = NULL;

		std::string authHeader = "Authorization: Bearer " + accessToken;
		headers = curl_slist_append(headers, authHeader.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlVectorWriter);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);       // 60 second timeout
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);      // Max 5 redirects

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

		CURLcode res = curl_easy_perform(curl);

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK) {
			throw std::runtime_error(std::string("NetworkError(doGet):") + curl_easy_strerror(res));
		}

		return std::string(readBuffer.begin(), readBuffer.end());
	}

	std::vector<nlohmann::json> performList(const char *url, const std::string &accessToken, int maxIterations = 20)
	{
		using namespace KaitoTokyo::CurlHelper;
		using namespace nlohmann;

		std::vector<json> items;
		std::string nextPageToken;
		do {
			CurlUrlHandle urlHandle;
			urlHandle.setUrl(url);

			if (!nextPageToken.empty()) {
				urlHandle.appendQuery(("pageToken=" + nextPageToken).c_str());
			}

			auto url = urlHandle.c_str();
			std::string responseBody = doGet(url.get(), accessToken);
			json j = json::parse(responseBody);

			if (j.contains("error")) {
				throw std::runtime_error(fmt::format("APIError(performList):{}", j["error"].dump()));
			}

			json jItems = std::move(j["items"]);
			for (auto &x : jItems) {
				items.push_back(std::move(x));
			}

			if (!j.contains("nextPageToken"))
				break;

			nextPageToken = j["nextPageToken"].get<std::string>();
		} while (--maxIterations > 0);

		return items;
	}

	AccessTokenFunc accessTokenFunc_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
