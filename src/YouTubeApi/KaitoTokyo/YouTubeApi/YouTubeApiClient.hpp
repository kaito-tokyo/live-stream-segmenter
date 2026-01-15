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

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <string>
#include <variant>
#include <vector>

#include <KaitoTokyo/CurlHelper/CurlHandle.hpp>
#include <KaitoTokyo/Jthread/Jthread.hpp>
#include <KaitoTokyo/Logger/ILogger.hpp>

#include "YouTubeTypes.hpp"

namespace KaitoTokyo::YouTubeApi {

class YouTubeApiClient {
public:
	YouTubeApiClient(std::shared_ptr<CurlHelper::CurlHandle> curl);

	~YouTubeApiClient() noexcept;

	void setLogger(std::shared_ptr<const Logger::ILogger> logger) { logger_ = std::move(logger); }

	std::variant<std::vector<std::shared_ptr<YouTubeLiveStream>>, std::shared_ptr<YouTubeApiError>>
	listLiveStreams(Jthread::stop_token stoken, const std::string &accessToken,
			std::span<const std::string> ids = {});

	std::variant<std::shared_ptr<YouTubeLiveStream>, std::shared_ptr<YouTubeApiError>>
	insertLiveStream(Jthread::stop_token stoken, const std::string &accessToken,
			 std::shared_ptr<const InsertingYouTubeLiveStream> insertingLiveStream);

	std::variant<std::monostate, std::shared_ptr<YouTubeApiError>>
	deleteLiveStream(Jthread::stop_token stoken, const std::string &accessToken, const std::string &liveStreamId);

	std::variant<std::vector<std::shared_ptr<YouTubeLiveBroadcast>>, std::shared_ptr<YouTubeApiError>>
	listLiveBroadcastsByStatus(Jthread::stop_token stoken, const std::string &accessToken,
				   const std::string &broadcastStatus);

	std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
	insertLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
			    const InsertingYouTubeLiveBroadcast &insertingLiveBroadcast);

	std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
	updateLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
			    const UpdatingYouTubeLiveBroadcast &updatingLiveBroadcast);

	std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
	bindLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken, const std::string &broadcastId,
			  const std::optional<std::string> &streamId);

	std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeApiError>>
	transitionLiveBroadcast(Jthread::stop_token stoken, const std::string &accessToken,
				const std::string &broadcastId, const std::string &broadcastStatus);

	std::variant<std::monostate, std::shared_ptr<YouTubeApiError>>
	setThumbnail(Jthread::stop_token stoken, const std::string &accessToken, const std::string &videoId,
		     const std::filesystem::path &thumbnailPath);

private:
	std::shared_ptr<CurlHelper::CurlHandle> curl_;

	std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::YouTubeApi
