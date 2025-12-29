/*
 * Live Stream Segmenter -  YouTubeApi Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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

#include <functional>
#include <future>
#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include <CurlUrlHandler.hpp>
#include <CurlVectorWriter.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

namespace {

std::string doGet(const char *url, const char *accessToken)
{
	if (!url || url[0] == '\0') {
		throw std::invalid_argument("URLEmptyError(doGet)");
	}

	const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if (!curl) {
		throw std::runtime_error("InitError(doGet)");
	}

	CurlHelper::CurlVectorWriterBuffer readBuffer;
	struct curl_slist *headers = NULL;

	std::string authHeader = std::string("Authorization: Bearer ") + accessToken;
	headers = curl_slist_append(headers, authHeader.c_str());

	curl_easy_setopt(curl.get(), CURLOPT_URL, url);
	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_MAXREDIRS, 5L);

	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

	CURLcode res = curl_easy_perform(curl.get());

	curl_slist_free_all(headers);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("NetworkError(doGet):") + curl_easy_strerror(res));
	}

	return std::string(readBuffer.begin(), readBuffer.end());
}

std::string doPost(const char *url, const char *accessToken, const char *body)
{
	if (!url || url[0] == '\0') {
		throw std::invalid_argument("URLEmptyError(YouTubeApiClient::doPost)");
	}

	const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if (!curl) {
		throw std::runtime_error("InitError(YouTubeApiClient::doPost)");
	}

	CurlHelper::CurlVectorWriterBuffer readBuffer;
	struct curl_slist *headers = NULL;

	std::string authHeader = std::string("Authorization: Bearer ") + accessToken;
	headers = curl_slist_append(headers, authHeader.c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl.get(), CURLOPT_URL, url);
	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body);

	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);

	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

	CURLcode res = curl_easy_perform(curl.get());

	curl_slist_free_all(headers);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("NetworkError(YouTubeApiClient::doPost):") +
					 curl_easy_strerror(res));
	}

	return std::string(readBuffer.begin(), readBuffer.end());
}

std::vector<nlohmann::json> performList(const char *url, const char *accessToken, int maxIterations = 20)
{
	std::vector<nlohmann::json> items;
	std::string nextPageToken;
	do {
		CurlHelper::CurlUrlHandle urlHandle;
		urlHandle.setUrl(url);

		if (!nextPageToken.empty()) {
			urlHandle.appendQuery(("pageToken=" + nextPageToken).c_str());
		}

		auto url = urlHandle.c_str();
		std::string responseBody = doGet(url.get(), accessToken);
		nlohmann::json j = nlohmann::json::parse(responseBody);

		if (j.contains("error")) {
			throw std::runtime_error(std::string("APIError(performList):") + j["error"].dump());
		}

		nlohmann::json jItems = std::move(j["items"]);
		for (auto &x : jItems) {
			items.push_back(std::move(x));
		}

		if (!j.contains("nextPageToken"))
			break;

		nextPageToken = j["nextPageToken"].get<std::string>();
	} while (--maxIterations > 0);

	return items;
}

} // anonymous namespace

YouTubeApiClient::YouTubeApiClient(std::shared_ptr<const Logger::ILogger> logger) : logger_(std::move(logger)) {}

std::vector<YouTubeStreamKey> YouTubeApiClient::listStreamKeys(const std::string &accessToken) const
{
	std::vector<YouTubeStreamKey> streamKeys;

	const char *url = "https://www.googleapis.com/youtube/v3/liveStreams?part=snippet,cdn&mine=true";

	auto items = performList(url, accessToken.c_str());

	for (const auto &item : items) {
		YouTubeStreamKey key;
		key.id = item.value("id", "");
		key.kind = item.value("kind", "");
		key.snippet_title = item["snippet"].value("title", "");
		key.snippet_description = item["snippet"].value("description", "");
		key.snippet_channelId = item["snippet"].value("channelId", "");
		key.snippet_publishedAt = item["snippet"].value("publishedAt", "");
		key.snippet_privacyStatus = item["snippet"].value("privacyStatus", "");
		key.cdn_ingestionType = item["cdn"].value("ingestionType", "");
		key.cdn_resolution = item["cdn"].value("resolution", "");
		key.cdn_frameRate = item["cdn"].value("frameRate", "");
		key.cdn_isReusable = item["cdn"].value("isReusable", "");
		key.cdn_region = item["cdn"].value("region", "");
		key.cdn_ingestionInfo_streamName = item["cdn"]["ingestionInfo"].value("streamName", "");
		key.cdn_ingestionInfo_ingestionAddress = item["cdn"]["ingestionInfo"].value("ingestionAddress", "");
		key.cdn_ingestionInfo_backupIngestionAddress =
			item["cdn"]["ingestionInfo"].value("backupIngestionAddress", "");

		streamKeys.push_back(std::move(key));
	}

	return streamKeys;
}

void YouTubeApiClient::createLiveBroadcast(const std::string &accessToken,
					   const YouTubeLiveBroadcastSettings &settings) const
{
	// バリデーション
	if (settings.snippet_title.empty() || settings.snippet_scheduledStartTime.empty()) {
		throw std::invalid_argument("Title and ScheduledStartTime are required.");
	}

	// JSON変換 (ここで上記の to_json が呼ばれ、ネスト構造が作られます)
	nlohmann::json requestBody = settings;
	std::string bodyStr = requestBody.dump();

	// POST実行
	std::string responseBody =
		doPost("https://www.googleapis.com/youtube/v3/liveBroadcasts?part=snippet,status,contentDetails",
		       accessToken.c_str(), bodyStr.c_str());

	logger_->info("CreatedLiveBroadcast", {Logger::LogField{"responseBody", responseBody}});
}

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
