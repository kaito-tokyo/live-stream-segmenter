/*
Live Stream Segmenter

MIT License

Copyright (c) 2025 Kaito Udagawa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleAuthResponse.hpp"
#include "GoogleTokenState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleOAuth2Flow {
public:
    using OnSuccess = std::function<void(GoogleTokenState)>;
    using OnError = std::function<void(std::string)>;
    using OpenBrowserCallback = std::function<void(const std::string &)>;

    // コンストラクタ (実装もここに記述)
    GoogleOAuth2Flow(std::string clientId, std::string clientSecret, OpenBrowserCallback openBrowser,
                     std::shared_ptr<const Logger::ILogger> logger)
        : clientId_(std::move(clientId)),
          clientSecret_(std::move(clientSecret)),
          openBrowser_(std::move(openBrowser)),
          logger_(std::move(logger)),
          server_(std::make_unique<httplib::Server>())
    {
    }

    virtual ~GoogleOAuth2Flow() noexcept
    {
        stop();
    }

    GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
    GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
	GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

    void start(OnSuccess onSuccess, OnError onError)
    {
        if (isRunning_) {
            logger_->warn("[OAuth2] Flow already running.");
            return;
        }

        // サーバー起動スレッドを開始
        serverThread_ = std::thread([this, onSuccess, onError]() {
            this->runServer(onSuccess, onError);
        });
    }

    void stop()
    {
        if (isRunning_) {
            logger_->info("[OAuth2] Stopping local server...");
            server_->stop();
            if (serverThread_.joinable()) {
                serverThread_.join();
            }
            isRunning_ = false;
        }
    }

private:
    void runServer(OnSuccess onSuccess, OnError onError)
    {
        isRunning_ = true;

        // 1. エフェメラルポート(0)でバインド
        int port = server_->bind_to_any_port("127.0.0.1");
        if (port < 0) {
            onError("Failed to bind local server (Port 0).");
            isRunning_ = false;
            return;
        }

        logger_->info("[OAuth2] Local server listening on port {}", port);

        std::string redirectUri = "http://127.0.0.1:" + std::to_string(port) + "/callback";

        // 2. コールバックハンドラ
        server_->Get("/callback", [this, redirectUri, onSuccess, onError](const httplib::Request &req, httplib::Response &res) {
            if (req.has_param("code")) {
                std::string code = req.get_param_value("code");

                res.set_content(
                    "<html><head><title>Login Successful</title></head>"
                    "<body style='text-align:center; font-family:sans-serif; margin-top:50px;'>"
                    "<h1 style='color:#4CAF50;'>Login Successful</h1>"
                    "<p>You can close this window now.</p>"
                    "<script>window.close();</script>"
                    "</body></html>",
                    "text/html");

                // レスポンス送信後にサーバー停止 & トークン交換
                std::thread([this, code, redirectUri, onSuccess, onError]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    server_->stop();
                    exchangeCode(code, redirectUri, onSuccess, onError);
                }).detach();

            } else if (req.has_param("error")) {
                std::string err = req.get_param_value("error");
                res.set_content("Error: " + err, "text/plain");

                std::thread([this, err, onError]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    server_->stop();
                    onError("OAuth Error: " + err);
                }).detach();
            } else {
                res.status = 400;
                res.set_content("Invalid Request", "text/plain");
            }
        });

        // 3. ブラウザ起動用のURL構築
        // Note: URL構築のためだけにCurlUrlSearchParamsを使う場合、ダミーのCURL*が必要ですが、
        // 単純なエスケープなら curl_easy_escape を直接呼ぶか、ヘルパーを工夫しても良いです。
        // ここでは安全に独自のスコープでCURLハンドルを作ってヘルパーを使います。
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curlForEscape(curl_easy_init(), &curl_easy_cleanup);
        if (curlForEscape) {
            using namespace KaitoTokyo::CurlHelper;
            CurlUrlSearchParams qp(curlForEscape.get());
            
            qp.append("client_id", clientId_);
            qp.append("redirect_uri", redirectUri);
            qp.append("response_type", "code");
            qp.append("scope", "https://www.googleapis.com/auth/youtube.readonly https://www.googleapis.com/auth/userinfo.email");
            qp.append("access_type", "offline");
            qp.append("prompt", "consent");

            std::string authUrl = "https://accounts.google.com/o/oauth2/v2/auth?" + qp.toString();

            logger_->info("[OAuth2] Opening browser...");
            openBrowser_(authUrl);
        } else {
            onError("Failed to initialize CURL for URL generation");
            isRunning_ = false;
            return;
        }

        // 4. 待ち受け開始 (ブロッキング)
        server_->listen_after_bind();

        isRunning_ = false;
        logger_->info("[OAuth2] Server stopped.");
    }

    void exchangeCode(const std::string &code, const std::string &redirectUri, OnSuccess onSuccess, OnError onError)
    {
        using namespace KaitoTokyo::CurlHelper;
        using nlohmann::json;

        logger_->info("[OAuth2] Exchanging code for token...");

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
        if (!curl) {
            onError("Failed to init curl");
            return;
        }

        CurlVectorWriterType readBuffer;

        CurlUrlSearchParams params(curl.get());
        params.append("client_id", clientId_);
        params.append("client_secret", clientSecret_);
        params.append("code", code);
        params.append("grant_type", "authorization_code");
        params.append("redirect_uri", redirectUri);

        std::string postData = params.toString();

        curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);

        // SSL検証 (本番環境では必須)
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl.get());

        if (res != CURLE_OK) {
            onError(std::string("Network error during token exchange: ") + curl_easy_strerror(res));
            return;
        }

        try {
            json j = json::parse(readBuffer.begin(), readBuffer.end());
            if (j.contains("error")) {
                onError("API Error: " + j.value("error_description", "Unknown error"));
                return;
            }

            auto response = j.get<GoogleTokenResponse>();
            GoogleTokenData data;
            data.updateFromTokenResponse(response);

            if (data.refresh_token.empty()) {
                onError("Failed to obtain Refresh Token. Please unlink app permissions and try again.");
                return;
            }

            fetchUserProfile(data, onSuccess, onError);

        } catch (const std::exception &e) {
            onError(std::string("JSON parse error: ") + e.what());
        }
    }

    void fetchUserProfile(GoogleTokenData data, OnSuccess onSuccess, OnError onError)
    {
        using namespace KaitoTokyo::CurlHelper;
        using nlohmann::json;

        logger_->info("[OAuth2] Fetching user profile...");

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
        if (!curl) {
             // プロファイル取得失敗は致命的ではないので、成功としてデータを返す選択肢もあり
             onSuccess(data); 
             return;
        }

        CurlVectorWriterType readBuffer;
        std::string authHeader = "Authorization: Bearer " + data.access_token;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_URL, "https://www.googleapis.com/oauth2/v2/userinfo");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);
        
        // SSL
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl.get());
        curl_slist_free_all(headers);

        if (res == CURLE_OK) {
            try {
                json j = json::parse(readBuffer.begin(), readBuffer.end());
                if (j.contains("email")) {
                    data.email = j["email"].get<std::string>();
                }
            } catch (...) {
                // 無視して続行
            }
        }

        logger_->info("[OAuth2] Authentication completed for: {}", data.email);
        onSuccess(data);
    }

    const std::string clientId_;
    const std::string clientSecret_;
    OpenBrowserCallback openBrowser_;
    std::shared_ptr<const Logger::ILogger> logger_;

    std::unique_ptr<httplib::Server> server_;
    std::thread serverThread_;
    std::atomic<bool> isRunning_{false};
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth