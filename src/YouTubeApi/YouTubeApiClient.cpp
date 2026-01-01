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

#include <CurlReadCallback.hpp>
#include <CurlSlistHandle.hpp>
#include <CurlUrlHandle.hpp>
#include <CurlUrlSearchParams.hpp>
#include <CurlWriteCallback.hpp>
#include <ILogger.hpp>

namespace KaitoTokyo::YouTubeApi {

namespace {

std::vector<char> doGet(CURL *curl, const char *url, std::shared_ptr<const Logger::ILogger> logger,
			curl_slist *headers = nullptr)
{
	if (!logger) {
		throw std::invalid_argument("LoggerIsNullError(doPost)");
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(doGet)");
	}
	if (!url) {
		logger->error("UrlIsNullError");
		throw std::invalid_argument("UrlIsNullError(doGet)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 2L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(doGet)");
	}

	return readBuffer;
}

std::vector<char> doPost(CURL *curl, const char *url, std::shared_ptr<const Logger::ILogger> logger,
			 curl_slist *headers = nullptr)
{
	if (!logger) {
		throw std::invalid_argument("LoggerIsNullError(doPost)");
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(doPost)");
	}
	if (!url) {
		logger->error("UrlIsNullError");
		throw std::invalid_argument("UrlIsNullError(doPost)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(doPost)");
	}

	return readBuffer;
}

std::vector<char> doPost(CURL *curl, const char *url, std::string_view body,
			 std::shared_ptr<const Logger::ILogger> logger, curl_slist *headers = nullptr)
{
	if (!logger) {
		throw std::invalid_argument("LoggerIsNullError(doPost)");
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(doPost)");
	}
	if (!url) {
		logger->error("UrlIsNullError");
		throw std::invalid_argument("UrlIsNullError(doPost)");
	}
	if (body.empty()) {
		logger->error("BodyIsEmptyError");
		throw std::invalid_argument("BodyIsEmptyError(doPost)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(doPost)");
	}

	return readBuffer;
}

std::vector<char> doPost(CURL *curl, const char *url, std::ifstream &ifs, std::uintmax_t ifsSize,
			 std::shared_ptr<const Logger::ILogger> logger, curl_slist *headers = nullptr)
{
	if (!logger) {
		throw std::invalid_argument("LoggerIsNullError(doPost)");
	}
	if (!curl) {
		logger->error("CurlIsNullError");
		throw std::invalid_argument("CurlIsNullError(doPost)");
	}
	if (!url) {
		logger->error("UrlIsNullError");
		throw std::invalid_argument("UrlIsNullError(doPost)");
	}
	if (!ifs.is_open()) {
		logger->error("IfstreamIsNotOpenError");
		throw std::invalid_argument("IfstreamIsNotOpenError(doPost)");
	}
	if (ifsSize == 0) {
		logger->error("IfstreamSizeIsZeroError");
		throw std::invalid_argument("IfstreamSizeIsZeroError(doPost)");
	}

	std::vector<char> readBuffer;

	curl_easy_reset(curl);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(ifsSize));

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, CurlHelper::CurlIfstreamReadCallback);
	curl_easy_setopt(curl, CURLOPT_READDATA, &ifs);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlCharVectorWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(doPost)");
	}

	return readBuffer;
}

std::vector<nlohmann::json> performList(CURL *curl, const char *url, std::shared_ptr<const Logger::ILogger> logger,
					curl_slist *headers = nullptr, int maxIterations = 20)
{
	std::vector<nlohmann::json> items;
	std::string nextPageToken;
	do {
		CurlHelper::CurlUrlHandle urlHandle;
		urlHandle.setUrl(url);

		if (!nextPageToken.empty()) {
			CurlHelper::CurlUrlSearchParams params(curl);
			params.append("pageToken", nextPageToken);
			std::string qs = params.toString();
			urlHandle.appendQuery(qs.c_str());
		}

		auto url = urlHandle.c_str();
		std::vector<char> responseBody = doGet(curl, url.get(), logger, headers);
		nlohmann::json j = nlohmann::json::parse(responseBody);

		if (j.contains("error")) {
			logger->error("YouTubeApiError", {{"error", j["error"].dump()}});
			throw std::runtime_error("APIError(performList)");
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
	: curl_(curl ? curl : throw std::invalid_argument("CurlIsNullError(YouTubeApiClient)"))
{
}

YouTubeApiClient::~YouTubeApiClient() noexcept = default;

std::vector<YouTubeLiveStream> YouTubeApiClient::listLiveStreams(std::string_view accessToken,
								 std::span<std::string_view> ids)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(listLiveStreams)");
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

	std::vector<nlohmann::json> items = performList(curl_->getRaw(), url.get(), logger_, headers.getRaw());

	std::vector<YouTubeLiveStream> liveStreams;
	for (const nlohmann::json &item : items) {
		liveStreams.push_back(item.get<YouTubeLiveStream>());
	}

	return liveStreams;
}

std::vector<YouTubeLiveBroadcast> YouTubeApiClient::listLiveBroadcastsByStatus(std::string_view accessToken,
									       std::string broadcastStatus)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(listLiveBroadcastsByStatus)");
	}
	if (broadcastStatus.empty()) {
		logger_->error("BroadcastStatusIsEmptyError");
		throw std::invalid_argument("BroadcastStatusIsEmptyError(listLiveBroadcastsByStatus)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("part", "id,snippet,contentDetails,status");
	params.append("broadcastStatus", std::move(broadcastStatus));
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts");
	urlHandle.appendQuery(qs.c_str());

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	auto url = urlHandle.c_str();
	std::vector<nlohmann::json> items = performList(curl_->getRaw(), url.get(), logger_, headers.getRaw());

	std::vector<YouTubeLiveBroadcast> broadcasts;
	for (const nlohmann::json &item : items) {
		broadcasts.push_back(item.get<YouTubeLiveBroadcast>());
	}

	return broadcasts;
}

YouTubeLiveBroadcast YouTubeApiClient::insertLiveBroadcast(std::string_view accessToken,
							   const YouTubeLiveBroadcastSettings &settings)
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

	nlohmann::json requestBody = settings;
	std::string bodyStr = requestBody.dump();

	std::vector<char> responseBody = doPost(curl_->getRaw(), url.get(), bodyStr, logger_, headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);
	return j.get<YouTubeLiveBroadcast>();
}

YouTubeLiveBroadcast YouTubeApiClient::bindLiveBroadcast(std::string_view accessToken, std::string broadcastId,
							 std::optional<std::string> streamId)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(bindLiveBroadcast)");
	}
	if (broadcastId.empty()) {
		logger_->error("BroadcastIdIsEmptyError");
		throw std::invalid_argument("BroadcastIdIsEmptyError(bindLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("id", std::move(broadcastId));
	params.append("part", "id,snippet,contentDetails,status");
	if (streamId.has_value()) {
		params.append("streamId", std::move(streamId.value()));
	}

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/youtube/v3/liveBroadcasts/bind");
	std::string qs = params.toString();
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	std::vector<char> responseBody = doPost(curl_->getRaw(), url.get(), logger_, headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);
	return j.get<YouTubeLiveBroadcast>();
}

YouTubeLiveBroadcast YouTubeApiClient::transitionLiveBroadcast(std::string_view accessToken, std::string broadcastId,
							       std::string broadcastStatus)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(transitionLiveBroadcast)");
	}
	if (broadcastId.empty()) {
		logger_->error("BroadcastIdIsEmptyError");
		throw std::invalid_argument("BroadcastIdIsEmptyError(transitionLiveBroadcast)");
	}
	if (broadcastStatus.empty()) {
		logger_->error("BroadcastStatusIsEmptyError");
		throw std::invalid_argument("BroadcastStatusIsEmptyError(transitionLiveBroadcast)");
	}

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("id", std::move(broadcastId));
	params.append("broadcastStatus", std::move(broadcastStatus));
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
	std::vector<char> responseBody = doPost(curl_->getRaw(), url.get(), logger_, headers.getRaw());

	nlohmann::json j = nlohmann::json::parse(responseBody);
	return j.get<YouTubeLiveBroadcast>();
}

void YouTubeApiClient::setThumbnail(std::string_view accessToken, std::string videoId,
				    const std::filesystem::path &thumbnailPath)
{
	constexpr std::uintmax_t kMaxThumbnailBytes = 2 * 1024 * 1024;

	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(setThumbnail)");
	}
	if (videoId.empty()) {
		logger_->error("VideoIdIsEmptyError");
		throw std::invalid_argument("VideoIdIsEmptyError(setThumbnail)");
	}
	if (thumbnailPath.empty()) {
		logger_->error("ThumbnailPathIsEmptyError");
		throw std::invalid_argument("ThumbnailPathIsEmptyError(setThumbnail)");
	}

	if (!std::filesystem::exists(thumbnailPath)) {
		logger_->error("ThumbnailFileNotExistError", {{"path", thumbnailPath.string()}});
		throw std::invalid_argument("ThumbnailFileNotExistError(setThumbnail)");
	}
	if (!std::filesystem::is_regular_file(thumbnailPath)) {
		logger_->error("ThumbnailNotRegularFileError", {{"path", thumbnailPath.string()}});
		throw std::invalid_argument("ThumbnailNotRegularFileError(setThumbnail)");
	}

	std::uintmax_t size = std::filesystem::file_size(thumbnailPath);
	if (size > kMaxThumbnailBytes) {
		logger_->error("ThumbnailFileSizeExceedsLimitError", {{"path", thumbnailPath.string()},
								      {"size", std::to_string(size)},
								      {"maxSize", std::to_string(kMaxThumbnailBytes)}});
		throw std::invalid_argument("ThumbnailFileSizeExceedsLimitError(setThumbnail)");
	}
	// FIXME: Path whitelist will be implemented later.

	CurlHelper::CurlUrlSearchParams params(curl_->getRaw());
	params.append("videoId", std::move(videoId));
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
		throw std::runtime_error("ThumbnailFileOpenError(setThumbnail)");
	}

	std::vector<char> responseBody = doPost(curl_->getRaw(), url.get(), ifs, size, logger_, headers.getRaw());
	ifs.close();
}

} // namespace KaitoTokyo::YouTubeApi
