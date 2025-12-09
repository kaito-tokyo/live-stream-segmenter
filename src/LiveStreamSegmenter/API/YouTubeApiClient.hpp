#pragma once

#include <string>
#include <vector>
#include <functional>
#include "YouTubeTypes.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::API {

/**
 * @brief YouTube Data API Client (Pure C++)
 * * Qt, OBS API への依存なし。
 * * 通信は同期的(ブロッキング)に行われます。呼び出し側でスレッド管理が必要です。
 */
class YouTubeApiClient {
public:
    YouTubeApiClient() = default;
    ~YouTubeApiClient() = default;

    // コールバック型の定義
    using FetchKeysSuccess = std::function<void(const std::vector<YouTubeStreamKey>&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    /**
     * @brief ストリームキー一覧を取得する (GET liveStreams)
     * @param accessToken 有効なOAuth2アクセストークン
     */
    void fetchStreamKeys(const std::string& accessToken,
                         FetchKeysSuccess onSuccess,
                         ErrorCallback onError);

private:
    // 内部実装: HTTPリクエストを実行する汎用メソッド
    void performGetRequest(const std::string& url,
                           const std::string& accessToken,
                           std::function<void(const std::string& responseBody)> onSuccess,
                           ErrorCallback onError);
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
