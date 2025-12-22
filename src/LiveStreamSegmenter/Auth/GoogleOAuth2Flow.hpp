/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <coroutine>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <curl/curl.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

#include "AuthTask.hpp"
#include "GoogleAuthResponse.hpp"
#include "GoogleOAuth2ClientCredentials.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

struct GoogleOAuth2FlowUserAgent {
	std::function<void(const std::string &url)> onOpenUrl;
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

	template<typename CodeProvider, typename ContextSwitcher>
	AuthTask<std::optional<GoogleAuthResponse>> authorize(std::string redirectUri, CodeProvider &&codeProvider,
							      ContextSwitcher &&contextSwitcher)
	{
		// 1. Initialize Curl (Check)
		const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
		if (!curl)
			throw std::runtime_error("InitError(authorize)");

		// 2. Build Auth URL
		using namespace CurlHelper;
		CurlUrlSearchParams qp(curl.get());
		qp.append("client_id", clientCredentials_.client_id);
		qp.append("redirect_uri", redirectUri);
		qp.append("response_type", "code");
		qp.append("scope", scopes_);
		qp.append("access_type", "offline");
		qp.append("prompt", "consent");

		const std::string authUrl =
			fmt::format("https://accounts.google.com/o/oauth2/v2/auth?{}", qp.toString());
		logger_->info("GoogleOAuth2Flow opening {}", authUrl);

		// 3. Notify User to open Browser
		if (userAgent_->onOpenUrl) {
			userAgent_->onOpenUrl(authUrl);
		}

		// 4. Wait for Code (Delegate to User)
		logger_->info("Waiting for authorization code via user provider...");
		std::string code = co_await codeProvider(authUrl);

		if (code.empty()) {
			logger_->error("Authorization code was empty.");
			co_return std::nullopt;
		}

		co_await contextSwitcher;

		logger_->info("Received code. Exchanging for token...");

		// 5. Exchange Code for Token
		try {
			auto result = exchangeCode(clientCredentials_, code, redirectUri);
			logger_->info("GoogleOAuth2Flow exchanged token successfully.");
			co_return result;
		} catch (const std::exception &e) {
			logger_->logException(e, "GoogleOAuth2Flow failed to exchange token.");
			throw;
		}
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

		const CURLcode res = curl_easy_perform(curl.get());

		if (res != CURLE_OK) {
			throw std::runtime_error(
				fmt::format("NetworkError(exchangeCode): {}", curl_easy_strerror(res)));
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
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
