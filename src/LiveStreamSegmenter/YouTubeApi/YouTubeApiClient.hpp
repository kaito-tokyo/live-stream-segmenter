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

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <curl/curl.h>

#include <ILogger.hpp>

#include "YouTubeTypes.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

class YouTubeApiClient {
public:
	explicit YouTubeApiClient(std::shared_ptr<const Logger::ILogger> logger);
	~YouTubeApiClient() noexcept;

	std::vector<YouTubeStreamKey> listStreamKeys(std::string_view accessToken);

	YouTubeLiveBroadcast createLiveBroadcast(std::string_view accessToken,
						 const YouTubeLiveBroadcastSettings &settings);

	void setThumbnail(std::string_view accessToken, std::string_view videoId,
			  const std::filesystem::path &thumbnailPath);

private:
	std::shared_ptr<const Logger::ILogger> logger_;

	const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
