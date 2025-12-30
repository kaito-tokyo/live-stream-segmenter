/*
 * Live Stream Segmenter -  YouTubeApi Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
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
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <CurlReader.hpp>
#include <CurlSlistHandle.hpp>
#include <CurlUrlHandler.hpp>
#include <CurlUrlSearchParams.hpp>
#include <CurlVectorWriter.hpp>
#include <ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

namespace {

std::string doGet(CURL *curl, const char *url, std::shared_ptr<const Logger::ILogger> logger,
		  curl_slist *headers = nullptr)
{
	assert(curl);
	assert(url);

	CurlHelper::CurlVectorWriterBuffer readBuffer;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doGet)");
	}

	return std::string(readBuffer.begin(), readBuffer.end());
}

std::string doPost(CURL *curl, const char *url, std::string_view body, std::shared_ptr<const Logger::ILogger> logger,
		   curl_slist *headers = nullptr)
{
	assert(curl);
	assert(url);
	assert(!body.empty());
	assert(logger);

	CurlHelper::CurlVectorWriterBuffer readBuffer;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<curl_off_t>(body.size()));

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPost)");
	}

	return std::string(readBuffer.begin(), readBuffer.end());
}

std::string doPost(CURL *curl, const char *url, std::ifstream &ifs, std::streamsize ifsSize,
		   std::shared_ptr<const Logger::ILogger> logger, curl_slist *headers = nullptr)
{
	assert(curl);
	assert(url);
	assert(ifs.is_open());
	assert(ifsSize > 0);
	assert(logger);

	CurlHelper::CurlVectorWriterBuffer readBuffer;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, CurlHelper::CurlIfstreamReadCallback);
	curl_easy_setopt(curl, CURLOPT_READDATA, &ifs);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<curl_off_t>(ifsSize));

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlHelper::CurlVectorWriter);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		logger->error("CurlPerformError", {{"error", curl_easy_strerror(res)}});
		throw std::runtime_error("CurlPerformError(YouTubeApiClient::doPost)");
	}

	return std::string(readBuffer.begin(), readBuffer.end());
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
		std::string responseBody = doGet(curl, url.get(), logger, headers);
		nlohmann::json j = nlohmann::json::parse(responseBody);

		if (j.contains("error")) {
			logger->error("YouTubeApiError", {{"error", j["error"].dump()}});
			throw std::runtime_error("APIError(YouTubeApiClient::performList)");
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

} // anonymous namespace

YouTubeApiClient::YouTubeApiClient(std::shared_ptr<const Logger::ILogger> logger)
	: logger_(logger ? std::move(logger)
			 : throw std::invalid_argument("LoggerIsNullError(YouTubeApiClient::YouTubeApiClient)")),
	  curl_([]() {
		  auto ptr = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>(curl_easy_init(), &curl_easy_cleanup);
		  if (!ptr) {
			  throw std::runtime_error("CurlInitError(YouTubeApiClient::YouTubeApiClient)");
		  }
		  return ptr;
	  }())
{
}

YouTubeApiClient::~YouTubeApiClient() noexcept = default;

std::vector<YouTubeStreamKey> YouTubeApiClient::listStreamKeys(std::string_view accessToken)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::listStreamKeys)");
	}

	curl_easy_reset(curl_.get());

	const char *url = "https://www.googleapis.com/youtube/v3/liveStreams?part=snippet,cdn&mine=true";

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	std::vector<nlohmann::json> items = performList(curl_.get(), url, logger_, headers.get());

	std::vector<YouTubeStreamKey> streamKeys;
	for (const nlohmann::json &item : items) {
		streamKeys.push_back(item.get<YouTubeStreamKey>());
	}

	return streamKeys;
}

YouTubeLiveBroadcast YouTubeApiClient::createLiveBroadcast(std::string_view accessToken,
							   const YouTubeLiveBroadcastSettings &settings)
{
	if (accessToken.empty()) {
		logger_->error("AccessTokenIsEmptyError");
		throw std::invalid_argument("AccessTokenIsEmptyError(YouTubeApiClient::createLiveBroadcast)");
	}

	curl_easy_reset(curl_.get());

	const char *url = "https://www.googleapis.com/youtube/v3/liveBroadcasts?part=snippet,status,contentDetails";

	CurlHelper::CurlSlistHandle headers;
	std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());
	headers.append("Content-Type: application/json");

	nlohmann::json requestBody = settings;
	std::string bodyStr = requestBody.dump();

	std::string responseBody = doPost(curl_.get(), url, bodyStr, logger_, headers.get());
	logger_->info("LiveBroadcastCreated", {{"responseBody", responseBody}});

	nlohmann::json j = nlohmann::json::parse(responseBody);
	return j.get<YouTubeLiveBroadcast>();
}

void YouTubeApiClient::setThumbnail(std::string_view accessToken, std::string_view videoId,
				    const std::filesystem::path &thumbnailPath)
{
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
	// FIXME: Path whitelist will be implemented later.

	curl_easy_reset(curl_.get());

	CurlHelper::CurlUrlSearchParams params(curl_.get());
	params.append("videoId", videoId);
	std::string qs = params.toString();

	CurlHelper::CurlUrlHandle urlHandle;
	urlHandle.setUrl("https://www.googleapis.com/upload/youtube/v3/thumbnails/set");
	urlHandle.appendQuery(qs.c_str());
	auto url = urlHandle.c_str();

	CurlHelper::CurlSlistHandle headers;

	const std::string authHeader = fmt::format("Authorization: Bearer {}", accessToken);
	headers.append(authHeader.c_str());

	const std::filesystem::path extension = thumbnailPath.extension();
	if (extension == ".png") {
		headers.append("Content-Type: image/png");
	} else if (extension == ".jpg" || extension == ".jpeg") {
		headers.append("Content-Type: image/jpeg");
	} else {
		headers.append("Content-Type: application/octet-stream");
	}

	std::ifstream ifs(thumbnailPath, std::ios::binary);
	if (!ifs.is_open()) {
		logger_->error("ThumbnailFileOpenError", {{"path", thumbnailPath.string()}});
		throw std::runtime_error("ThumbnailFileOpenError(YouTubeApiClient::setThumbnail)");
	}
	ifs.seekg(0, std::ios::end);
	if (!ifs.good()) {
		logger_->error("ThumbnailFileSeekError", {{"path", thumbnailPath.string()}, {"position", "end"}});
		throw std::runtime_error("ThumbnailFileSeekError(YouTubeApiClient::setThumbnail)");
	}
	std::streamsize size = ifs.tellg();
	if (size < 0) {
		logger_->error("ThumbnailFileTellError", {{"path", thumbnailPath.string()}});
		throw std::runtime_error("ThumbnailFileTellError(YouTubeApiClient::setThumbnail)");
	}
	ifs.seekg(0, std::ios::beg);
	if (!ifs.good()) {
		logger_->error("ThumbnailFileSeekError", {{"path", thumbnailPath.string()}, {"position", "beg"}});
		throw std::runtime_error("ThumbnailFileSeekError(YouTubeApiClient::setThumbnail)");
	}

	constexpr std::streamsize kMaxThumbnailBytes = 2 * 1024 * 1024;
	if (size <= 0) {
		logger_->error("InvalidFileSizeError", {{"size", std::to_string(size)}});
		throw std::runtime_error("InvalidFileSizeError(YouTubeApiClient::setThumbnail)");
	}
	if (size > kMaxThumbnailBytes) {
		logger_->error("FileSizeExceedsLimitError",
			       {{"size", std::to_string(size)}, {"maxSize", std::to_string(kMaxThumbnailBytes)}});
		throw std::runtime_error("FileSizeExceedsLimitError(YouTubeApiClient::setThumbnail)");
	}

	std::string responseBody = doPost(curl_.get(), url.get(), ifs, size, logger_, headers.get());
	logger_->info("ThumbnailSet", {{"responseBody", responseBody}});
}

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
