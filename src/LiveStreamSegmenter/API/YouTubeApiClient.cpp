#include "YouTubeApiClient.hpp"
#include <iostream>
#include <sstream> // エラーメッセージ構築用

// 外部ライブラリ
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace KaitoTokyo::LiveStreamSegmenter::API {
// RAII wrapper for CURLU
class CurlUrlHandle {
public:
	CurlUrlHandle() : handle_(curl_url()) {
		if (!handle_) {
			throw std::runtime_error("Failed to init curl_url.");
		}
	}
	~CurlUrlHandle() {
		if (handle_) {
			curl_url_cleanup(handle_);
		}
	}
	// Non-copyable, non-movable
	CurlUrlHandle(const CurlUrlHandle&) = delete;
	CurlUrlHandle& operator=(const CurlUrlHandle&) = delete;
    CurlUrlHandle(CurlUrlHandle&&) = delete;
    CurlUrlHandle& operator=(CurlUrlHandle&&) = delete;

	void setUrl(const std::string &url) {
		CURLUcode uc = curl_url_set(handle_, CURLUPART_URL, url.c_str(), 0);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("Failed to parse URL: " + url);
		}
	}

	void appendQuery(const std::string &query) {
		CURLUcode uc = curl_url_set(handle_, CURLUPART_QUERY, query.c_str(), CURLU_APPENDQUERY);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("Failed to append query: " + query);
		}
	}

	std::unique_ptr<char, decltype(&curl_free)> c_str() const {
		char *urlStr = nullptr;
		CURLUcode uc = curl_url_get(handle_, CURLUPART_URL, &urlStr, 0);
		if (uc != CURLUE_OK || !urlStr) {
			throw std::runtime_error("Failed to get URL string from CURLU handle");
		}
		return std::unique_ptr<char, decltype(&curl_free)>(urlStr, curl_free);
	}

private:
	CURLU* handle_;
};

// --- Static Curl Helper ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	// Check for multiplication overflow
	if (size != 0 && nmemb > (std::numeric_limits<size_t>::max() / size)) {
		return 0; // Signal error (prevents buffer overflow)
	}

	size_t totalSize = size * nmemb;
	try {
		auto *str = static_cast<std::string *>(userp);
		str->append(static_cast<char *>(contents), totalSize);
	} catch (const std::bad_alloc &) {
		return 0; // Signal error
	} catch (...) {
		return 0; // Signal error
	}
	return totalSize;
}

// --- Implementation ---

void YouTubeApiClient::fetchStreamKeys(const std::string &accessToken, FetchKeysSuccess onSuccess)
{
	std::string url = "https://www.googleapis.com/youtube/v3/liveStreams?part=snippet,cdn&mine=true";

	try {
		std::string responseBody = doGet(url.c_str(), accessToken);
		auto j = json::parse(responseBody);

		if (j.contains("error")) {
			std::string msg = "Unknown API Error";
			if (j["error"].is_object() && j["error"].contains("message")) {
				msg = j["error"]["message"].get<std::string>();
			} else {
				msg = j["error"].dump();
			}
			throw std::runtime_error("API Error: " + msg);
		}

		std::vector<YouTubeStreamKey> keys;
		if (j.contains("items")) {
			int index = 0;
			for (const auto &item : j["items"]) {
				std::string currentField = "unknown";
				try {
					currentField = "cdn.ingestionType";
					if (!item["cdn"].contains("ingestionType"))
						continue;

					std::string ingestType = item["cdn"]["ingestionType"].get<std::string>();
					if (ingestType != "rtmp")
						continue;

					YouTubeStreamKey key;

					currentField = "id";
					key.id = item["id"].get<std::string>();

					currentField = "snippet.title";
					key.title = item["snippet"]["title"].get<std::string>();

					currentField = "cdn.ingestionInfo.streamName";
					key.streamName = item["cdn"]["ingestionInfo"]["streamName"].get<std::string>();

					currentField = "cdn.resolution";
					if (item["cdn"].contains("resolution")) {
						auto &res = item["cdn"]["resolution"];
						if (res.is_string())
							key.resolution = res.get<std::string>();
						else
							key.resolution = res.dump();
					}

					currentField = "cdn.frameRate";
					if (item["cdn"].contains("frameRate")) {
						auto &fps = item["cdn"]["frameRate"];
						if (fps.is_string())
							key.frameRate = fps.get<std::string>();
						else
							key.frameRate = fps.dump();
					}

					keys.push_back(key);
					index++;

				} catch (const std::exception &e) {
					std::stringstream ss;
					ss << "Parse error at item[" << index << "], field [" << currentField << "]: " << e.what()
					   << "\nDump: " << item.dump();
					throw std::runtime_error(ss.str());
				}
			}
		}

		onSuccess(keys);

	} catch (const std::exception &e) {
		throw std::runtime_error(std::string("JSON Error: ") + e.what());
	}
}

std::string YouTubeApiClient::doGet(const char *url, const std::string &accessToken) const
{
	if (!url || url[0] == '\0') {
		throw std::invalid_argument("URL must not be empty");
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to init curl.");
	}

	std::string readBuffer;
	struct curl_slist *headers = NULL;

	std::string authHeader = "Authorization: Bearer " + accessToken;
	headers = curl_slist_append(headers, authHeader.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
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
		throw std::runtime_error(std::string("Network Error: ") + curl_easy_strerror(res));
	}

	return readBuffer;
}

std::vector<json> YouTubeApiClient::performList(
    const std::string &url,
    const std::string &accessToken) const
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
            json jError = j["error"];
            if (jError.is_object() && jError.contains("message")) {
                throw std::runtime_error("API Error: " + jError["message"].get<std::string>());
            } else {
                throw std::runtime_error("API Error: " + jError.dump());
            }
        }

        if (j.contains("items")) {
            json jItems = j["items"];
            if (jItems.is_array()) {
                items.insert(items.end(), jItems.begin(), jItems.end());
            }
        }

        nextPageToken.clear();

        if (j.contains("nextPageToken")) {
            json jNextPageToken = j["nextPageToken"];
            if (jNextPageToken.is_string()) {
                nextPageToken = jNextPageToken.get<std::string>();
            }
        }
    } while (!nextPageToken.empty());
	return items;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::API
