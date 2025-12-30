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

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

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

struct YouTubeLiveBroadcastThumbnail {
	std::string url;
	std::optional<std::uint32_t> width;
	std::optional<std::uint32_t> height;
};

struct YouTubeLiveBroadcast {
	// Top-level fields
	std::string kind;
	std::string etag;
	std::string id;

	// snippet fields
	std::string snippet_publishedAt;
	std::string snippet_channelId;
	std::string snippet_title;
	std::string snippet_description;
	std::unordered_map<std::string, YouTubeLiveBroadcastThumbnail> snippet_thumbnails;
	std::string snippet_scheduledStartTime;
	std::optional<std::string> snippet_scheduledEndTime;
	std::optional<std::string> snippet_actualStartTime;
	std::optional<std::string> snippet_actualEndTime;
	std::optional<bool> snippet_isDefaultBroadcast;
	std::optional<std::string> snippet_liveChatId;

	// status fields
	std::string status_lifeCycleStatus;
	std::string status_privacyStatus;
	std::string status_recordingStatus;
	std::optional<bool> status_madeForKids;
	std::optional<bool> status_selfDeclaredMadeForKids;

	// contentDetails fields
	std::optional<std::string> contentDetails_boundStreamId;
	std::optional<std::string> contentDetails_boundStreamLastUpdateTimeMs;
	std::optional<bool> contentDetails_monitorStream_enableMonitorStream;
	std::optional<std::uint32_t> contentDetails_monitorStream_broadcastStreamDelayMs;
	std::optional<std::string> contentDetails_monitorStream_embedHtml;
	std::optional<bool> contentDetails_enableEmbed;
	std::optional<bool> contentDetails_enableDvr;
	std::optional<bool> contentDetails_recordFromStart;
	std::optional<bool> contentDetails_enableClosedCaptions;
	std::optional<std::string> contentDetails_closedCaptionsType;
	std::optional<std::string> contentDetails_projection;
	std::optional<bool> contentDetails_enableLowLatency;
	std::optional<std::string> contentDetails_latencyPreference;
	std::optional<bool> contentDetails_enableAutoStart;
	std::optional<bool> contentDetails_enableAutoStop;

	// statistics fields
	std::optional<uint64_t> statistics_totalChatCount;

	// monetizationDetails fields
	std::optional<bool> monetizationDetails_cuepointSchedule_enabled;
	std::optional<std::string> monetizationDetails_cuepointSchedule_pauseAdsUntil;
	std::optional<std::string> monetizationDetails_cuepointSchedule_scheduleStrategy;
	std::optional<uint32_t> monetizationDetails_cuepointSchedule_repeatIntervalSecs;
};

void to_json(nlohmann::json &j, const YouTubeLiveBroadcast &p);
void from_json(const nlohmann::json &j, YouTubeLiveBroadcast &p);

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
void from_json(const nlohmann::json &j, YouTubeLiveBroadcastSettings &p);

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
