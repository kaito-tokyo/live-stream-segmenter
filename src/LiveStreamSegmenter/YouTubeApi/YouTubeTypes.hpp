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

#include <string>

#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

/**
 * @brief Represents a YouTube stream key and its associated metadata.
 * Contains stream ID, title, stream name, resolution, and frame rate.
 */
struct YouTubeStreamKey {
	std::string id;
	std::string kind;
	std::string snippet_title;
	std::string snippet_description;
	std::string snippet_channelId;
	std::string snippet_publishedAt;
	std::string snippet_privacyStatus;
	std::string cdn_ingestionType;
	std::string cdn_resolution;
	std::string cdn_frameRate;
	std::string cdn_isReusable;
	std::string cdn_region;
	std::string cdn_ingestionInfo_streamName;
	std::string cdn_ingestionInfo_ingestionAddress;
	std::string cdn_ingestionInfo_backupIngestionAddress;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(YouTubeStreamKey, id, kind, snippet_title, snippet_description, snippet_channelId,
				   snippet_publishedAt, snippet_privacyStatus, cdn_ingestionType, cdn_resolution,
				   cdn_frameRate, cdn_isReusable, cdn_region, cdn_ingestionInfo_streamName,
				   cdn_ingestionInfo_ingestionAddress, cdn_ingestionInfo_backupIngestionAddress)

struct YouTubeBroadcast {
	std::string id;
	std::string snippet_title;
	std::string snippet_description;
	std::string snippet_scheduledStartTime;
	std::string status_privacyStatus;
	std::string contentDetails_boundStreamId;
	std::string kind;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
