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

#include <nlohmann/json_fwd.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

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

void to_json(nlohmann::json &j, const YouTubeStreamKey &p);
void from_json(const nlohmann::json &j, YouTubeStreamKey &p);

struct YouTubeLiveBroadcastSettings {
	std::string snippet_title;
	std::string snippet_scheduledStartTime;
	std::string snippet_description;

	std::string status_privacyStatus = "private";
	bool status_selfDeclaredMadeForKids = false;

	bool contentDetails_enableAutoStart = false;
	bool contentDetails_enableAutoStop = false;
	bool contentDetails_enableDvr = true;
	bool contentDetails_enableEmbed = true;
	bool contentDetails_recordFromStart = true;
	std::string contentDetails_latencyPreference = "normal"; // "normal", "low", "ultraLow"

	bool contentDetails_monitorStream_enableMonitorStream = false;
};

void to_json(nlohmann::json &j, const YouTubeLiveBroadcastSettings &p);

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
