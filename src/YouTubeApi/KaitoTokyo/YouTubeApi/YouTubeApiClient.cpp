/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo YouTubeApi Library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "YouTubeApiClient.hpp"

#include <cassert>
#include <cctype>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

#include <KaitoTokyo/CurlHelper/CurlReadCallback.hpp>
#include <KaitoTokyo/CurlHelper/CurlSlistHandle.hpp>
#include <KaitoTokyo/CurlHelper/CurlUrlHandle.hpp>
#include <KaitoTokyo/CurlHelper/CurlUrlSearchParams.hpp>
#include <KaitoTokyo/CurlHelper/CurlWriteCallback.hpp>
#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>

namespace KaitoTokyo::YouTubeApi {

namespace {

int progressCallback(void *clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
{
	auto *stoken = static_cast<Jthread::stop_token *>(clientp);
	if (stoken && stoken->stop_requested()) {
		return 1;
	} else {
		return 0;
	}
}

std::vector<char> doGet(std::shared_ptr<const Logger::ILogger> logger, std::shared_ptr<CurlHelper::CurlHandle> curl,
			Jthread::stop_token stoken, const std::string &url, curl_slist *headers = nullptr)
{
	if (!logger) {
		logger = Logger::NullLogger::instance();
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::doGet)");
	}
	if (url.empty()) {
		logger->error("UrlIsEmptyError");
		throw std::invalid_argument("UrlIsEmptyError(YouTubeApiClient::doGet)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl->getRaw(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl->getRaw(), CURLOPT_FOLLOWLOCATION, 2L);

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, progressCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doGet)");
	}

	return readBuffer;
}

std::vector<char> doPost(std::shared_ptr<const Logger::ILogger> logger, std::shared_ptr<CurlHelper::CurlHandle> curl,
			 Jthread::stop_token stoken, const std::string &url, curl_slist *headers = nullptr)
{
	if (!logger) {
		logger = Logger::NullLogger::instance();
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::doPost)");
	}
	if (url.empty()) {
		logger->error("UrlIsEmptyError");
		throw std::invalid_argument("UrlIsEmptyError(YouTubeApiClient::doPost)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl->getRaw(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDS, "");
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDSIZE, 0L);

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, progressCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPost)");
	}

	return readBuffer;
}

std::vector<char> doPostWithString(std::shared_ptr<const Logger::ILogger> logger,
				   std::shared_ptr<CurlHelper::CurlHandle> curl, Jthread::stop_token stoken,
				   const std::string &url, std::string_view body, curl_slist *headers = nullptr)
{
	if (!logger) {
		logger = Logger::NullLogger::instance();
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::doPostWithString)");
	}
	if (url.empty()) {
		logger->error("UrlIsEmptyError");
		throw std::invalid_argument("UrlIsEmptyError(YouTubeApiClient::doPostWithString)");
	}
	if (body.empty()) {
		logger->error("BodyIsEmptyError");
		throw std::invalid_argument("BodyIsEmptyError(YouTubeApiClient::doPostWithString)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl->getRaw(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDS, body.data());
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, progressCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPostWithString)");
	}

	return readBuffer;
}

std::vector<char> doPostWithIfstream(std::shared_ptr<const Logger::ILogger> logger,
				     std::shared_ptr<CurlHelper::CurlHandle> curl, Jthread::stop_token stoken,
				     const std::string &url, std::ifstream &ifs, std::uintmax_t ifsSize,
				     curl_slist *headers = nullptr)
{
	if (!logger) {
		logger = Logger::NullLogger::instance();
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::doPostWithIfstream)");
	}
	if (url.empty()) {
		logger->error("UrlIsEmptyError");
		throw std::invalid_argument("UrlIsEmptyError(YouTubeApiClient::doPostWithIfstream)");
	}
	if (!ifs.is_open()) {
		logger->error("IfstreamIsNotOpenError");
		throw std::invalid_argument("IfstreamIsNotOpenError(YouTubeApiClient::doPostWithIfstream)");
	}
	if (ifsSize == 0) {
		logger->error("IfstreamSizeIsZeroError");
		throw std::invalid_argument("IfstreamSizeIsZeroError(YouTubeApiClient::doPostWithIfstream)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl->getRaw(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POST, 1L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDSIZE, static_cast<long>(ifsSize));

	curl_easy_setopt(curl->getRaw(), CURLOPT_READFUNCTION, CurlHelper::CurlIfstreamReadCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_READDATA, &ifs);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, progressCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_READFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_READDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPostWithIfstream)");
	}

	return readBuffer;
}

std::vector<char> doPutWithString(std::shared_ptr<const Logger::ILogger> logger,
				  std::shared_ptr<CurlHelper::CurlHandle> curl, Jthread::stop_token stoken,
				  const std::string &url, std::string_view body, curl_slist *headers = nullptr)
{
	if (!logger) {
		logger = Logger::NullLogger::instance();
	}

	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::doPutWithString)");
	}
	if (url.empty()) {
		logger->error("UrlIsEmptyError");
		throw std::invalid_argument("UrlIsEmptyError(YouTubeApiClient::doPutWithString)");
	}
	if (body.empty()) {
		logger->error("BodyIsEmptyError");
		throw std::invalid_argument("BodyIsEmptyError(YouTubeApiClient::doPutWithString)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl->getRaw(), CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl->getRaw(), CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDS, body.data());
	curl_easy_setopt(curl->getRaw(), CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl->getRaw(), CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, progressCallback);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, &stoken);

	curl_easy_setopt(curl->getRaw(), CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl->getRaw(), CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl->getRaw());

	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_WRITEDATA, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFOFUNCTION, nullptr);
	curl_easy_setopt(curl->getRaw(), CURLOPT_XFERINFODATA, nullptr);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPutWithString)");
	}

	return readBuffer;
}

std::variant<std::vector<nlohmann::json>, std::shared_ptr<YouTubeApiError>>
performList(std::shared_ptr<const Logger::ILogger> logger, std::shared_ptr<CurlHelper::CurlHandle> curl,
	    Jthread::stop_token stoken, const std::string &url, curl_slist *headers = nullptr, int maxIterations = 20)
{
	std::vector<nlohmann::json> items;
	std::string nextPageToken;
	do {
		CurlHelper::CurlUrlHandle urlHandle;
		urlHandle.setUrl(url.c_str());

		if (!nextPageToken.empty()) {
			CurlHelper::CurlUrlSearchParams params(curl->getRaw());
			params.append("pageToken", nextPageToken);
			std::string qs = params.toString();
			urlHandle.appendQuery(qs.c_str());
		}

		auto nextUrl = urlHandle.c_str();
		std::vector<char> responseBody = doGet(logger, curl, stoken, nextUrl.get(), headers);
		nlohmann::json j = nlohmann::json::parse(responseBody);

		if (j.contains("error")) {
			auto error = std::make_shared<YouTubeApiError>();
			j["error"].get_to(*error);
			logger->error("YouTubeApiError", {{"error", j["error"].dump()}});
			return error;
		}

		nlohmann::json jItems = std::move(j["items"]);
		for (auto &x : jItems) {
			items.push_back(std::move(x));
		}

		if (!j.contains("nextPageToken"))
			break;

		nextPageToken = j["nextPageToken"].get<std::string>();
	} while (--maxIterations > 0);

	return items;
}

char toLowerAscii(char c)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

std::string toLowercase(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), toLowerAscii);
	return s;
}

std::string getLowercaseExtension(const std::filesystem::path &p)
{
	std::string ext = p.extension().string();
	return toLowercase(ext);
}

} // anonymous namespace

YouTubeApiClient::YouTubeApiClient(std::shared_ptr<CurlHelper::CurlHandle> curl)
	: curl_(curl ? curl : throw std::invalid_argument("CurlIsNullError(YouTubeApiClient::YouTubeApiClient)"))
{
}

YouTubeApiClient::~YouTubeApiClient() noexcept = default;

std::variant<std::vector<std::shared_ptr<YouTubeLiveStream>>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::listLiveStreams(Jthread::stop_token stoken, const std::string &accessToken,
				  std::span<const std::string> ids)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::listLiveStreams)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("part", "id,snippet,cdn,status");
	if (ids.empty()) {
		params.append("mine", "true");
	} else {
		params.append("id", fmt::format("{}", fmt::join(ids, ",")));
	}
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveStreams");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	std::variant<std::vector<nlohmann::json>, std::shared_ptr<YouTubeApiError>> listResult =
		performList(logger_, curl_, stoken, url.get(), headers.getRaw());

	if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(listResult)) {
		return std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(listResult);
	}

	const auto &items = std::get<std::vector<nlohmann::json>>(listResult);

	std::vector<std::shared_ptr<YouTubeLiveStream>> liveStreams;
	for (const nlohmann::json &item : items) {
		auto newLiveStream = std::make_shared<YouTubeLiveStream>();
		item.get_to(*newLiveStream);
		liveStreams.push_back(newLiveStream);
	}

	return liveStreams;
}

std::variant<std::vector<std::shared_ptr<YouTubeLiveBroadcast>>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::listLiveBroadcastsByStatus(Jthread::stop_token stoken, const std::string &accessToken,
					     const std::string &broadcastStatus)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::listLiveBroadcastsByStatus)");
	}
	if (broadcastStatus.empty()) {
		logger_->error("BroadcastStatusIsEmptyError");
		throw std::invalid_argument(
			"BroadcastStatusIsEmptyError(YouTubeApiClient::listLiveBroadcastsByStatus)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("part", "id,snippet,contentDetails,status");
	params.append("broadcastStatus", broadcastStatus);
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts");
	urlHandle.appendQuery(qs.c_str());

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	auto url = urlHandle.c_str();
	std::variant<std::vector<nlohmann::json>, std::shared_ptr<YouTubeApiError>> listResult =
		performList(logger_, curl_, stoken, url.get(), headers.getRaw());

	if (std::holds_alternative<std::shared_ptr<YouTubeApi::YouTubeApiError>>(listResult)) {
		return std::get<std::shared_ptr<YouTubeApi::YouTubeApiError>>(listResult);
	}

	const auto &items = std::get<std::vector<nlohmann::json>>(listResult);

	std::vector<std::shared_ptr<YouTubeLiveBroadcast>> broadcasts;
	for (const nlohmann::json &item : items) {
		auto newBroadcast = std::make_shared<YouTubeLiveBroadcast>();
		item.get_to(*newBroadcast);
		broadcasts.push_back(newBroadcast);
	}

	return broadcasts;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::insertLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
				      const InsertingYouTubeLiveBroadcast &insertingLiveBroadcast)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(insertLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("part", "id,snippet,contentDetails,status");
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());
	headers.append("Content-Type: application/json");

	nlohmann::json requestBody = insertingLiveBroadcast;
	std::string bodyStr = requestBody.dump();

	std::vector<char> responseBody = doPostWithString(logger_, curl_, stoken, url.get(), bodyStr, headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);

	if (j.contains("error")) {
		std::shared_ptr<YouTubeApiError> error = std::make_shared<YouTubeApiError>();
		j["error"].get_to(*error);
		logger_->error("YouTubeApiError", {{"error", j["error"].dump()}});
		return error;
	}

	auto liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	j.get_to(*liveBroadcast);
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::updateLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
				      const UpdatingYouTubeLiveBroadcast &updatingLiveBroadcast)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(updateLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("part", "id,snippet,contentDetails,status");
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());
	headers.append("Content-Type: application/json");

	nlohmann::json requestBody = updatingLiveBroadcast;
	std::string bodyStr = requestBody.dump();

	std::vector<char> responseBody = doPutWithString(logger_, curl_, stoken, url.get(), bodyStr, headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);
	if (j.contains("error")) {
		std::shared_ptr<YouTubeApiError> error = std::make_shared<YouTubeApiError>();
		j["error"].get_to(*error);
		logger_->error("YouTubeApiError", {{"error", j["error"].dump()}});
		return error;
	}

	auto liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	j.get_to(*liveBroadcast);
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::bindLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
				    const std::string &broadcastId, const std::optional<std::string> &streamId)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::bindLiveBroadcast)");
	}
	if (broadcastId.empty()) {
		logger_->error("BroadcastIdIsEmptyError");
		throw std::invalid_argument("BroadcastIdIsEmptyError(YouTubeApiClient::bindLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("id", broadcastId);
	params.append("part", "id,snippet,contentDetails,status");
	if (streamId.has_value()) {
		params.append("streamId", streamId.value());
	}

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts/bind");
	std::string qs = params.toString();
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	std::vector<char> responseBody = doPost(logger_, curl_, stoken, url.get(), headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);

	if (j.contains("error")) {
		std::shared_ptr<YouTubeApiError> error = std::make_shared<YouTubeApiError>();
		j["error"].get_to(*error);
		logger_->error("YouTubeApiError", {{"error", j["error"].dump()}});
		return error;
	}

	auto liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	j.get_to(*liveBroadcast);
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::transitionLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
					  const std::string &broadcastId, const std::string &broadcastStatus)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::transitionLiveBroadcast)");
	}
	if (broadcastId.empty()) {
		logger_->error("BroadcastIdIsEmptyError");
		throw std::invalid_argument("BroadcastIdIsEmptyError(YouTubeApiClient::transitionLiveBroadcast)");
	}
	if (broadcastStatus.empty()) {
		logger_->error("BroadcastStatusIsEmptyError");
		throw std::invalid_argument("BroadcastStatusIsEmptyError(YouTubeApiClient::transitionLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("id", broadcastId);
	params.append("broadcastStatus", broadcastStatus);
	params.append("part", "id,snippet,contentDetails,status");
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts/transition");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	logger_->info("TransitioningLiveBroadcast",
		      {{"broadcastId", broadcastId}, {"broadcastStatus", broadcastStatus}});
	std::vector<char> responseBody = doPost(logger_, curl_, stoken, url.get(), headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);

	if (j.contains("error")) {
		std::shared_ptr<YouTubeApiError> error = std::make_shared<YouTubeApiError>();
		j["error"].get_to(*error);
		logger_->error("YouTubeApiError", {{"error", j["error"].dump()}});
		return error;
	}

	auto liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	j.get_to(*liveBroadcast);
	return liveBroadcast;
}

std::variant<std::monostate, std::shared_ptr<YouTubeApiError>>
YouTubeApiClient::setThumbnail(Jthread::stop_token stoken, const std::string &accessToken, const std::string &videoId,
			       const std::filesystem::path &thumbnailPath)
{
	constexpr std::uintmax_t kMaxThumbnailBytes = 2 * 1024 * 1024;

	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::setThumbnail)");
	}
	if (videoId.empty()) {
		logger_->error("VideoIdIsEmptyError");
		throw std::invalid_argument("VideoIdIsEmptyError(YouTubeApiClient::setThumbnail)");
	}
	if (thumbnailPath.empty()) {
		logger_->error("ThumbnailPathIsEmptyError");
		throw std::invalid_argument("ThumbnailPathIsEmptyError(YouTubeApiClient::setThumbnail)");
	}

	if (!std::filesystem::exists(thumbnailPath)) {
		logger_->error("ThumbnailFileNotExistError", {{"path", thumbnailPath.string()}});
		throw std::invalid_argument("ThumbnailFileNotExistError(YouTubeApiClient::setThumbnail)");
	}
	if (!std::filesystem::is_regular_file(thumbnailPath)) {
		logger_->error("ThumbnailNotRegularFileError", {{"path", thumbnailPath.string()}});
		throw std::invalid_argument("ThumbnailNotRegularFileError(YouTubeApiClient::setThumbnail)");
	}

	std::uintmax_t size = std::filesystem::file_size(thumbnailPath);
	if (size > kMaxThumbnailBytes) {
		logger_->error("ThumbnailFileSizeExceedsLimitError", {{"path", thumbnailPath.string()},
								      {"size", std::to_string(size)},
								      {"maxSize", std::to_string(kMaxThumbnailBytes)}});
		throw std::invalid_argument("ThumbnailFileSizeExceedsLimitError(YouTubeApiClient::setThumbnail)");
	}
	// FIXME: Path whitelist will be implemented later.

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("videoId", videoId);
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/upload/youtube/v3/thumbnails/set");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;

	const std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	const std::string ext = getLowercaseExtension(thumbnailPath);
	if (ext == ".png") {
		headers.append("Content-Type: image/png");
	} else if (ext == ".jpg" || ext == ".jpeg") {
		headers.append("Content-Type: image/jpeg");
	} else {
		headers.append("Content-Type: application/octet-stream");
	}

	std::ifstream ifs(thumbnailPath, std::ios::binary);
	if (!ifs.is_open()) {
		logger_->error("ThumbnailFileOpenError", {{"path", thumbnailPath.string()}});
		throw std::runtime_error("ThumbnailFileOpenError(YouTubeApiClient::setThumbnail)");
	}

	std::vector<char> responseBody =
		doPostWithIfstream(logger_, curl_, stoken, url.get(), ifs, size, headers.getRaw());
	ifs.close();

	nlohmann::json j = nlohmann::json::parse(responseBody);
	if (j.contains("error")) {
		std::shared_ptr<YouTubeApiError> error = std::make_shared<YouTubeApiError>();
		j["error"].get_to(*error);
		logger_->error("YouTubeApiError", {{"error", j["error"].dump()}});
		return error;
	}

	return std::monostate{};
}

} // namespace KaitoTokyo::YouTubeApi
