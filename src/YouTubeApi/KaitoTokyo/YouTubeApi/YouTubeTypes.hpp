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

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include <nlohmann/json_fwd.hpp>

namespace KaitoTokyo::YouTubeApi {

struct YouTubeLiveStream {
	std::string kind;
	std::string etag;
	std::string id;

	struct Snippet {
		std::string publishedAt;
		std::string channelId;
		std::string title;
		std::string description;
		std::optional<bool> isDefaultStream;
	} snippet;

	struct Cdn {
		std::string ingestionType;
		struct IngestionInfo {
			std::string streamName;
			std::string ingestionAddress;
			std::string backupIngestionAddress;
		} ingestionInfo;
		std::string resolution;
		std::string frameRate;
	} cdn;

	struct Status {
		std::string streamStatus;
		struct HealthStatus {
			std::string status;
			std::optional<std::uint64_t> lastUpdateTimeSeconds;
			struct ConfigurationIssue {
				std::string type;
				std::string severity;
				std::string reason;
				std::string description;
			};
			std::vector<ConfigurationIssue> configurationIssues;
		} healthStatus;
	};
	std::optional<Status> status;

	struct ContentDetails {
		std::string closedCaptionsIngestionUrl;
		std::optional<bool> isReusable;
	};
	std::optional<ContentDetails> contentDetails;
};

void to_json(nlohmann::json &j, const YouTubeLiveStream &p);
void from_json(const nlohmann::json &j, YouTubeLiveStream &p);

struct YouTubeLiveBroadcastThumbnail {
	std::string url;
	std::optional<std::uint32_t> width;
	std::optional<std::uint32_t> height;
};

struct YouTubeLiveBroadcast {
	std::optional<std::string> kind;
	std::optional<std::string> etag;
	std::optional<std::string> id;

	struct Snippet {
		std::optional<std::string> publishedAt;
		std::optional<std::string> channelId;
		std::optional<std::string> title;
		std::optional<std::string> description;
		std::optional<std::unordered_map<std::string, YouTubeLiveBroadcastThumbnail>> thumbnails;
		std::optional<std::string> scheduledStartTime;
		std::optional<std::string> scheduledEndTime;
		std::optional<std::string> actualStartTime;
		std::optional<std::string> actualEndTime;
		std::optional<bool> isDefaultBroadcast;
		std::optional<std::string> liveChatId;
	};
	std::optional<Snippet> snippet;

	struct Status {
		std::optional<std::string> lifeCycleStatus;
		std::optional<std::string> privacyStatus;
		std::optional<std::string> recordingStatus;
		std::optional<bool> madeForKids;
		std::optional<bool> selfDeclaredMadeForKids;
	};
	std::optional<Status> status;

	struct ContentDetails {
		std::optional<std::string> boundStreamId;
		std::optional<std::string> boundStreamLastUpdateTimeMs;
		struct MonitorStream {
			std::optional<bool> enableMonitorStream;
			std::optional<std::uint32_t> broadcastStreamDelayMs;
			std::optional<std::string> embedHtml;
		};
		std::optional<MonitorStream> monitorStream;
		std::optional<bool> enableEmbed;
		std::optional<bool> enableDvr;
		std::optional<bool> recordFromStart;
		std::optional<bool> enableClosedCaptions;
		std::optional<std::string> closedCaptionsType;
		std::optional<std::string> projection;
		std::optional<bool> enableLowLatency;
		std::optional<std::string> latencyPreference;
		std::optional<bool> enableAutoStart;
		std::optional<bool> enableAutoStop;
	};
	std::optional<ContentDetails> contentDetails;

	struct Statistics {
		std::optional<std::uint64_t> totalChatCount;
	};
	std::optional<Statistics> statistics;

	struct MonetizationDetails {
		struct CuepointSchedule {
			std::optional<bool> enabled;
			std::optional<std::string> pauseAdsUntil;
			std::optional<std::string> scheduleStrategy;
			std::optional<std::uint32_t> repeatIntervalSecs;
		};
		std::optional<CuepointSchedule> cuepointSchedule;
	};
	std::optional<MonetizationDetails> monetizationDetails;
};

void to_json(nlohmann::json &j, const YouTubeLiveBroadcast &p);
void from_json(const nlohmann::json &j, YouTubeLiveBroadcast &p);

struct InsertingYouTubeLiveBroadcast {
	struct Snippet {
		std::string title;
		std::optional<std::string> description;
		std::string scheduledStartTime;
		std::optional<std::string> scheduledEndTime;
	} snippet;

	struct Status {
		std::string privacyStatus = "private";
		std::optional<bool> selfDeclaredMadeForKids;
	} status;

	struct ContentDetails {
		std::optional<bool> enableAutoStart;
		std::optional<bool> enableAutoStop;
		std::optional<bool> enableClosedCaptions;
		std::optional<bool> enableDvr;
		std::optional<bool> enableEmbed;
		std::optional<bool> recordFromStart;
		std::optional<std::string> latencyPreference;
		struct MonitorStream {
			std::optional<bool> enableMonitorStream;
			std::optional<uint32_t> broadcastStreamDelayMs;
		} monitorStream;
	} contentDetails;
};

void to_json(nlohmann::json &j, const InsertingYouTubeLiveBroadcast &p);
void from_json(const nlohmann::json &j, InsertingYouTubeLiveBroadcast &p);

struct UpdatingYouTubeLiveBroadcast {
	std::string id;
	struct Snippet {
		std::optional<std::string> title;
		std::optional<std::string> description;
		std::string scheduledStartTime;
		std::optional<std::string> scheduledEndTime;
	} snippet;

	struct Status {
		std::optional<std::string> privacyStatus;
	} status;

	struct ContentDetails {
		struct MonitorStream {
			bool enableMonitorStream = false;
			std::optional<uint32_t> broadcastStreamDelayMs;
		} monitorStream;
		std::optional<bool> enableAutoStart;
		std::optional<bool> enableAutoStop;
		std::optional<bool> enableClosedCaptions;
		std::optional<bool> enableDvr;
		std::optional<bool> enableEmbed;
		std::optional<bool> recordFromStart;
	} contentDetails;

	struct MonetizationDetails {
		struct CuepointSchedule {
			std::optional<std::string> pauseAdsUntil;
		} cuepointSchedule;
	} monetizationDetails;
};

void to_json(nlohmann::json &j, const UpdatingYouTubeLiveBroadcast &p);
void from_json(const nlohmann::json &j, UpdatingYouTubeLiveBroadcast &p);

} // namespace KaitoTokyo::YouTubeApi
