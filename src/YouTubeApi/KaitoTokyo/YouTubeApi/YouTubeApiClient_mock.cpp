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

#include <utility>

namespace KaitoTokyo::YouTubeApi {

YouTubeApiClient::YouTubeApiClient(std::shared_ptr<CurlHelper::CurlHandle> curl)
	: curl_(std::move(curl)),
	  logger_(nullptr)
{
}

YouTubeApiClient::~YouTubeApiClient() noexcept = default;

std::variant<std::vector<std::shared_ptr<YouTubeLiveStream>>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::listLiveStreams([[maybe_unused]] Jthread::stop_token stoken,
				  [[maybe_unused]] const std::string &accessToken,
				  [[maybe_unused]] std::span<const std::string> ids)
{
	std::vector<std::shared_ptr<YouTubeLiveStream>> liveStreams{std::make_shared<YouTubeLiveStream>()};
	liveStreams[0]->id = "mocked_stream_id";
	liveStreams[0]->snippet.title = "Mocked Stream";
	return liveStreams;
}

std::variant<std::vector<std::shared_ptr<YouTubeLiveBroadcast>>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::listLiveBroadcastsByStatus([[maybe_unused]] Jthread::stop_token stoken,
					     [[maybe_unused]] const std::string &accessToken,
					     [[maybe_unused]] const std::string &broadcastStatus)
{
	std::vector<std::shared_ptr<YouTubeLiveBroadcast>> liveBroadcasts{std::make_shared<YouTubeLiveBroadcast>()};
	liveBroadcasts[0]->id = "mocked_broadcast_id";
	if (!liveBroadcasts[0]->snippet)
		liveBroadcasts[0]->snippet.emplace();
	liveBroadcasts[0]->snippet->title = "Mocked Broadcast";
	return liveBroadcasts;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::insertLiveBroadcast([[maybe_unused]] Jthread::stop_token stoken,
				      [[maybe_unused]] const std::string &accessToken,
				      [[maybe_unused]] const InsertingYouTubeLiveBroadcast &insertingLiveBroadcast)
{
	std::shared_ptr<YouTubeLiveBroadcast> liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	liveBroadcast->id = "mocked_inserted_broadcast_id";
	if (!liveBroadcast->snippet)
		liveBroadcast->snippet.emplace();
	liveBroadcast->snippet->title = insertingLiveBroadcast.snippet.title;
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::updateLiveBroadcast([[maybe_unused]] Jthread::stop_token stoken,
				      [[maybe_unused]] const std::string &accessToken,
				      [[maybe_unused]] const UpdatingYouTubeLiveBroadcast &updatingLiveBroadcast)
{
	std::shared_ptr<YouTubeLiveBroadcast> liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	liveBroadcast->id = updatingLiveBroadcast.id;
	if (!liveBroadcast->snippet)
		liveBroadcast->snippet.emplace();
	if (updatingLiveBroadcast.snippet.title)
		liveBroadcast->snippet->title = *updatingLiveBroadcast.snippet.title;
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::bindLiveBroadcast([[maybe_unused]] Jthread::stop_token stoken,
				    [[maybe_unused]] const std::string &accessToken, const std::string &broadcastId,
				    [[maybe_unused]] const std::optional<std::string> &streamId)
{
	std::shared_ptr<YouTubeLiveBroadcast> liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	liveBroadcast->id = broadcastId;
	if (!liveBroadcast->snippet)
		liveBroadcast->snippet.emplace();
	liveBroadcast->snippet->title = "Bound Broadcast";
	return liveBroadcast;
}

std::variant<std::shared_ptr<YouTubeLiveBroadcast>, std::shared_ptr<YouTubeError>>
YouTubeApiClient::transitionLiveBroadcast([[maybe_unused]] Jthread::stop_token stoken,
					  [[maybe_unused]] const std::string &accessToken,
					  [[maybe_unused]] const std::string &broadcastId,
					  [[maybe_unused]] const std::string &broadcastStatus)
{
	std::shared_ptr<YouTubeLiveBroadcast> liveBroadcast = std::make_shared<YouTubeLiveBroadcast>();
	liveBroadcast->id = broadcastId;
	if (!liveBroadcast->snippet)
		liveBroadcast->snippet.emplace();
	liveBroadcast->snippet->title = "Transitioned Broadcast";
	return liveBroadcast;
}

std::variant<std::monostate, std::shared_ptr<YouTubeError>> YouTubeApiClient::setThumbnail(
	[[maybe_unused]] Jthread::stop_token stoken, [[maybe_unused]] const std::string &accessToken,
	[[maybe_unused]] const std::string &videoId, [[maybe_unused]] const std::filesystem::path &thumbnailPath)
{
	return std::monostate{};
}

} // namespace KaitoTokyo::YouTubeApi
