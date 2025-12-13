/*
 * Live Stream Segmenter
 * Copyright (c) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <fmt/format.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleAuthResponse.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

struct GoogleOAuth2FlowUserAgent {
	std::function<void(const std::string &url)> openBrowser;
	httplib::Server::Handler onLoginSuccess;
	httplib::Server::Handler onLoginFailure;
};

class GoogleOAuth2Flow {
public:
	GoogleOAuth2Flow(std::string clientId, std::string clientSecret, std::string scopes,
			 GoogleOAuth2FlowUserAgent userAgent, std::shared_ptr<const Logger::ILogger> logger)
		: clientId_(std::move(clientId)),
		  clientSecret_(std::move(clientSecret)),
		  scopes_(std::move(scopes)),
		  userAgent_(std::move(userAgent)),
		  logger_(std::move(logger))
	{
	}

	~GoogleOAuth2Flow() { stopOAuth2Flow(); }

	GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
	GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

	[[nodiscard]]
	std::future<std::optional<GoogleTokenResponse>> startOAuth2Flow()
	{
		using namespace KaitoTokyo::CurlHelper;

		server_ = std::make_shared<httplib::Server>();

		return std::async(
			std::launch::async,
			[logger = logger_, server = server_, userAgent = userAgent_, clientId = clientId_,
			 clientSecret = clientSecret_, scopes = scopes_]() -> std::optional<GoogleTokenResponse> {
				std::optional<GoogleTokenResponse> result = std::nullopt;

				const int port = server->bind_to_any_port("127.0.0.1");
				if (port < 0)
					throw std::runtime_error("ServerBindError(startOAuth2Flow)");

				logger->info("GoogleOAuth2Flow's local server listening on port {}...", port);
				const std::string redirectUri = fmt::format("http://127.0.0.1:{}/callback", port);

				server->Get("/callback", [logger, server, userAgent, clientId, clientSecret,
							  redirectUri, &result](const httplib::Request &req,
										httplib::Response &res) {
					if (req.has_param("code")) {
						try {
							const auto code = req.get_param_value("code");
							logger->info(
								"GoogleOAuth2Flow received authorization code. Exchanging for token...");

							result =
								exchangeCode(clientId, clientSecret, code, redirectUri);

							logger->info("GoogleOAuth2Flow exchanged token successfully.");

							userAgent.onLoginSuccess(req, res);
						} catch (const std::exception &e) {
							logger->logException(
								e, "GoogleOAuth2Flow failed to exchange token.");

							userAgent.onLoginFailure(req, res);
						}
					} else if (req.has_param("error")) {
						logger->error("GoogleOAuth2Flow received OAuth error: {}",
							      req.get_param_value("error"));

						userAgent.onLoginFailure(req, res);
					}

					server->stop();
				});

				{
					const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
						curl_easy_init(), &curl_easy_cleanup);
					if (!curl)
						throw std::runtime_error("InitError(startOAuth2Flow)");

					KaitoTokyo::CurlHelper::CurlUrlSearchParams qp(curl.get());
					qp.append("client_id", clientId);
					qp.append("redirect_uri", redirectUri);
					qp.append("response_type", "code");
					qp.append("scope", scopes);
					qp.append("access_type", "offline");
					qp.append("prompt", "consent");

					const std::string authUrl = fmt::format(
						"https://accounts.google.com/o/oauth2/v2/auth?{}", qp.toString());
					logger->info("GoogleOAuth2Flow opening {}", authUrl);

					userAgent.openBrowser(authUrl);
				}

				server->listen_after_bind();

				return result;
			});
	}

	void stopOAuth2Flow()
	{
		if (server_ && server_->is_running()) {
			server_->stop();
		}
	}

private:
	static GoogleTokenResponse exchangeCode(const std::string &clientId, const std::string &clientSecret,
						const std::string &code, const std::string &redirectUri)
	{
		using namespace KaitoTokyo::CurlHelper;
		using nlohmann::json;

		const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl)
			throw std::runtime_error("InitError(exchangeCode)");

		CurlVectorWriterBuffer readBuffer;

		CurlUrlSearchParams params(curl.get());
		params.append("client_id", clientId);
		params.append("client_secret", clientSecret);
		params.append("code", code);
		params.append("grant_type", "authorization_code");
		params.append("redirect_uri", redirectUri);

		const std::string postData = params.toString();

		curl_easy_setopt(curl.get(), CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, CurlVectorWriter);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 60L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);

		const CURLcode res = curl_easy_perform(curl.get());

		if (res != CURLE_OK) {
			throw std::runtime_error(fmt::format("NetworkError(exchangeCode):{}", curl_easy_strerror(res)));
		}

		const json j = json::parse(readBuffer.begin(), readBuffer.end());
		if (j.contains("error")) {
			throw std::runtime_error(fmt::format("APIError(exchangeCode): {}", j.dump()));
		}

		return j.get<GoogleTokenResponse>();
	}

	const std::string clientId_;
	const std::string clientSecret_;
	const std::string scopes_;

	GoogleOAuth2FlowUserAgent userAgent_;

	std::shared_ptr<const Logger::ILogger> logger_;
	std::shared_ptr<httplib::Server> server_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
