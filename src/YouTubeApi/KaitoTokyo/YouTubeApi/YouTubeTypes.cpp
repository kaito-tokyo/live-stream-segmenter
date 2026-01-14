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

void to_json(nlohmann::json &j, const YouTubeApiError &p)
{
	j = nlohmann::json{};
	j["code"] = p.code;
	j["message"] = p.message;
	j["errors"] = nlohmann::json::array();
	for (const auto &err : p.errors) {
		nlohmann::json errJson;
		errJson["domain"] = err.domain;
		errJson["reason"] = err.reason;
		errJson["message"] = err.message;
		j["errors"].push_back(std::move(errJson));
	}
}

void from_json(const nlohmann::json &j, YouTubeApiError &p)
{
	j.at("code").get_to(p.code);
	j.at("message").get_to(p.message);
	p.errors.clear();
	if (j.contains("errors")) {
		for (const auto &errJson : j.at("errors")) {
			YouTubeApiError::ErrorDetail err;
			errJson.at("domain").get_to(err.domain);
			errJson.at("reason").get_to(err.reason);
			errJson.at("message").get_to(err.message);
			p.errors.push_back(std::move(err));
		}
	}
}

void to_json(nlohmann::json &j, const YouTubeLiveStream &p)
{

	j = nlohmann::json{};
	if (p.kind)
		j["kind"] = *p.kind;
	if (p.etag)
		j["etag"] = *p.etag;
	if (p.id)
		j["id"] = *p.id;

	// snippet
	if (p.snippet) {
		nlohmann::json snippetJson;
		if (p.snippet->publishedAt)
			snippetJson["publishedAt"] = *p.snippet->publishedAt;
		if (p.snippet->channelId)
			snippetJson["channelId"] = *p.snippet->channelId;
		if (p.snippet->title)
			snippetJson["title"] = *p.snippet->title;
		if (p.snippet->description)
			snippetJson["description"] = *p.snippet->description;
		if (p.snippet->isDefaultStream)
			snippetJson["isDefaultStream"] = *p.snippet->isDefaultStream;
		if (!snippetJson.empty())
			j["snippet"] = std::move(snippetJson);
	}

	// cdn
	if (p.cdn) {
		nlohmann::json cdnJson;
		if (p.cdn->ingestionType)
			cdnJson["ingestionType"] = *p.cdn->ingestionType;
		if (p.cdn->ingestionInfo) {
			nlohmann::json ingestionInfoJson;
			if (p.cdn->ingestionInfo->streamName)
				ingestionInfoJson["streamName"] = *p.cdn->ingestionInfo->streamName;
			if (p.cdn->ingestionInfo->ingestionAddress)
				ingestionInfoJson["ingestionAddress"] = *p.cdn->ingestionInfo->ingestionAddress;
			if (p.cdn->ingestionInfo->backupIngestionAddress)
				ingestionInfoJson["backupIngestionAddress"] =
					*p.cdn->ingestionInfo->backupIngestionAddress;
			if (!ingestionInfoJson.empty())
				cdnJson["ingestionInfo"] = std::move(ingestionInfoJson);
		}
		if (p.cdn->resolution)
			cdnJson["resolution"] = *p.cdn->resolution;
		if (p.cdn->frameRate)
			cdnJson["frameRate"] = *p.cdn->frameRate;
		if (!cdnJson.empty())
			j["cdn"] = std::move(cdnJson);
	}

	// status
	if (p.status) {
		nlohmann::json statusJson;
		if (p.status->streamStatus)
			statusJson["streamStatus"] = *p.status->streamStatus;
		if (p.status->healthStatus) {
			nlohmann::json healthStatusJson;
			if (p.status->healthStatus->status)
				healthStatusJson["status"] = *p.status->healthStatus->status;
			if (p.status->healthStatus->lastUpdateTimeSeconds)
				healthStatusJson["lastUpdateTimeSeconds"] =
					*p.status->healthStatus->lastUpdateTimeSeconds;
			if (p.status->healthStatus->configurationIssues) {
				nlohmann::json arr = nlohmann::json::array();
				for (const auto &issue : *p.status->healthStatus->configurationIssues) {
					nlohmann::json issueJson;
					if (issue.type)
						issueJson["type"] = *issue.type;
					if (issue.severity)
						issueJson["severity"] = *issue.severity;
					if (issue.reason)
						issueJson["reason"] = *issue.reason;
					if (issue.description)
						issueJson["description"] = *issue.description;
					arr.push_back(std::move(issueJson));
				}
				healthStatusJson["configurationIssues"] = std::move(arr);
			}
			if (!healthStatusJson.empty())
				statusJson["healthStatus"] = std::move(healthStatusJson);
		}
		if (!statusJson.empty())
			j["status"] = std::move(statusJson);
	}

	// contentDetails
	if (p.contentDetails) {
		nlohmann::json contentDetailsJson;
		if (p.contentDetails->closedCaptionsIngestionUrl)
			contentDetailsJson["closedCaptionsIngestionUrl"] =
				*p.contentDetails->closedCaptionsIngestionUrl;
		if (p.contentDetails->isReusable)
			contentDetailsJson["isReusable"] = *p.contentDetails->isReusable;
		if (!contentDetailsJson.empty())
			j["contentDetails"] = std::move(contentDetailsJson);
	}
}

void from_json(const nlohmann::json &j, YouTubeLiveStream &p)
{
	// kind, etag, id
	p.kind = j.contains("kind") ? std::make_optional(j.at("kind").get<std::string>()) : std::nullopt;
	p.etag = j.contains("etag") ? std::make_optional(j.at("etag").get<std::string>()) : std::nullopt;
	p.id = j.contains("id") ? std::make_optional(j.at("id").get<std::string>()) : std::nullopt;

	// snippet
	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		YouTubeLiveStream::Snippet s;
		s.publishedAt = snippet.contains("publishedAt")
					? std::make_optional(snippet.at("publishedAt").get<std::string>())
					: std::nullopt;
		s.channelId = snippet.contains("channelId")
				      ? std::make_optional(snippet.at("channelId").get<std::string>())
				      : std::nullopt;
		s.title = snippet.contains("title") ? std::make_optional(snippet.at("title").get<std::string>())
						    : std::nullopt;
		s.description = snippet.contains("description")
					? std::make_optional(snippet.at("description").get<std::string>())
					: std::nullopt;
		s.isDefaultStream = snippet.contains("isDefaultStream")
					    ? std::make_optional(snippet.at("isDefaultStream").get<bool>())
					    : std::nullopt;
		p.snippet = std::move(s);
	} else {
		p.snippet = std::nullopt;
	}

	// cdn
	if (j.contains("cdn")) {
		const auto &cdn = j.at("cdn");
		YouTubeLiveStream::Cdn c;
		c.ingestionType = cdn.contains("ingestionType")
					  ? std::make_optional(cdn.at("ingestionType").get<std::string>())
					  : std::nullopt;
		if (cdn.contains("ingestionInfo")) {
			const auto &ingestionInfo = cdn.at("ingestionInfo");
			YouTubeLiveStream::Cdn::IngestionInfo info;
			info.streamName =
				ingestionInfo.contains("streamName")
					? std::make_optional(ingestionInfo.at("streamName").get<std::string>())
					: std::nullopt;
			info.ingestionAddress =
				ingestionInfo.contains("ingestionAddress")
					? std::make_optional(ingestionInfo.at("ingestionAddress").get<std::string>())
					: std::nullopt;
			info.backupIngestionAddress =
				ingestionInfo.contains("backupIngestionAddress")
					? std::make_optional(
						  ingestionInfo.at("backupIngestionAddress").get<std::string>())
					: std::nullopt;
			c.ingestionInfo = std::move(info);
		} else {
			c.ingestionInfo = std::nullopt;
		}
		c.resolution = cdn.contains("resolution") ? std::make_optional(cdn.at("resolution").get<std::string>())
							  : std::nullopt;
		c.frameRate = cdn.contains("frameRate") ? std::make_optional(cdn.at("frameRate").get<std::string>())
							: std::nullopt;
		p.cdn = std::move(c);
	} else {
		p.cdn = std::nullopt;
	}

	// status
	if (j.contains("status")) {
		const auto &status = j.at("status");
		YouTubeLiveStream::Status s;
		s.streamStatus = status.contains("streamStatus")
					 ? std::make_optional(status.at("streamStatus").get<std::string>())
					 : std::nullopt;
		if (status.contains("healthStatus")) {
			const auto &healthStatus = status.at("healthStatus");
			YouTubeLiveStream::Status::HealthStatus h;
			h.status = healthStatus.contains("status")
					   ? std::make_optional(healthStatus.at("status").get<std::string>())
					   : std::nullopt;
			h.lastUpdateTimeSeconds =
				healthStatus.contains("lastUpdateTimeSeconds")
					? std::make_optional(
						  healthStatus.at("lastUpdateTimeSeconds").get<std::uint64_t>())
					: std::nullopt;
			if (healthStatus.contains("configurationIssues")) {
				std::vector<YouTubeLiveStream::Status::HealthStatus::ConfigurationIssue> issues;
				for (const auto &issue : healthStatus.at("configurationIssues")) {
					YouTubeLiveStream::Status::HealthStatus::ConfigurationIssue ci;
					ci.type = issue.contains("type")
							  ? std::make_optional(issue.at("type").get<std::string>())
							  : std::nullopt;
					ci.severity =
						issue.contains("severity")
							? std::make_optional(issue.at("severity").get<std::string>())
							: std::nullopt;
					ci.reason = issue.contains("reason")
							    ? std::make_optional(issue.at("reason").get<std::string>())
							    : std::nullopt;
					ci.description =
						issue.contains("description")
							? std::make_optional(issue.at("description").get<std::string>())
							: std::nullopt;
					issues.push_back(std::move(ci));
				}
				h.configurationIssues = std::move(issues);
			} else {
				h.configurationIssues = std::nullopt;
			}
			s.healthStatus = std::move(h);
		} else {
			s.healthStatus = std::nullopt;
		}
		p.status = std::move(s);
	} else {
		p.status = std::nullopt;
	}

	// contentDetails
	if (j.contains("contentDetails")) {
		const auto &contentDetails = j.at("contentDetails");
		YouTubeLiveStream::ContentDetails c;
		c.closedCaptionsIngestionUrl =
			contentDetails.contains("closedCaptionsIngestionUrl")
				? std::make_optional(contentDetails.at("closedCaptionsIngestionUrl").get<std::string>())
				: std::nullopt;
		c.isReusable = contentDetails.contains("isReusable")
				       ? std::make_optional(contentDetails.at("isReusable").get<bool>())
				       : std::nullopt;
		p.contentDetails = std::move(c);
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
				s.at("thumbnails").get<std::unordered_map<std::string, YouTubeLiveBroadcast::Snippet::Thumbnail>>();
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
