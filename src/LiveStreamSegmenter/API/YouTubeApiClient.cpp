#include "YouTubeApiClient.hpp"
#include <iostream>
#include <sstream> // エラーメッセージ構築用

// 外部ライブラリ
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace KaitoTokyo::LiveStreamSegmenter::API {

// --- Static Curl Helper ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    auto *str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), realsize);
    return realsize;
}

// --- Implementation ---

void YouTubeApiClient::fetchStreamKeys(const std::string& accessToken,
                                       FetchKeysSuccess onSuccess,
                                       ErrorCallback onError)
{
    // API URL
    std::string url = "https://www.googleapis.com/youtube/v3/liveStreams?part=snippet,cdn&mine=true";

    performGetRequest(url, accessToken,
        [onSuccess, onError](const std::string& responseBody) {
            try {
                // 【デバッグ用】 コンソールに生データを出力
                // (OBSをコマンドラインから起動している場合に見えます)
                std::cout << "[YouTubeApiClient] Raw JSON: " << responseBody << std::endl;

                auto j = json::parse(responseBody);

                if (j.contains("error")) {
                    // エラーオブジェクトが文字列かオブジェクトか確認して取得
                    std::string msg = "Unknown API Error";
                    if (j["error"].is_object() && j["error"].contains("message")) {
                         msg = j["error"]["message"].get<std::string>();
                    } else {
                         msg = j["error"].dump();
                    }
                    onError("API Error: " + msg);
                    return;
                }

                std::vector<YouTubeStreamKey> keys;
                if (j.contains("items")) {
                    int index = 0;
                    for (const auto& item : j["items"]) {// トラブルシュート: どの項目の解析中か記録
                        std::string currentField = "unknown";
                        try {
                            // 1. Ingestion Info Check
                            // 【修正】ingestionType は cdn 直下にあります
                            currentField = "cdn.ingestionType";
                            if (!item["cdn"].contains("ingestionType")) continue;
                            
                            std::string ingestType = item["cdn"]["ingestionType"].get<std::string>();
                            if (ingestType != "rtmp") continue;

                            YouTubeStreamKey key;
                            
                            // 2. ID
                            currentField = "id";
                            key.id = item["id"].get<std::string>();
                            
                            // 3. Title
                            currentField = "snippet.title";
                            key.title = item["snippet"]["title"].get<std::string>();
                            
                            // 4. StreamName
                            currentField = "cdn.ingestionInfo.streamName";
                            key.streamName = item["cdn"]["ingestionInfo"]["streamName"].get<std::string>();
                            
                            // 5. Resolution (Optional)
                            currentField = "cdn.resolution";
                            if (item["cdn"].contains("resolution")) {
                                auto& res = item["cdn"]["resolution"];
                                if (res.is_string()) key.resolution = res.get<std::string>();
                                else key.resolution = res.dump();
                            }
                            
                            // 6. FrameRate (Optional)
                            currentField = "cdn.frameRate";
                            if (item["cdn"].contains("frameRate")) {
                                auto& fps = item["cdn"]["frameRate"];
                                if (fps.is_string()) key.frameRate = fps.get<std::string>();
                                else key.frameRate = fps.dump();
                            }

                            keys.push_back(key);
                            index++;

                        } catch (const std::exception& e) {
                            // パース中のアイテム情報をエラーに含める
                            std::stringstream ss;
                            ss << "Parse error at item[" << index << "], field [" << currentField << "]: " 
                               << e.what() << "\nDump: " << item.dump();
                            onError(ss.str());
                            return;
                        }
                    }
                }
                
                onSuccess(keys);

            } catch (const std::exception& e) {
                // 全体的なJSONパースエラーの場合
                // 生データの一部をエラーメッセージに含めて原因を特定しやすくする
                std::string debugInfo = responseBody;
                if (debugInfo.length() > 500) debugInfo = debugInfo.substr(0, 500) + "...";
                
                onError(std::string("JSON Error: ") + e.what() + "\nResponse: " + debugInfo);
            }
        },
        onError
    );
}

// ... (performGetRequest は変更なし) ...
void YouTubeApiClient::performGetRequest(const std::string& url,
                                         const std::string& accessToken,
                                         std::function<void(const std::string&)> onSuccess,
                                         ErrorCallback onError)
{
    // (以前のコードのまま)
    CURL *curl = curl_easy_init();
    if (!curl) {
        onError("Failed to initialize libcurl.");
        return;
    }

    std::string readBuffer;
    struct curl_slist *headers = NULL;
    
    std::string authHeader = "Authorization: Bearer " + accessToken;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    // SSL検証 (開発用)
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        onError(std::string("Network Error: ") + curl_easy_strerror(res));
    } else {
        onSuccess(readBuffer);
    }
}

} // namespace
