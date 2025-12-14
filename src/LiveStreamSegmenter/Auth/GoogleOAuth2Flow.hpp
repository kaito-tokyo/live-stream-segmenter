/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <curl/curl.h>
#include <fmt/format.h>
#include <httplib.h>
#include <jthread.hpp>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "GoogleAuthResponse.hpp"
#include "GoogleOAuth2ClientCredentials.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

struct GoogleOAuth2FlowUserAgent {
	std::function<void(const std::string &url)> onOpenUrl;
	httplib::Server::Handler onLoginSuccess;
	httplib::Server::Handler onLoginFailure;
	std::function<void(const std::optional<GoogleAuthResponse> &tokenResponse)> onTokenReceived;
};

class GoogleOAuth2Flow {
public:
	GoogleOAuth2Flow(GoogleOAuth2ClientCredentials clientCredentials, std::string scopes,
			 std::shared_ptr<GoogleOAuth2FlowUserAgent> userAgent,
			 std::shared_ptr<const Logger::ILogger> logger)
		: clientCredentials_(std::move(clientCredentials)),
		  scopes_(std::move(scopes)),
		  userAgent_(std::move(userAgent)),
		  logger_(std::move(logger))
	{
	}

	~GoogleOAuth2Flow() = default;

	GoogleOAuth2Flow(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow &operator=(const GoogleOAuth2Flow &) = delete;
	GoogleOAuth2Flow(GoogleOAuth2Flow &&) = delete;
	GoogleOAuth2Flow &operator=(GoogleOAuth2Flow &&) = delete;

	void startOAuth2Flow()
	{
		server_ = std::make_shared<httplib::Server>();

		worker_ = std::jthread([logger = logger_, server = server_, userAgent = userAgent_,
					clientCredentials = clientCredentials_,
					scopes = scopes_](std::stop_token stoken) {
			std::stop_callback callback(stoken, [server]() {
				if (server && server->is_running()) {
					server->stop();
				}
			});

			std::optional<GoogleAuthResponse> result = std::nullopt;

			const int port = server->bind_to_any_port("127.0.0.1");
			if (port < 0)
				throw std::runtime_error("ServerBindError(startOAuth2Flow)");

			logger->info("GoogleOAuth2Flow's local server listening on port {}...", port);
			const std::string redirectUri = fmt::format("http://127.0.0.1:{}/callback", port);

			server->Get("/callback", [logger, server, userAgent, clientCredentials, redirectUri,
						  &result](const httplib::Request &req, httplib::Response &res) {
				if (req.has_param("code")) {
					try {
						const auto code = req.get_param_value("code");
						logger->info(
							"GoogleOAuth2Flow received authorization code. Exchanging for token...");

						result = exchangeCode(clientCredentials, code, redirectUri);

						logger->info("GoogleOAuth2Flow exchanged token successfully.");

						if (userAgent->onLoginSuccess) {
							userAgent->onLoginSuccess(req, res);
						}
					} catch (const std::exception &e) {
						logger->logException(e, "GoogleOAuth2Flow failed to exchange token.");

						if (userAgent->onLoginFailure) {
							userAgent->onLoginFailure(req, res);
						}
					}
				} else if (req.has_param("error")) {
					logger->error("GoogleOAuth2Flow received OAuth error: {}",
						      req.get_param_value("error"));

					if (userAgent->onLoginFailure) {
						userAgent->onLoginFailure(req, res);
					}
				}

				server->stop();
			});

			{
				const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
											       &curl_easy_cleanup);
				if (!curl)
					throw std::runtime_error("InitError(startOAuth2Flow)");

				KaitoTokyo::CurlHelper::CurlUrlSearchParams qp(curl.get());
				qp.append("client_id", clientCredentials.client_id);
				qp.append("redirect_uri", redirectUri);
				qp.append("response_type", "code");
				qp.append("scope", scopes);
				qp.append("access_type", "offline");
				qp.append("prompt", "consent");

				const std::string authUrl =
					fmt::format("https://accounts.google.com/o/oauth2/v2/auth?{}", qp.toString());
				logger->info("GoogleOAuth2Flow opening {}", authUrl);

				if (userAgent->onOpenUrl) {
					userAgent->onOpenUrl(authUrl);
				}
			}

			server->listen_after_bind();

			if (userAgent->onTokenReceived) {
				userAgent->onTokenReceived(result);
			}
		});
	}

private:
	[[nodiscard]]
	static GoogleAuthResponse exchangeCode(const GoogleOAuth2ClientCredentials &clientCredentials,
					       const std::string &code, const std::string &redirectUri)
	{
		using nlohmann::json;

		using namespace CurlHelper;

		const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl)
			throw std::runtime_error("InitError(exchangeCode)");

		CurlVectorWriterBuffer readBuffer;

		CurlUrlSearchParams params(curl.get());
		params.append("client_id", clientCredentials.client_id);
		params.append("client_secret", clientCredentials.client_secret);
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

		return j.get<GoogleAuthResponse>();
	}

	const GoogleOAuth2ClientCredentials clientCredentials_;
	const std::string scopes_;
	std::shared_ptr<GoogleOAuth2FlowUserAgent> userAgent_;
	std::shared_ptr<const Logger::ILogger> logger_;

	std::shared_ptr<httplib::Server> server_;
	std::jthread worker_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
