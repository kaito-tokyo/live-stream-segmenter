/*
 * MIT License
 * 
 * Copyright (c) 2025 Kaito Udagawa
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

#include "YouTubeApiClient.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace KaitoTokyo::LiveStreamSegmenter::API {

namespace {

inline size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	if (size != 0 && nmemb > (std::numeric_limits<size_t>::max() / size)) {
		return 0; // Signal error
	}

	size_t totalSize = size * nmemb;
	try {
		auto *vec = static_cast<std::vector<char> *>(userp);
		vec->insert(vec->end(), static_cast<char *>(contents), static_cast<char *>(contents) + totalSize);
	} catch (...) {
		return 0; // Signal error
	}
	return totalSize;
}

inline std::string doGet(const char *url, const std::string &accessToken)
{
	if (!url || url[0] == '\0') {
		throw std::invalid_argument("URLEmptyError(doGet)");
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("InitError(doGet)");
	}

	std::vector<char> readBuffer;
	struct curl_slist *headers = NULL;

	std::string authHeader = "Authorization: Bearer " + accessToken;
	headers = curl_slist_append(headers, authHeader.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
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

inline std::vector<json> performList(const char *url, const std::string &accessToken, int maxIterations = 20)
{
	std::vector<json> items;
	std::string nextPageToken;
	do {
		CurlUrlHandle urlHandle;
		urlHandle.setUrl(url);

		if (!nextPageToken.empty()) {
			urlHandle.appendQuery("pageToken=" + nextPageToken);
		}

		std::string responseBody = doGet(urlHandle.c_str().get(), accessToken);
		json j = json::parse(responseBody);

		if (j.contains("error")) {
			throw std::runtime_error("APIError(performList):" + j["error"].dump());
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

} // namespace

std::vector<YouTubeStreamKey> YouTubeApiClient::listStreamKeys()
{
	const char *url = "https://www.googleapis.com/youtube/v3/liveStreams?part=snippet,cdn&mine=true";
	std::string accessToken = accessTokenFunc_().get();
	std::vector<json> items = performList(url, accessToken);
	std::vector<YouTubeStreamKey> streamKeys;
	const json emptyObject = json::object();
	for (const json &item : items) {
		const json &snippet = item.value("snippet", emptyObject);
		const json &cdn = item.value("cdn", emptyObject);
		const json &info = cdn.value("ingestionInfo", emptyObject);

		YouTubeStreamKey key;
		key.id = item.value("id", "");
		key.kind = item.value("kind", "");
		key.snippet_title = snippet.value("title", "");
		key.snippet_description = snippet.value("description", "");
		key.snippet_channelId = snippet.value("channelId", "");
		key.snippet_publishedAt = snippet.value("publishedAt", "");
		key.snippet_privacyStatus = snippet.value("privacyStatus", "");
		key.cdn_ingestionType = cdn.value("ingestionType", "");
		key.cdn_resolution = cdn.value("resolution", "");
		key.cdn_frameRate = cdn.value("frameRate", "");
		key.cdn_isReusable = cdn.value("isReusable", "");
		key.cdn_region = cdn.value("region", "");
		key.cdn_ingestionInfo_streamName = info.value("streamName", "");
		key.cdn_ingestionInfo_ingestionAddress = info.value("ingestionAddress", "");
		key.cdn_ingestionInfo_backupIngestionAddress = info.value("backupIngestionAddress", "");
		streamKeys.push_back(std::move(key));
	}
	return streamKeys;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::API
