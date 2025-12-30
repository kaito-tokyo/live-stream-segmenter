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

#include <memory>
#include <string>
#include <vector>

#include <ILogger.hpp>

#include "YouTubeTypes.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

class YouTubeApiClient {
public:
	explicit YouTubeApiClient(std::shared_ptr<const Logger::ILogger> logger);
	~YouTubeApiClient() = default;

	std::vector<YouTubeStreamKey> listStreamKeys(const std::string &accessToken) const;

	YouTubeLiveBroadcast createLiveBroadcast(const std::string &accessToken,
						 const YouTubeLiveBroadcastSettings &settings) const;

	void setThumbnail(std::string_view accessToken, std::string_view videoId,
			  const std::filesystem::path &thumbnailPath) const;

private:
	std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
