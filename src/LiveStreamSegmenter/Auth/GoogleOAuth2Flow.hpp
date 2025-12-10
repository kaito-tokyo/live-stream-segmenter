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

#include <curl/curl.h>
#include <httplib.h>
#include <jthread.hpp>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleAuthResponse.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleOAuth2Flow {
public:
	using OpenBrowserCallback = std::function<void(const std::string &)>;

	GoogleOAuth2Flow(std::string clientId, std::string clientSecret, std::string scopes,
			 OpenBrowserCallback openBrowser, std::shared_ptr<const Logger::ILogger> logger)
		: clientId_(std::move(clientId)),
		  clientSecret_(std::move(clientSecret)),
		  scopes_(std::move(scopes)),
		  openBrowser_(std::move(openBrowser)),
		  logger_(std::move(logger)),
		  server_(std::make_unique<httplib::Server>())
	{
	}

	~GoogleOAuth2Flow() = default;

	GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
	GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

	[[nodiscard]]
	std::future<GoogleTokenResponse> start()
	{
		auto promise = std::make_shared<std::promise<GoogleTokenResponse>>();
		std::future<GoogleTokenResponse> future = promise->get_future();

		serverThread_ = std::jthread([this, promise](std::stop_token stoken) {
			try {
				this->runServer(stoken, promise);
			} catch (...) {
				try {
					promise->set_exception(std::current_exception());
				} catch (...) {
				}
			}
		});

		return future;
	}

	void stop()
	{
		serverThread_.request_stop();
		if (server_)
			server_->stop();
	}

private:
	/**
     * @brief サーバー実行メインループ
     */
	void runServer(std::stop_token stoken, std::shared_ptr<std::promise<GoogleTokenResponse>> promise)
	{
		// 停止要求時のコールバック
		std::stop_callback callback(stoken, [this]() {
			logger_->info("[OAuth2] Stop requested. Stopping http server...");
			if (server_)
				server_->stop();
		});

		int port = server_->bind_to_any_port("127.0.0.1");
		if (port < 0)
			throw std::runtime_error("Failed to bind local server.");

		logger_->info("[OAuth2] Local server listening on port {}", port);
		std::string redirectUri = "http://127.0.0.1:" + std::to_string(port) + "/callback";

		// 結果を受け渡すためのローカル変数
		std::string capturedCode;
		std::string capturedError;

		server_->Get("/callback", [&](const httplib::Request &req, httplib::Response &res) {
			if (req.has_param("code")) {
				capturedCode = req.get_param_value("code");
				res.set_content(
					"<html><script>window.close();</script><body><h1>Login Successful</h1></body></html>",
					"text/html");

				// サーバー停止のトリガー
				if (stopperThread_.joinable())
					stopperThread_.join();
				stopperThread_ = std::jthread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					if (server_)
						server_->stop();
				});

			} else if (req.has_param("error")) {
				capturedError = req.get_param_value("error");
				res.set_content("Error: " + capturedError, "text/plain");

				if (stopperThread_.joinable())
					stopperThread_.join();
				stopperThread_ = std::jthread([this]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					if (server_)
						server_->stop();
				});
			}
		});

		// URL生成とブラウザ起動
		{
			std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
			if (!curl)
				throw std::runtime_error("Failed to init CURL");

			using namespace KaitoTokyo::CurlHelper;
			CurlUrlSearchParams qp(curl.get());
			qp.append("client_id", clientId_);
			qp.append("redirect_uri", redirectUri);
			qp.append("response_type", "code");
			qp.append("scope", scopes_); // コンストラクタで受け取ったスコープを使用
			qp.append("access_type", "offline");
			qp.append("prompt", "consent"); // リフレッシュトークンを確実に取得するため

			openBrowser_("https://accounts.google.com/o/oauth2/v2/auth?" + qp.toString());
		}

		// サーバー待機 (stop()が呼ばれるか、コールバックでstopされるまでブロック)
		server_->listen_after_bind();

		// ストッパースレッドの終了を待つ
		if (stopperThread_.joinable()) {
			stopperThread_.join();
		}

		if (!capturedCode.empty()) {
			// コード交換を実行し、結果をPromiseにセット
			auto response = exchangeCode(capturedCode, redirectUri);
			promise->set_value(response);
		} else if (!capturedError.empty()) {
			throw std::runtime_error("OAuth Error: " + capturedError);
		}
	}

	/**
     * @brief 認可コードをトークンに交換する
     */
	GoogleTokenResponse exchangeCode(const std::string &code, const std::string &redirectUri)
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
			throw std::runtime_error(std::string("Network error during token exchange: ") +
						 curl_easy_strerror(res));
		}

		json j = json::parse(readBuffer.begin(), readBuffer.end());
		if (j.contains("error")) {
			throw std::runtime_error("API Error: " + j.value("error_description", "Unknown error"));
		}

		// レスポンスをパースしてそのまま返す
		auto response = j.get<GoogleTokenResponse>();

		logger_->info("[OAuth2] Token exchange successful.");
		return response;
	}

	const std::string clientId_;
	const std::string clientSecret_;
	const std::string scopes_;
	OpenBrowserCallback openBrowser_;
	std::shared_ptr<const Logger::ILogger> logger_;

	std::unique_ptr<httplib::Server> server_;

	std::jthread serverThread_;
	std::jthread stopperThread_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
