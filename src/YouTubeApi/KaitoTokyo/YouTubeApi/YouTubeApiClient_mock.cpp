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

std::vector<YouTubeLiveStream> YouTubeApiClient::listLiveStreams([[maybe_unused]] const std::string &accessToken,
								 [[maybe_unused]] std::span<const std::string> ids)
{
	YouTubeLiveStream stream;
	stream.id = "mocked_stream_id";
	stream.snippet.title = "Mocked Stream";
	return {stream};
}

std::vector<YouTubeLiveBroadcast> YouTubeApiClient::listLiveBroadcastsByStatus([[maybe_unused]] const std::string &accessToken,
									       [[maybe_unused]] const std::string &broadcastStatus)
{
	YouTubeLiveBroadcast broadcast;
	broadcast.id = "mocked_broadcast_id";
	if (!broadcast.snippet)
		broadcast.snippet.emplace();
	broadcast.snippet->title = "Mocked Broadcast";
	return {broadcast};
}

YouTubeLiveBroadcast YouTubeApiClient::insertLiveBroadcast([[maybe_unused]] const std::string &accessToken,
							   [[maybe_unused]] const InsertingYouTubeLiveBroadcast &insertingLiveBroadcast)
{
	YouTubeLiveBroadcast broadcast;
	broadcast.id = "mocked_inserted_broadcast_id";
	if (!broadcast.snippet)
		broadcast.snippet.emplace();
	broadcast.snippet->title = insertingLiveBroadcast.snippet.title;
	return broadcast;
}

YouTubeLiveBroadcast YouTubeApiClient::updateLiveBroadcast([[maybe_unused]] const std::string &accessToken,
							   [[maybe_unused]] const UpdatingYouTubeLiveBroadcast &updatingLiveBroadcast)
{
	YouTubeLiveBroadcast broadcast;
	broadcast.id = updatingLiveBroadcast.id;
	if (!broadcast.snippet)
		broadcast.snippet.emplace();
	if (updatingLiveBroadcast.snippet.title)
		broadcast.snippet->title = *updatingLiveBroadcast.snippet.title;
	return broadcast;
}

YouTubeLiveBroadcast YouTubeApiClient::bindLiveBroadcast([[maybe_unused]] const std::string &accessToken, const std::string &broadcastId,
							 [[maybe_unused]] const std::optional<std::string> &streamId)
{
	YouTubeLiveBroadcast broadcast;
	broadcast.id = broadcastId;
	if (!broadcast.snippet)
		broadcast.snippet.emplace();
	broadcast.snippet->title = "Bound Broadcast";
	return broadcast;
}

YouTubeLiveBroadcast YouTubeApiClient::transitionLiveBroadcast([[maybe_unused]] const std::string &accessToken,
							       [[maybe_unused]] const std::string &broadcastId,
							       [[maybe_unused]] const std::string &broadcastStatus)
{
	YouTubeLiveBroadcast broadcast;
	broadcast.id = broadcastId;
	if (!broadcast.snippet)
		broadcast.snippet.emplace();
	broadcast.snippet->title = "Transitioned Broadcast";
	return broadcast;
}

void YouTubeApiClient::setThumbnail([[maybe_unused]] const std::string &accessToken, [[maybe_unused]] const std::string &videoId,
				    [[maybe_unused]] const std::filesystem::path &thumbnailPath)
{
	// モック: 何もしない
}

} // namespace KaitoTokyo::YouTubeApi
