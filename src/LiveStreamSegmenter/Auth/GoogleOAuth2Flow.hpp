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

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

// Vendor Libraries
#include <jthread.hpp>        // josuttis/jthread (C++20 backport)
#include <httplib.h>          // yhirose/cpp-httplib
#include <nlohmann/json.hpp>  // nlohmann/json
#include <curl/curl.h>        // libcurl

// Project Helpers
#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleTokenState.hpp"    // 旧 GoogleTokenData.hpp
#include "GoogleAuthResponse.hpp"  // 旧 GoogleTokenResponse.hpp

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

/**
 * @brief Google OAuth 2.0 認証フローを管理するクラス (Header-only)
 * ローカルサーバーを起動し、ブラウザ経由で認可コードを受け取り、トークンに交換します。
 */
class GoogleOAuth2Flow {
public:
    /// ブラウザを開くためのコールバック型
    using OpenBrowserCallback = std::function<void(const std::string &)>;

    /**
     * @brief コンストラクタ
     */
    GoogleOAuth2Flow(std::string clientId, std::string clientSecret, OpenBrowserCallback openBrowser,
                     std::shared_ptr<const Logger::ILogger> logger)
        : clientId_(std::move(clientId)),
          clientSecret_(std::move(clientSecret)),
          openBrowser_(std::move(openBrowser)),
          logger_(std::move(logger)),
          server_(std::make_unique<httplib::Server>())
    {
    }

    /**
     * @brief デストラクタ
     * std::jthread を使用しているため、オブジェクト破棄時に自動的に
     * 停止要求(request_stop)が送られ、サーバーが停止し、スレッドが join されます。
     */
    ~GoogleOAuth2Flow() = default;

    // コピー・ムーブ禁止
    GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
    GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
    GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
    GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

    /**
     * @brief 認証フローを開始します (非同期)
     * @return 認証完了時に GoogleTokenState を返す std::future
     */
	[[nodiscard]]
    std::future<GoogleTokenState> start()
    {
        auto promise = std::make_shared<std::promise<GoogleTokenState>>();
        std::future<GoogleTokenState> future = promise->get_future();

        serverThread_ = std::jthread([this, promise](std::stop_token stoken) {
            try {
                this->runServer(stoken, promise);
            } catch (...) {
                try { promise->set_exception(std::current_exception()); } catch (...) {}
            }
        });

        return future;
    }

    /**
     * @brief フローを手動で中止します
     */
    void stop()
    {
        serverThread_.request_stop();
    }

private:
    /**
     * @brief サーバー実行メインループ
     */
void runServer(std::stop_token stoken, std::shared_ptr<std::promise<GoogleTokenState>> promise)
    {
        // 停止要求時のコールバック
        std::stop_callback callback(stoken, [this]() {
            logger_->info("[OAuth2] Stop requested. Stopping http server...");
            if (server_) server_->stop();
        });

        int port = server_->bind_to_any_port("127.0.0.1");
        if (port < 0) throw std::runtime_error("Failed to bind local server.");

        logger_->info("[OAuth2] Local server listening on port {}", port);
        std::string redirectUri = "http://127.0.0.1:" + std::to_string(port) + "/callback";

        // 結果を受け渡すためのローカル変数
        std::string capturedCode;
        std::string capturedError;

        server_->Get("/callback", [&](const httplib::Request &req, httplib::Response &res) {
            if (req.has_param("code")) {
                // 1. コードを確保
                capturedCode = req.get_param_value("code");

                res.set_content("<html><script>window.close();</script><body><h1>Login Successful</h1></body></html>", "text/html");

                // 2. レスポンス送信後にサーバーを止めるための「時限停止タスク」をセット
                // handler自体はすぐreturnしないとブラウザが待機状態になるため、別スレッドでstopを呼ぶ
                if (stopperThread_.joinable()) stopperThread_.join();
                
                stopperThread_ = std::jthread([this]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    if (server_) server_->stop();
                });

            } else if (req.has_param("error")) {
                capturedError = req.get_param_value("error");
                res.set_content("Error: " + capturedError, "text/plain");

                if (stopperThread_.joinable()) stopperThread_.join();
                stopperThread_ = std::jthread([this]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    if (server_) server_->stop();
                });
            }
        });

        // URL生成とブラウザ起動 (省略なし)
        {
             std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
             if (!curl) throw std::runtime_error("Failed to init CURL");
             
             using namespace KaitoTokyo::CurlHelper;
             CurlUrlSearchParams qp(curl.get());
             qp.append("client_id", clientId_);
             qp.append("redirect_uri", redirectUri);
             qp.append("response_type", "code");
             qp.append("scope", "https://www.googleapis.com/auth/youtube.readonly https://www.googleapis.com/auth/userinfo.email");
             qp.append("access_type", "offline");
             qp.append("prompt", "consent");

             openBrowser_("https://accounts.google.com/o/oauth2/v2/auth?" + qp.toString());
        }

        // 3. ブロック開始 (stop()が呼ばれるまでここで止まる)
        server_->listen_after_bind();

        // 4. ループを抜けた後、ここから先はメインスレッド(runServer)の続きとして安全に実行できる
        
        // ストッパースレッドの終了を確実にする
        if (stopperThread_.joinable()) {
            stopperThread_.join();
        }

        // 停止理由が「コード取得成功」だった場合のみトークン交換へ
        if (!capturedCode.empty()) {
            exchangeCode(capturedCode, redirectUri, promise);
        } else if (!capturedError.empty()) {
            throw std::runtime_error("OAuth Error: " + capturedError);
        }
        // それ以外（デストラクタによる強制停止など）は何もしない
    }

    /**
     * @brief 認可コードをアクセストークン/リフレッシュトークンに交換する
     */
    void exchangeCode(const std::string &code, const std::string &redirectUri, std::shared_ptr<std::promise<GoogleTokenState>> promise)
    {
        using namespace KaitoTokyo::CurlHelper;
        using nlohmann::json;

        logger_->info("[OAuth2] Exchanging code for token...");

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
        if (!curl) {
            throw std::runtime_error("Failed to init curl");
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
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl.get());

        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("Network error during token exchange: ") + curl_easy_strerror(res));
        }

        json j = json::parse(readBuffer.begin(), readBuffer.end());
        if (j.contains("error")) {
            throw std::runtime_error("API Error: " + j.value("error_description", "Unknown error"));
        }

        // レスポンスをパース (GoogleAuthResponse)
        auto response = j.get<GoogleAuthResponse>();
        GoogleTokenState state;
        state.updateFromTokenResponse(response);

        if (state.refresh_token.empty()) {
            throw std::runtime_error("Failed to obtain Refresh Token. Please unlink app permissions and try again.");
        }

        // 次のステップ: プロファイル取得 (同期的に実行)
        fetchUserProfile(state, promise);
    }

    /**
     * @brief アクセストークンを使ってユーザー情報(Email)を取得する
     */
    void fetchUserProfile(GoogleTokenState state, std::shared_ptr<std::promise<GoogleTokenState>> promise)
    {
        using namespace KaitoTokyo::CurlHelper;
        using nlohmann::json;

        logger_->info("[OAuth2] Fetching user profile...");

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
        if (!curl) {
             // プロファイル取得失敗は致命的エラーにしない
             promise->set_value(state); 
             return;
        }

        CurlVectorWriterType readBuffer;
        
        std::string authHeader = "Authorization: Bearer " + state.access_token;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_URL, "https://www.googleapis.com/oauth2/v2/userinfo");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl.get());
        curl_slist_free_all(headers);

        if (res == CURLE_OK) {
            try {
                json j = json::parse(readBuffer.begin(), readBuffer.end());
                if (j.contains("email")) {
                    state.email = j["email"].get<std::string>();
                }
            } catch (...) {
                // パースエラーは無視
            }
        }

        logger_->info("[OAuth2] Authentication completed for: {}", state.email);
        
        // 完了通知 (GoogleTokenState)
        promise->set_value(state);
    }
	
	const std::string clientId_;
    const std::string clientSecret_;
    OpenBrowserCallback openBrowser_;
    std::shared_ptr<const Logger::ILogger> logger_;

    std::unique_ptr<httplib::Server> server_;
    
    // メインのワーカースレッド
    std::jthread serverThread_;
    
    // サーバー停止トリガー用の極小スレッド (メンバにしてRAII管理)
    std::jthread stopperThread_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
