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

#include "YouTubeTypes.hpp"

#include <nlohmann/json.hpp>

namespace KaitoTokyo::YouTubeApi {

void to_json(nlohmann::json &j, const YouTubeLiveStream &p)
{
	j = nlohmann::json{{"kind", p.kind}, {"etag", p.etag}, {"id", p.id}};

	// snippet
	nlohmann::json snippetJson = {{"publishedAt", p.snippet.publishedAt},
				      {"channelId", p.snippet.channelId},
				      {"title", p.snippet.title},
				      {"description", p.snippet.description}};
	if (p.snippet.isDefaultStream.has_value()) {
		snippetJson["isDefaultStream"] = p.snippet.isDefaultStream.value();
	}
	j["snippet"] = std::move(snippetJson);

	// cdn
	nlohmann::json cdnJson = {{"ingestionType", p.cdn.ingestionType},
				  {"ingestionInfo",
				   {{"streamName", p.cdn.ingestionInfo.streamName},
				    {"ingestionAddress", p.cdn.ingestionInfo.ingestionAddress},
				    {"backupIngestionAddress", p.cdn.ingestionInfo.backupIngestionAddress}}},
				  {"resolution", p.cdn.resolution},
				  {"frameRate", p.cdn.frameRate}};
	j["cdn"] = std::move(cdnJson);

	// status (optional)
	bool hasStatus = p.status.has_value() &&
			 (!p.status->streamStatus.empty() || !p.status->healthStatus.status.empty() ||
			  p.status->healthStatus.lastUpdateTimeSeconds.has_value() ||
			  !p.status->healthStatus.configurationIssues.empty());
	if (hasStatus) {
		nlohmann::json healthStatusJson = {{"status", p.status->healthStatus.status},
						   {"configurationIssues", nlohmann::json::array()}};
		if (p.status->healthStatus.lastUpdateTimeSeconds.has_value()) {
			healthStatusJson["lastUpdateTimeSeconds"] =
				p.status->healthStatus.lastUpdateTimeSeconds.value();
		}
		if (!p.status->healthStatus.configurationIssues.empty()) {
			auto &arr = healthStatusJson["configurationIssues"];
			for (const auto &issue : p.status->healthStatus.configurationIssues) {
				arr.push_back({{"type", issue.type},
					       {"severity", issue.severity},
					       {"reason", issue.reason},
					       {"description", issue.description}});
			}
		}
		nlohmann::json statusJson = {{"streamStatus", p.status->streamStatus},
					     {"healthStatus", std::move(healthStatusJson)}};
		j["status"] = std::move(statusJson);
	}

	// contentDetails (optional)
	bool hasContentDetails =
		p.contentDetails.has_value() &&
		(!p.contentDetails->closedCaptionsIngestionUrl.empty() || p.contentDetails->isReusable.has_value());
	if (hasContentDetails) {
		nlohmann::json contentDetailsJson = {
			{"closedCaptionsIngestionUrl", p.contentDetails->closedCaptionsIngestionUrl}};
		if (p.contentDetails->isReusable.has_value()) {
			contentDetailsJson["isReusable"] = p.contentDetails->isReusable.value();
		}
		j["contentDetails"] = std::move(contentDetailsJson);
	}
}

void from_json(const nlohmann::json &j, YouTubeLiveStream &p)
{
	// Required fields
	j.at("kind").get_to(p.kind);
	j.at("etag").get_to(p.etag);
	j.at("id").get_to(p.id);

	// snippet (optional)
	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		snippet.at("publishedAt").get_to(p.snippet.publishedAt);
		snippet.at("channelId").get_to(p.snippet.channelId);
		snippet.at("title").get_to(p.snippet.title);
		snippet.at("description").get_to(p.snippet.description);
		if (snippet.contains("isDefaultStream")) {
			p.snippet.isDefaultStream = snippet.at("isDefaultStream").get<bool>();
		} else {
			p.snippet.isDefaultStream = std::nullopt;
		}
	} else {
		p.snippet.publishedAt.clear();
		p.snippet.channelId.clear();
		p.snippet.title.clear();
		p.snippet.description.clear();
		p.snippet.isDefaultStream = std::nullopt;
	}

	// cdn (optional)
	if (j.contains("cdn")) {
		const auto &cdn = j.at("cdn");
		cdn.at("ingestionType").get_to(p.cdn.ingestionType);
		const auto &ingestionInfo = cdn.at("ingestionInfo");
		ingestionInfo.at("streamName").get_to(p.cdn.ingestionInfo.streamName);
		ingestionInfo.at("ingestionAddress").get_to(p.cdn.ingestionInfo.ingestionAddress);
		ingestionInfo.at("backupIngestionAddress").get_to(p.cdn.ingestionInfo.backupIngestionAddress);
		cdn.at("resolution").get_to(p.cdn.resolution);
		cdn.at("frameRate").get_to(p.cdn.frameRate);
	} else {
		p.cdn.ingestionType.clear();
		p.cdn.ingestionInfo.streamName.clear();
		p.cdn.ingestionInfo.ingestionAddress.clear();
		p.cdn.ingestionInfo.backupIngestionAddress.clear();
		p.cdn.resolution.clear();
		p.cdn.frameRate.clear();
	}

	// status (optional)
	if (j.contains("status")) {
		YouTubeLiveStream::Status statusObj;
		const auto &status = j.at("status");
		status.at("streamStatus").get_to(statusObj.streamStatus);
		const auto &healthStatus = status.at("healthStatus");
		healthStatus.at("status").get_to(statusObj.healthStatus.status);
		if (healthStatus.contains("lastUpdateTimeSeconds")) {
			statusObj.healthStatus.lastUpdateTimeSeconds =
				healthStatus.at("lastUpdateTimeSeconds").get<std::uint64_t>();
		} else {
			statusObj.healthStatus.lastUpdateTimeSeconds = std::nullopt;
		}
		statusObj.healthStatus.configurationIssues.clear();
		if (healthStatus.contains("configurationIssues")) {
			for (const auto &issue : healthStatus.at("configurationIssues")) {
				YouTubeLiveStream::Status::HealthStatus::ConfigurationIssue ci;
				issue.at("type").get_to(ci.type);
				issue.at("severity").get_to(ci.severity);
				issue.at("reason").get_to(ci.reason);
				issue.at("description").get_to(ci.description);
				statusObj.healthStatus.configurationIssues.push_back(std::move(ci));
			}
		}
		p.status = std::move(statusObj);
	} else {
		p.status = std::nullopt;
	}

	// contentDetails (optional)
	if (j.contains("contentDetails")) {
		YouTubeLiveStream::ContentDetails contentDetailsObj;
		const auto &contentDetails = j.at("contentDetails");
		contentDetails.at("closedCaptionsIngestionUrl").get_to(contentDetailsObj.closedCaptionsIngestionUrl);
		if (contentDetails.contains("isReusable")) {
			contentDetailsObj.isReusable = contentDetails.at("isReusable").get<bool>();
		} else {
			contentDetailsObj.isReusable = std::nullopt;
		}
		p.contentDetails = std::move(contentDetailsObj);
	} else {
		p.contentDetails = std::nullopt;
	}
}

void to_json(nlohmann::json &j, const YouTubeLiveBroadcastThumbnail &p)
{
	j = nlohmann::json{{"url", p.url}};
	if (p.width)
		j["width"] = *p.width;
	if (p.height)
		j["height"] = *p.height;
}
void from_json(const nlohmann::json &j, YouTubeLiveBroadcastThumbnail &p)
{
	j.at("url").get_to(p.url);
	if (j.contains("width"))
		j.at("width").get_to(p.width.emplace());
	if (j.contains("height"))
		j.at("height").get_to(p.height.emplace());
}

void to_json(nlohmann::json &j, const YouTubeLiveBroadcast &p)
{
	j = nlohmann::json{};
	if (p.kind) {
		j["kind"] = *p.kind;
	}
	if (p.etag) {
		j["etag"] = *p.etag;
	}
	if (p.id) {
		j["id"] = *p.id;
	}
	if (p.snippet) {
		nlohmann::json snippetJson;
		if (p.snippet->publishedAt) {
			snippetJson["publishedAt"] = *p.snippet->publishedAt;
		}
		if (p.snippet->channelId) {
			snippetJson["channelId"] = *p.snippet->channelId;
		}
		if (p.snippet->title) {
			snippetJson["title"] = *p.snippet->title;
		}
		if (p.snippet->description) {
			snippetJson["description"] = *p.snippet->description;
		}
		if (p.snippet->thumbnails) {
			snippetJson["thumbnails"] = *p.snippet->thumbnails;
		}
		if (p.snippet->scheduledStartTime) {
			snippetJson["scheduledStartTime"] = *p.snippet->scheduledStartTime;
		}
		if (p.snippet->scheduledEndTime) {
			snippetJson["scheduledEndTime"] = *p.snippet->scheduledEndTime;
		}
		if (p.snippet->actualStartTime) {
			snippetJson["actualStartTime"] = *p.snippet->actualStartTime;
		}
		if (p.snippet->actualEndTime) {
			snippetJson["actualEndTime"] = *p.snippet->actualEndTime;
		}
		if (p.snippet->isDefaultBroadcast) {
			snippetJson["isDefaultBroadcast"] = *p.snippet->isDefaultBroadcast;
		}
		if (p.snippet->liveChatId) {
			snippetJson["liveChatId"] = *p.snippet->liveChatId;
		}
		j["snippet"] = std::move(snippetJson);
	}
	if (p.status) {
		nlohmann::json statusJson;
		if (p.status->lifeCycleStatus) {
			statusJson["lifeCycleStatus"] = *p.status->lifeCycleStatus;
		}
		if (p.status->privacyStatus) {
			statusJson["privacyStatus"] = *p.status->privacyStatus;
		}
		if (p.status->recordingStatus) {
			statusJson["recordingStatus"] = *p.status->recordingStatus;
		}
		if (p.status->madeForKids) {
			statusJson["madeForKids"] = *p.status->madeForKids;
		}
		if (p.status->selfDeclaredMadeForKids) {
			statusJson["selfDeclaredMadeForKids"] = *p.status->selfDeclaredMadeForKids;
		}
		j["status"] = std::move(statusJson);
	}
	if (p.contentDetails) {
		nlohmann::json contentDetailsJson;
		if (p.contentDetails->boundStreamId) {
			contentDetailsJson["boundStreamId"] = *p.contentDetails->boundStreamId;
		}
		if (p.contentDetails->boundStreamLastUpdateTimeMs) {
			contentDetailsJson["boundStreamLastUpdateTimeMs"] =
				*p.contentDetails->boundStreamLastUpdateTimeMs;
		}
		if (p.contentDetails->monitorStream) {
			nlohmann::json monitorStreamJson;
			if (p.contentDetails->monitorStream->enableMonitorStream) {
				monitorStreamJson["enableMonitorStream"] =
					*p.contentDetails->monitorStream->enableMonitorStream;
			}
			if (p.contentDetails->monitorStream->broadcastStreamDelayMs) {
				monitorStreamJson["broadcastStreamDelayMs"] =
					*p.contentDetails->monitorStream->broadcastStreamDelayMs;
			}
			if (p.contentDetails->monitorStream->embedHtml) {
				monitorStreamJson["embedHtml"] = *p.contentDetails->monitorStream->embedHtml;
			}
			if (!monitorStreamJson.empty()) {
				contentDetailsJson["monitorStream"] = std::move(monitorStreamJson);
			}
		}
		if (p.contentDetails->enableEmbed) {
			contentDetailsJson["enableEmbed"] = *p.contentDetails->enableEmbed;
		}
		if (p.contentDetails->enableDvr) {
			contentDetailsJson["enableDvr"] = *p.contentDetails->enableDvr;
		}
		if (p.contentDetails->recordFromStart) {
			contentDetailsJson["recordFromStart"] = *p.contentDetails->recordFromStart;
		}
		if (p.contentDetails->enableClosedCaptions) {
			contentDetailsJson["enableClosedCaptions"] = *p.contentDetails->enableClosedCaptions;
		}
		if (p.contentDetails->closedCaptionsType) {
			contentDetailsJson["closedCaptionsType"] = *p.contentDetails->closedCaptionsType;
		}
		if (p.contentDetails->projection) {
			contentDetailsJson["projection"] = *p.contentDetails->projection;
		}
		if (p.contentDetails->enableLowLatency) {
			contentDetailsJson["enableLowLatency"] = *p.contentDetails->enableLowLatency;
		}
		if (p.contentDetails->latencyPreference) {
			contentDetailsJson["latencyPreference"] = *p.contentDetails->latencyPreference;
		}
		if (p.contentDetails->enableAutoStart) {
			contentDetailsJson["enableAutoStart"] = *p.contentDetails->enableAutoStart;
		}
		if (p.contentDetails->enableAutoStop) {
			contentDetailsJson["enableAutoStop"] = *p.contentDetails->enableAutoStop;
		}
		j["contentDetails"] = std::move(contentDetailsJson);
	}
	if (p.statistics) {
		nlohmann::json statisticsJson;
		if (p.statistics->totalChatCount) {
			statisticsJson["totalChatCount"] = *p.statistics->totalChatCount;
		}
		j["statistics"] = std::move(statisticsJson);
	}
	if (p.monetizationDetails) {
		nlohmann::json monetizationDetailsJson;
		if (p.monetizationDetails->cuepointSchedule) {
			nlohmann::json cuepointScheduleJson;
			if (p.monetizationDetails->cuepointSchedule->enabled) {
				cuepointScheduleJson["enabled"] = *p.monetizationDetails->cuepointSchedule->enabled;
			}
			if (p.monetizationDetails->cuepointSchedule->pauseAdsUntil) {
				cuepointScheduleJson["pauseAdsUntil"] =
					*p.monetizationDetails->cuepointSchedule->pauseAdsUntil;
			}
			if (p.monetizationDetails->cuepointSchedule->scheduleStrategy) {
				cuepointScheduleJson["scheduleStrategy"] =
					*p.monetizationDetails->cuepointSchedule->scheduleStrategy;
			}
			if (p.monetizationDetails->cuepointSchedule->repeatIntervalSecs) {
				cuepointScheduleJson["repeatIntervalSecs"] =
					*p.monetizationDetails->cuepointSchedule->repeatIntervalSecs;
			}
			if (!cuepointScheduleJson.empty()) {
				monetizationDetailsJson["cuepointSchedule"] = std::move(cuepointScheduleJson);
			}
		}
		j["monetizationDetails"] = std::move(monetizationDetailsJson);
	}
}

void from_json(const nlohmann::json &j, YouTubeLiveBroadcast &p)
{
	p = YouTubeLiveBroadcast{};
	if (j.contains("kind"))
		p.kind = j.at("kind").get<std::string>();
	if (j.contains("etag"))
		p.etag = j.at("etag").get<std::string>();
	if (j.contains("id"))
		p.id = j.at("id").get<std::string>();
	if (j.contains("snippet")) {
		YouTubeLiveBroadcast::Snippet snippetObj;
		const auto &s = j.at("snippet");
		if (s.contains("publishedAt"))
			snippetObj.publishedAt = s.at("publishedAt").get<std::string>();
		if (s.contains("channelId"))
			snippetObj.channelId = s.at("channelId").get<std::string>();
		if (s.contains("title"))
			snippetObj.title = s.at("title").get<std::string>();
		if (s.contains("description"))
			snippetObj.description = s.at("description").get<std::string>();
		if (s.contains("thumbnails"))
			snippetObj.thumbnails =
				s.at("thumbnails").get<std::unordered_map<std::string, YouTubeLiveBroadcastThumbnail>>();
		if (s.contains("scheduledStartTime"))
			snippetObj.scheduledStartTime = s.at("scheduledStartTime").get<std::string>();
		if (s.contains("scheduledEndTime"))
			snippetObj.scheduledEndTime = s.at("scheduledEndTime").get<std::string>();
		if (s.contains("actualStartTime"))
			snippetObj.actualStartTime = s.at("actualStartTime").get<std::string>();
		if (s.contains("actualEndTime"))
			snippetObj.actualEndTime = s.at("actualEndTime").get<std::string>();
		if (s.contains("isDefaultBroadcast"))
			snippetObj.isDefaultBroadcast = s.at("isDefaultBroadcast").get<bool>();
		if (s.contains("liveChatId"))
			snippetObj.liveChatId = s.at("liveChatId").get<std::string>();
		p.snippet = std::move(snippetObj);
	} else {
		p.snippet = std::nullopt;
	}
	if (j.contains("status")) {
		YouTubeLiveBroadcast::Status statusObj;
		const auto &s = j.at("status");
		if (s.contains("lifeCycleStatus"))
			statusObj.lifeCycleStatus = s.at("lifeCycleStatus").get<std::string>();
		if (s.contains("privacyStatus"))
			statusObj.privacyStatus = s.at("privacyStatus").get<std::string>();
		if (s.contains("recordingStatus"))
			statusObj.recordingStatus = s.at("recordingStatus").get<std::string>();
		if (s.contains("madeForKids"))
			statusObj.madeForKids = s.at("madeForKids").get<bool>();
		if (s.contains("selfDeclaredMadeForKids"))
			statusObj.selfDeclaredMadeForKids = s.at("selfDeclaredMadeForKids").get<bool>();
		p.status = std::move(statusObj);
	} else {
		p.status = std::nullopt;
	}
	if (j.contains("contentDetails")) {
		YouTubeLiveBroadcast::ContentDetails contentDetailsObj;
		const auto &c = j.at("contentDetails");
		if (c.contains("boundStreamId"))
			contentDetailsObj.boundStreamId = c.at("boundStreamId").get<std::string>();
		if (c.contains("boundStreamLastUpdateTimeMs"))
			contentDetailsObj.boundStreamLastUpdateTimeMs =
				c.at("boundStreamLastUpdateTimeMs").get<std::string>();
		if (c.contains("monitorStream")) {
			YouTubeLiveBroadcast::ContentDetails::MonitorStream monitorStreamObj;
			const auto &m = c.at("monitorStream");
			if (m.contains("enableMonitorStream"))
				monitorStreamObj.enableMonitorStream = m.at("enableMonitorStream").get<bool>();
			if (m.contains("broadcastStreamDelayMs"))
				monitorStreamObj.broadcastStreamDelayMs =
					m.at("broadcastStreamDelayMs").get<std::uint32_t>();
			if (m.contains("embedHtml"))
				monitorStreamObj.embedHtml = m.at("embedHtml").get<std::string>();
			contentDetailsObj.monitorStream = std::move(monitorStreamObj);
		} else {
			contentDetailsObj.monitorStream = std::nullopt;
		}
		if (c.contains("enableEmbed"))
			contentDetailsObj.enableEmbed = c.at("enableEmbed").get<bool>();
		if (c.contains("enableDvr"))
			contentDetailsObj.enableDvr = c.at("enableDvr").get<bool>();
		if (c.contains("recordFromStart"))
			contentDetailsObj.recordFromStart = c.at("recordFromStart").get<bool>();
		if (c.contains("enableClosedCaptions"))
			contentDetailsObj.enableClosedCaptions = c.at("enableClosedCaptions").get<bool>();
		if (c.contains("closedCaptionsType"))
			contentDetailsObj.closedCaptionsType = c.at("closedCaptionsType").get<std::string>();
		if (c.contains("projection"))
			contentDetailsObj.projection = c.at("projection").get<std::string>();
		if (c.contains("enableLowLatency"))
			contentDetailsObj.enableLowLatency = c.at("enableLowLatency").get<bool>();
		if (c.contains("latencyPreference"))
			contentDetailsObj.latencyPreference = c.at("latencyPreference").get<std::string>();
		if (c.contains("enableAutoStart"))
			contentDetailsObj.enableAutoStart = c.at("enableAutoStart").get<bool>();
		if (c.contains("enableAutoStop"))
			contentDetailsObj.enableAutoStop = c.at("enableAutoStop").get<bool>();
		p.contentDetails = std::move(contentDetailsObj);
	} else {
		p.contentDetails = std::nullopt;
	}
	if (j.contains("statistics")) {
		YouTubeLiveBroadcast::Statistics statisticsObj;
		const auto &s = j.at("statistics");
		if (s.contains("totalChatCount"))
			statisticsObj.totalChatCount = s.at("totalChatCount").get<std::uint64_t>();
		p.statistics = std::move(statisticsObj);
	} else {
		p.statistics = std::nullopt;
	}
	if (j.contains("monetizationDetails")) {
		YouTubeLiveBroadcast::MonetizationDetails monetizationDetailsObj;
		const auto &m = j.at("monetizationDetails");
		if (m.contains("cuepointSchedule")) {
			YouTubeLiveBroadcast::MonetizationDetails::CuepointSchedule cuepointScheduleObj;
			const auto &c = m.at("cuepointSchedule");
			if (c.contains("enabled"))
				cuepointScheduleObj.enabled = c.at("enabled").get<bool>();
			if (c.contains("pauseAdsUntil"))
				cuepointScheduleObj.pauseAdsUntil = c.at("pauseAdsUntil").get<std::string>();
			if (c.contains("scheduleStrategy"))
				cuepointScheduleObj.scheduleStrategy = c.at("scheduleStrategy").get<std::string>();
			if (c.contains("repeatIntervalSecs"))
				cuepointScheduleObj.repeatIntervalSecs =
					c.at("repeatIntervalSecs").get<std::uint32_t>();
			monetizationDetailsObj.cuepointSchedule = std::move(cuepointScheduleObj);
		} else {
			monetizationDetailsObj.cuepointSchedule = std::nullopt;
		}
		p.monetizationDetails = std::move(monetizationDetailsObj);
	} else {
		p.monetizationDetails = std::nullopt;
	}
}

void to_json(nlohmann::json &j, const InsertingYouTubeLiveBroadcast &p)
{
	j = nlohmann::json{};
	// snippet
	nlohmann::json snippetJson;
	snippetJson["title"] = p.snippet.title;
	snippetJson["scheduledStartTime"] = p.snippet.scheduledStartTime;
	if (p.snippet.description) {
		snippetJson["description"] = *p.snippet.description;
	}
	if (p.snippet.scheduledEndTime) {
		snippetJson["scheduledEndTime"] = *p.snippet.scheduledEndTime;
	}
	j["snippet"] = std::move(snippetJson);

	// status
	nlohmann::json statusJson;
	statusJson["privacyStatus"] = p.status.privacyStatus;
	if (p.status.selfDeclaredMadeForKids) {
		statusJson["selfDeclaredMadeForKids"] = *p.status.selfDeclaredMadeForKids;
	}
	j["status"] = std::move(statusJson);

	// contentDetails
	nlohmann::json contentDetailsJson;
	if (p.contentDetails.enableAutoStart) {
		contentDetailsJson["enableAutoStart"] = *p.contentDetails.enableAutoStart;
	}
	if (p.contentDetails.enableAutoStop) {
		contentDetailsJson["enableAutoStop"] = *p.contentDetails.enableAutoStop;
	}
	if (p.contentDetails.enableClosedCaptions) {
		contentDetailsJson["enableClosedCaptions"] = *p.contentDetails.enableClosedCaptions;
	}
	if (p.contentDetails.enableDvr) {
		contentDetailsJson["enableDvr"] = *p.contentDetails.enableDvr;
	}
	if (p.contentDetails.enableEmbed) {
		contentDetailsJson["enableEmbed"] = *p.contentDetails.enableEmbed;
	}
	if (p.contentDetails.recordFromStart) {
		contentDetailsJson["recordFromStart"] = *p.contentDetails.recordFromStart;
	}
	if (p.contentDetails.latencyPreference) {
		contentDetailsJson["latencyPreference"] = *p.contentDetails.latencyPreference;
	}
	nlohmann::json monitorStreamJson;
	if (p.contentDetails.monitorStream.enableMonitorStream) {
		monitorStreamJson["enableMonitorStream"] = *p.contentDetails.monitorStream.enableMonitorStream;
	}
	if (p.contentDetails.monitorStream.broadcastStreamDelayMs) {
		monitorStreamJson["broadcastStreamDelayMs"] = *p.contentDetails.monitorStream.broadcastStreamDelayMs;
	}
	if (!monitorStreamJson.empty()) {
		contentDetailsJson["monitorStream"] = std::move(monitorStreamJson);
	}
	if (!contentDetailsJson.empty()) {
		j["contentDetails"] = std::move(contentDetailsJson);
	}
}

void from_json(const nlohmann::json &j, InsertingYouTubeLiveBroadcast &p)
{
	const auto &snippet = j.at("snippet");
	snippet.at("title").get_to(p.snippet.title);
	if (snippet.contains("description")) {
		p.snippet.description = snippet.at("description").get<std::string>();
	} else {
		p.snippet.description = std::nullopt;
	}
	snippet.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);
	if (snippet.contains("scheduledEndTime")) {
		p.snippet.scheduledEndTime = snippet.at("scheduledEndTime").get<std::string>();
	} else {
		p.snippet.scheduledEndTime = std::nullopt;
	}

	const auto &status = j.at("status");
	status.at("privacyStatus").get_to(p.status.privacyStatus);
	if (status.contains("selfDeclaredMadeForKids")) {
		p.status.selfDeclaredMadeForKids = status.at("selfDeclaredMadeForKids").get<bool>();
	} else {
		p.status.selfDeclaredMadeForKids = std::nullopt;
	}

	const auto &contentDetails = j.at("contentDetails");
	if (contentDetails.contains("enableAutoStart")) {
		p.contentDetails.enableAutoStart = contentDetails.at("enableAutoStart").get<bool>();
	} else {
		p.contentDetails.enableAutoStart = std::nullopt;
	}
	if (contentDetails.contains("enableAutoStop")) {
		p.contentDetails.enableAutoStop = contentDetails.at("enableAutoStop").get<bool>();
	} else {
		p.contentDetails.enableAutoStop = std::nullopt;
	}
	if (contentDetails.contains("enableClosedCaptions")) {
		p.contentDetails.enableClosedCaptions = contentDetails.at("enableClosedCaptions").get<bool>();
	} else {
		p.contentDetails.enableClosedCaptions = std::nullopt;
	}
	if (contentDetails.contains("enableDvr")) {
		p.contentDetails.enableDvr = contentDetails.at("enableDvr").get<bool>();
	} else {
		p.contentDetails.enableDvr = std::nullopt;
	}
	if (contentDetails.contains("enableEmbed")) {
		p.contentDetails.enableEmbed = contentDetails.at("enableEmbed").get<bool>();
	} else {
		p.contentDetails.enableEmbed = std::nullopt;
	}
	if (contentDetails.contains("recordFromStart")) {
		p.contentDetails.recordFromStart = contentDetails.at("recordFromStart").get<bool>();
	} else {
		p.contentDetails.recordFromStart = std::nullopt;
	}
	if (contentDetails.contains("latencyPreference")) {
		p.contentDetails.latencyPreference = contentDetails.at("latencyPreference").get<std::string>();
	} else {
		p.contentDetails.latencyPreference = std::nullopt;
	}
	if (contentDetails.contains("monitorStream")) {
		const auto &monitorStream = contentDetails.at("monitorStream");
		if (monitorStream.contains("enableMonitorStream")) {
			p.contentDetails.monitorStream.enableMonitorStream =
				monitorStream.at("enableMonitorStream").get<bool>();
		} else {
			p.contentDetails.monitorStream.enableMonitorStream = std::nullopt;
		}
		if (monitorStream.contains("broadcastStreamDelayMs")) {
			p.contentDetails.monitorStream.broadcastStreamDelayMs =
				monitorStream.at("broadcastStreamDelayMs").get<uint32_t>();
		} else {
			p.contentDetails.monitorStream.broadcastStreamDelayMs = std::nullopt;
		}
	} else {
		p.contentDetails.monitorStream.enableMonitorStream = std::nullopt;
		p.contentDetails.monitorStream.broadcastStreamDelayMs = std::nullopt;
	}
}

void from_json(const nlohmann::json &j, UpdatingYouTubeLiveBroadcast &p)
{
	if (j.contains("id")) {
		j.at("id").get_to(p.id);
	}

	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		if (snippet.contains("title")) {
			snippet.at("title").get_to(p.snippet.title);
		} else {
			p.snippet.title = std::nullopt;
		}
		if (snippet.contains("description")) {
			snippet.at("description").get_to(p.snippet.description);
		} else {
			p.snippet.description = std::nullopt;
		}
		snippet.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);
		if (snippet.contains("scheduledEndTime")) {
			snippet.at("scheduledEndTime").get_to(p.snippet.scheduledEndTime.emplace());
		} else {
			p.snippet.scheduledEndTime = std::nullopt;
		}
	}

	if (j.contains("status")) {
		const auto &status = j.at("status");
		if (status.contains("privacyStatus")) {
			status.at("privacyStatus").get_to(p.status.privacyStatus);
		} else {
			p.status.privacyStatus = std::nullopt;
		}
	}

	if (j.contains("contentDetails")) {
		const auto &cd = j.at("contentDetails");
		if (cd.contains("monitorStream")) {
			const auto &ms = cd.at("monitorStream");
			ms.at("enableMonitorStream").get_to(p.contentDetails.monitorStream.enableMonitorStream);
			if (ms.contains("broadcastStreamDelayMs")) {
				ms.at("broadcastStreamDelayMs")
					.get_to(p.contentDetails.monitorStream.broadcastStreamDelayMs.emplace());
			} else {
				p.contentDetails.monitorStream.broadcastStreamDelayMs = std::nullopt;
			}
		} else {
			p.contentDetails.monitorStream.enableMonitorStream = false;
			p.contentDetails.monitorStream.broadcastStreamDelayMs = std::nullopt;
		}
		if (cd.contains("enableAutoStart")) {
			cd.at("enableAutoStart").get_to(p.contentDetails.enableAutoStart);
		} else {
			p.contentDetails.enableAutoStart = std::nullopt;
		}
		if (cd.contains("enableAutoStop")) {
			cd.at("enableAutoStop").get_to(p.contentDetails.enableAutoStop);
		} else {
			p.contentDetails.enableAutoStop = std::nullopt;
		}
		if (cd.contains("enableClosedCaptions")) {
			cd.at("enableClosedCaptions").get_to(p.contentDetails.enableClosedCaptions);
		} else {
			p.contentDetails.enableClosedCaptions = std::nullopt;
		}
		if (cd.contains("enableDvr")) {
			cd.at("enableDvr").get_to(p.contentDetails.enableDvr);
		} else {
			p.contentDetails.enableDvr = std::nullopt;
		}
		if (cd.contains("enableEmbed")) {
			cd.at("enableEmbed").get_to(p.contentDetails.enableEmbed);
		} else {
			p.contentDetails.enableEmbed = std::nullopt;
		}
		if (cd.contains("recordFromStart")) {
			cd.at("recordFromStart").get_to(p.contentDetails.recordFromStart);
		} else {
			p.contentDetails.recordFromStart = std::nullopt;
		}
	}

	if (j.contains("monetizationDetails")) {
		const auto &md = j.at("monetizationDetails");
		if (md.contains("cuepointSchedule")) {
			const auto &cs = md.at("cuepointSchedule");
			if (cs.contains("pauseAdsUntil")) {
				cs.at("pauseAdsUntil").get_to(p.monetizationDetails.cuepointSchedule.pauseAdsUntil);
			} else {
				p.monetizationDetails.cuepointSchedule.pauseAdsUntil = std::nullopt;
			}
		} else {
			p.monetizationDetails.cuepointSchedule.pauseAdsUntil = std::nullopt;
		}
	}
}

void to_json(nlohmann::json &j, const UpdatingYouTubeLiveBroadcast &p)
{
	j = nlohmann::json{};
	j["id"] = p.id;

	// snippet
	nlohmann::json snippetJson;
	if (p.snippet.title) {
		snippetJson["title"] = *p.snippet.title;
	}
	if (p.snippet.description) {
		snippetJson["description"] = *p.snippet.description;
	}
	snippetJson["scheduledStartTime"] = p.snippet.scheduledStartTime;
	if (p.snippet.scheduledEndTime) {
		snippetJson["scheduledEndTime"] = *p.snippet.scheduledEndTime;
	}
	j["snippet"] = std::move(snippetJson);

	// status
	nlohmann::json statusJson;
	if (p.status.privacyStatus) {
		statusJson["privacyStatus"] = *p.status.privacyStatus;
	}
	if (!statusJson.empty()) {
		j["status"] = std::move(statusJson);
	}

	// contentDetails
	nlohmann::json contentDetailsJson;
	nlohmann::json monitorStreamJson;
	monitorStreamJson["enableMonitorStream"] = p.contentDetails.monitorStream.enableMonitorStream;
	if (p.contentDetails.monitorStream.broadcastStreamDelayMs) {
		monitorStreamJson["broadcastStreamDelayMs"] = *p.contentDetails.monitorStream.broadcastStreamDelayMs;
	}
	contentDetailsJson["monitorStream"] = std::move(monitorStreamJson);
	if (p.contentDetails.enableAutoStart) {
		contentDetailsJson["enableAutoStart"] = *p.contentDetails.enableAutoStart;
	}
	if (p.contentDetails.enableAutoStop) {
		contentDetailsJson["enableAutoStop"] = *p.contentDetails.enableAutoStop;
	}
	if (p.contentDetails.enableClosedCaptions) {
		contentDetailsJson["enableClosedCaptions"] = *p.contentDetails.enableClosedCaptions;
	}
	if (p.contentDetails.enableDvr) {
		contentDetailsJson["enableDvr"] = *p.contentDetails.enableDvr;
	}
	if (p.contentDetails.enableEmbed) {
		contentDetailsJson["enableEmbed"] = *p.contentDetails.enableEmbed;
	}
	if (p.contentDetails.recordFromStart) {
		contentDetailsJson["recordFromStart"] = *p.contentDetails.recordFromStart;
	}
	j["contentDetails"] = std::move(contentDetailsJson);

	// monetizationDetails
	nlohmann::json monetizationDetailsJson;
	nlohmann::json cuepointScheduleJson;
	if (p.monetizationDetails.cuepointSchedule.pauseAdsUntil) {
		cuepointScheduleJson["pauseAdsUntil"] = *p.monetizationDetails.cuepointSchedule.pauseAdsUntil;
	}
	if (!cuepointScheduleJson.empty()) {
		monetizationDetailsJson["cuepointSchedule"] = std::move(cuepointScheduleJson);
	}
	if (!monetizationDetailsJson.empty()) {
		j["monetizationDetails"] = std::move(monetizationDetailsJson);
	}
}

} // namespace KaitoTokyo::YouTubeApi
