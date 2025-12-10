#include "YouTubeApiClient.hpp"
#include <iostream>
#include <sstream> // エラーメッセージ構築用

// 外部ライブラリ
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace KaitoTokyo::LiveStreamSegmenter::API {

class CurlUrlHandle {
public:
	CurlUrlHandle() : handle_(curl_url())
	{
		if (!handle_) {
			throw std::runtime_error("InitError(CurlUrlHandle)");
		}
	}
	~CurlUrlHandle()
	{
		if (handle_) {
			curl_url_cleanup(handle_);
		}
	}

	CurlUrlHandle(const CurlUrlHandle &) = delete;
	CurlUrlHandle &operator=(const CurlUrlHandle &) = delete;
	CurlUrlHandle(CurlUrlHandle &&) = delete;
	CurlUrlHandle &operator=(CurlUrlHandle &&) = delete;

	void setUrl(const char *url)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_URL, url, 0);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("URLParseError(CurlUrlHandle):" + std::string(url));
		}
	}

	void appendQuery(const std::string &query)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_QUERY, query.c_str(), CURLU_APPENDQUERY);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("QueryAppendError(CurlUrlHandle):" + query);
		}
	}

	std::unique_ptr<char, decltype(&curl_free)> c_str() const
	{
		char *urlStr = nullptr;
		CURLUcode uc = curl_url_get(handle_, CURLUPART_URL, &urlStr, 0);
		if (uc != CURLUE_OK || !urlStr) {
			throw std::runtime_error("GetUrlError(CurlUrlHandle)");
		}
		return std::unique_ptr<char, decltype(&curl_free)>(urlStr, curl_free);
	}

private:
	CURLU *const handle_;
};

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
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       // 10 second timeout
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);      // Max 5 redirects

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	double contentLength = 0;
	curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
	if (contentLength > 0 && contentLength < static_cast<double>(std::numeric_limits<size_t>::max())) {
		readBuffer.reserve(static_cast<size_t>(contentLength));
	}

	CURLcode res = curl_easy_perform(curl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		throw std::runtime_error(std::string("NetworkError(doGet):") + curl_easy_strerror(res));
	}

	return std::string(readBuffer.begin(), readBuffer.end());
}

inline std::vector<json> performList(const char *url, const std::string &accessToken, int maxPages = 20)
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
	} while (--maxPages > 0);
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
