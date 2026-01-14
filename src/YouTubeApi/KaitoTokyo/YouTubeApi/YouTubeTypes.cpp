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
	if (j.contains("kind"))
		j.at("kind").get_to(p.kind);
	if (j.contains("etag"))
		j.at("etag").get_to(p.etag);
	if (j.contains("id"))
		j.at("id").get_to(p.id);

	// snippet
	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		YouTubeLiveStream::Snippet s;
		if (snippet.contains("publishedAt"))
			snippet.at("publishedAt").get_to(s.publishedAt);
		if (snippet.contains("channelId"))
			snippet.at("channelId").get_to(s.channelId);
		if (snippet.contains("title"))
			snippet.at("title").get_to(s.title);
		if (snippet.contains("description"))
			snippet.at("description").get_to(s.description);
		if (snippet.contains("isDefaultStream"))
			snippet.at("isDefaultStream").get_to(s.isDefaultStream);
		p.snippet = std::move(s);
	} else {
		p.snippet = std::nullopt;
	}

	// cdn
	if (j.contains("cdn")) {
		const auto &cdn = j.at("cdn");
		YouTubeLiveStream::Cdn c;
		if (cdn.contains("ingestionType"))
			cdn.at("ingestionType").get_to(c.ingestionType);
		if (cdn.contains("ingestionInfo")) {
			const auto &ingestionInfo = cdn.at("ingestionInfo");
			YouTubeLiveStream::Cdn::IngestionInfo info;
			if (ingestionInfo.contains("streamName"))
				ingestionInfo.at("streamName").get_to(info.streamName);
			if (ingestionInfo.contains("ingestionAddress"))
				ingestionInfo.at("ingestionAddress").get_to(info.ingestionAddress);
			if (ingestionInfo.contains("backupIngestionAddress"))
				ingestionInfo.at("backupIngestionAddress").get_to(info.backupIngestionAddress);
			c.ingestionInfo = std::move(info);
		} else {
			c.ingestionInfo = std::nullopt;
		}
		if (cdn.contains("resolution"))
			cdn.at("resolution").get_to(c.resolution);
		if (cdn.contains("frameRate"))
			cdn.at("frameRate").get_to(c.frameRate);
		p.cdn = std::move(c);
	} else {
		p.cdn = std::nullopt;
	}

	// status
	if (j.contains("status")) {
		const auto &status = j.at("status");
		YouTubeLiveStream::Status s;
		if (status.contains("streamStatus"))
			status.at("streamStatus").get_to(s.streamStatus);
		if (status.contains("healthStatus")) {
			const auto &healthStatus = status.at("healthStatus");
			YouTubeLiveStream::Status::HealthStatus h;
			if (healthStatus.contains("status"))
				healthStatus.at("status").get_to(h.status);
			if (healthStatus.contains("lastUpdateTimeSeconds"))
				healthStatus.at("lastUpdateTimeSeconds").get_to(h.lastUpdateTimeSeconds);
			if (healthStatus.contains("configurationIssues")) {
				std::vector<YouTubeLiveStream::Status::HealthStatus::ConfigurationIssue> issues;
				for (const auto &issue : healthStatus.at("configurationIssues")) {
					YouTubeLiveStream::Status::HealthStatus::ConfigurationIssue ci;
					if (issue.contains("type"))
						issue.at("type").get_to(ci.type);
					if (issue.contains("severity"))
						issue.at("severity").get_to(ci.severity);
					if (issue.contains("reason"))
						issue.at("reason").get_to(ci.reason);
					if (issue.contains("description"))
						issue.at("description").get_to(ci.description);
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
		if (contentDetails.contains("closedCaptionsIngestionUrl"))
			contentDetails.at("closedCaptionsIngestionUrl").get_to(c.closedCaptionsIngestionUrl);
		if (contentDetails.contains("isReusable"))
			contentDetails.at("isReusable").get_to(c.isReusable);
		p.contentDetails = std::move(c);
	} else {
		p.contentDetails = std::nullopt;
	}
}

void to_json(nlohmann::json &j, const InsertingYouTubeLiveStream &p)
{
    j = nlohmann::json{};

    // snippet
    if (p.snippet) {
        nlohmann::json snippetJson;
        snippetJson["title"] = p.snippet->title;
        if (p.snippet->description) {
            snippetJson["description"] = *p.snippet->description;
        }
        j["snippet"] = std::move(snippetJson);
    }

    // cdn
    nlohmann::json cdnJson;
    cdnJson["ingestionType"] = p.cdn.ingestionType;
    cdnJson["resolution"] = p.cdn.resolution;
    cdnJson["frameRate"] = p.cdn.frameRate;
    j["cdn"] = std::move(cdnJson);

    // contentDetails
    if (p.contentDetails) {
        nlohmann::json contentDetailsJson;
        if (p.contentDetails->isReusable) {
            contentDetailsJson["isReusable"] = *p.contentDetails->isReusable;
        }
        j["contentDetails"] = std::move(contentDetailsJson);
    }
}

void from_json(const nlohmann::json &j, InsertingYouTubeLiveStream &p)
{
	// snippet
	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		InsertingYouTubeLiveStream::Snippet s;
		snippet.at("title").get_to(s.title);
		if (snippet.contains("description")) {
			snippet.at("description").get_to(s.description);
		}
		p.snippet = std::move(s);
	} else {
		p.snippet = std::nullopt;
	}

	// cdn
	const auto &cdn = j.at("cdn");
	cdn.at("ingestionType").get_to(p.cdn.ingestionType);
	cdn.at("resolution").get_to(p.cdn.resolution);
	cdn.at("frameRate").get_to(p.cdn.frameRate);

	// contentDetails
	if (j.contains("contentDetails")) {
		const auto &contentDetails = j.at("contentDetails");
		InsertingYouTubeLiveStream::ContentDetails c;
		if (contentDetails.contains("isReusable")) {
			contentDetails.at("isReusable").get_to(c.isReusable);
		}
		p.contentDetails = std::move(c);
	} else {
		p.contentDetails = std::nullopt;
	}
}

void to_json(nlohmann::json &j, const YouTubeLiveBroadcastThumbnail &p)
{
	j = nlohmann::json{};
	if (p.url)
		j["url"] = *p.url;
	if (p.width)
		j["width"] = *p.width;
	if (p.height)
		j["height"] = *p.height;
}

void from_json(const nlohmann::json &j, YouTubeLiveBroadcastThumbnail &p)
{
	if (j.contains("url"))
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
		j.at("kind").get_to(p.kind);
	if (j.contains("etag"))
		j.at("etag").get_to(p.etag);
	if (j.contains("id"))
		j.at("id").get_to(p.id);
	if (j.contains("snippet")) {
		YouTubeLiveBroadcast::Snippet snippetObj;
		const auto &s = j.at("snippet");
		if (s.contains("publishedAt"))
			s.at("publishedAt").get_to(snippetObj.publishedAt);
		if (s.contains("channelId"))
			s.at("channelId").get_to(snippetObj.channelId);
		if (s.contains("title"))
			s.at("title").get_to(snippetObj.title);
		if (s.contains("description"))
			s.at("description").get_to(snippetObj.description);
		if (s.contains("thumbnails"))
			s.at("thumbnails").get_to(snippetObj.thumbnails);
		if (s.contains("scheduledStartTime"))
			s.at("scheduledStartTime").get_to(snippetObj.scheduledStartTime);
		if (s.contains("scheduledEndTime"))
			s.at("scheduledEndTime").get_to(snippetObj.scheduledEndTime);
		if (s.contains("actualStartTime"))
			s.at("actualStartTime").get_to(snippetObj.actualStartTime);
		if (s.contains("actualEndTime"))
			s.at("actualEndTime").get_to(snippetObj.actualEndTime);
		if (s.contains("isDefaultBroadcast"))
			s.at("isDefaultBroadcast").get_to(snippetObj.isDefaultBroadcast);
		if (s.contains("liveChatId"))
			s.at("liveChatId").get_to(snippetObj.liveChatId);
		p.snippet = std::move(snippetObj);
	} else {
		p.snippet = std::nullopt;
	}
	if (j.contains("status")) {
		YouTubeLiveBroadcast::Status statusObj;
		const auto &s = j.at("status");
		if (s.contains("lifeCycleStatus"))
			s.at("lifeCycleStatus").get_to(statusObj.lifeCycleStatus);
		if (s.contains("privacyStatus"))
			s.at("privacyStatus").get_to(statusObj.privacyStatus);
		if (s.contains("recordingStatus"))
			s.at("recordingStatus").get_to(statusObj.recordingStatus);
		if (s.contains("madeForKids"))
			s.at("madeForKids").get_to(statusObj.madeForKids);
		if (s.contains("selfDeclaredMadeForKids"))
			s.at("selfDeclaredMadeForKids").get_to(statusObj.selfDeclaredMadeForKids);
		p.status = std::move(statusObj);
	} else {
		p.status = std::nullopt;
	}
	if (j.contains("contentDetails")) {
		YouTubeLiveBroadcast::ContentDetails contentDetailsObj;
		const auto &c = j.at("contentDetails");
		if (c.contains("boundStreamId"))
			c.at("boundStreamId").get_to(contentDetailsObj.boundStreamId);
		if (c.contains("boundStreamLastUpdateTimeMs"))
			c.at("boundStreamLastUpdateTimeMs").get_to(contentDetailsObj.boundStreamLastUpdateTimeMs);
		if (c.contains("monitorStream")) {
			YouTubeLiveBroadcast::ContentDetails::MonitorStream monitorStreamObj;
			const auto &m = c.at("monitorStream");
			if (m.contains("enableMonitorStream"))
				m.at("enableMonitorStream").get_to(monitorStreamObj.enableMonitorStream);
			if (m.contains("broadcastStreamDelayMs"))
				m.at("broadcastStreamDelayMs").get_to(monitorStreamObj.broadcastStreamDelayMs);
			if (m.contains("embedHtml"))
				m.at("embedHtml").get_to(monitorStreamObj.embedHtml);
			contentDetailsObj.monitorStream = std::move(monitorStreamObj);
		} else {
			contentDetailsObj.monitorStream = std::nullopt;
		}
		if (c.contains("enableEmbed"))
			c.at("enableEmbed").get_to(contentDetailsObj.enableEmbed);
		if (c.contains("enableDvr"))
			c.at("enableDvr").get_to(contentDetailsObj.enableDvr);
		if (c.contains("recordFromStart"))
			c.at("recordFromStart").get_to(contentDetailsObj.recordFromStart);
		if (c.contains("enableClosedCaptions"))
			c.at("enableClosedCaptions").get_to(contentDetailsObj.enableClosedCaptions);
		if (c.contains("closedCaptionsType"))
			c.at("closedCaptionsType").get_to(contentDetailsObj.closedCaptionsType);
		if (c.contains("projection"))
			c.at("projection").get_to(contentDetailsObj.projection);
		if (c.contains("enableLowLatency"))
			c.at("enableLowLatency").get_to(contentDetailsObj.enableLowLatency);
		if (c.contains("latencyPreference"))
			c.at("latencyPreference").get_to(contentDetailsObj.latencyPreference);
		if (c.contains("enableAutoStart"))
			c.at("enableAutoStart").get_to(contentDetailsObj.enableAutoStart);
		if (c.contains("enableAutoStop"))
			c.at("enableAutoStop").get_to(contentDetailsObj.enableAutoStop);
		p.contentDetails = std::move(contentDetailsObj);
	} else {
		p.contentDetails = std::nullopt;
	}
	if (j.contains("statistics")) {
		YouTubeLiveBroadcast::Statistics statisticsObj;
		const auto &s = j.at("statistics");
		if (s.contains("totalChatCount"))
			s.at("totalChatCount").get_to(statisticsObj.totalChatCount);
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
				c.at("enabled").get_to(cuepointScheduleObj.enabled);
			if (c.contains("pauseAdsUntil"))
				c.at("pauseAdsUntil").get_to(cuepointScheduleObj.pauseAdsUntil);
			if (c.contains("scheduleStrategy"))
				c.at("scheduleStrategy").get_to(cuepointScheduleObj.scheduleStrategy);
			if (c.contains("repeatIntervalSecs"))
				c.at("repeatIntervalSecs").get_to(cuepointScheduleObj.repeatIntervalSecs);
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
	if (snippet.contains("description"))
		snippet.at("description").get_to(p.snippet.description);

	snippet.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);
	if (snippet.contains("scheduledEndTime"))
		snippet.at("scheduledEndTime").get_to(p.snippet.scheduledEndTime.emplace());

	const auto &status = j.at("status");
	status.at("privacyStatus").get_to(p.status.privacyStatus);
	if (status.contains("selfDeclaredMadeForKids"))
		status.at("selfDeclaredMadeForKids").get_to(p.status.selfDeclaredMadeForKids);

	const auto &contentDetails = j.at("contentDetails");
	if (contentDetails.contains("enableAutoStart"))
		contentDetails.at("enableAutoStart").get_to(p.contentDetails.enableAutoStart);
	if (contentDetails.contains("enableAutoStop"))
		contentDetails.at("enableAutoStop").get_to(p.contentDetails.enableAutoStop);
	if (contentDetails.contains("enableClosedCaptions"))
		contentDetails.at("enableClosedCaptions").get_to(p.contentDetails.enableClosedCaptions);
	if (contentDetails.contains("enableDvr"))
		contentDetails.at("enableDvr").get_to(p.contentDetails.enableDvr);
	if (contentDetails.contains("enableEmbed"))
		contentDetails.at("enableEmbed").get_to(p.contentDetails.enableEmbed);
	if (contentDetails.contains("recordFromStart"))
		contentDetails.at("recordFromStart").get_to(p.contentDetails.recordFromStart);
	if (contentDetails.contains("latencyPreference"))
		contentDetails.at("latencyPreference").get_to(p.contentDetails.latencyPreference);
	if (contentDetails.contains("monitorStream")) {
		const auto &monitorStream = contentDetails.at("monitorStream");
		if (monitorStream.contains("enableMonitorStream"))
			monitorStream.at("enableMonitorStream")
				.get_to(p.contentDetails.monitorStream.enableMonitorStream);
		if (monitorStream.contains("broadcastStreamDelayMs"))
			monitorStream.at("broadcastStreamDelayMs")
				.get_to(p.contentDetails.monitorStream.broadcastStreamDelayMs.emplace());
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

void from_json(const nlohmann::json &j, UpdatingYouTubeLiveBroadcast &p)
{
	if (j.contains("id")) {
		j.at("id").get_to(p.id);
	}

	if (j.contains("snippet")) {
		const auto &snippet = j.at("snippet");
		if (snippet.contains("title"))
			snippet.at("title").get_to(p.snippet.title);
		if (snippet.contains("description"))
			snippet.at("description").get_to(p.snippet.description);
		snippet.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);
		if (snippet.contains("scheduledEndTime"))
			snippet.at("scheduledEndTime").get_to(p.snippet.scheduledEndTime.emplace());
	}

	if (j.contains("status")) {
		const auto &status = j.at("status");
		if (status.contains("privacyStatus"))
			status.at("privacyStatus").get_to(p.status.privacyStatus);
	}

	if (j.contains("contentDetails")) {
		const auto &cd = j.at("contentDetails");
		if (cd.contains("monitorStream")) {
			const auto &ms = cd.at("monitorStream");
			ms.at("enableMonitorStream").get_to(p.contentDetails.monitorStream.enableMonitorStream);
			if (ms.contains("broadcastStreamDelayMs"))
				ms.at("broadcastStreamDelayMs")
					.get_to(p.contentDetails.monitorStream.broadcastStreamDelayMs.emplace());
		}
		if (cd.contains("enableAutoStart"))
			cd.at("enableAutoStart").get_to(p.contentDetails.enableAutoStart);
		if (cd.contains("enableAutoStop"))
			cd.at("enableAutoStop").get_to(p.contentDetails.enableAutoStop);
		if (cd.contains("enableClosedCaptions"))
			cd.at("enableClosedCaptions").get_to(p.contentDetails.enableClosedCaptions);
		if (cd.contains("enableDvr"))
			cd.at("enableDvr").get_to(p.contentDetails.enableDvr);
		if (cd.contains("enableEmbed"))
			cd.at("enableEmbed").get_to(p.contentDetails.enableEmbed);
		if (cd.contains("recordFromStart"))
			cd.at("recordFromStart").get_to(p.contentDetails.recordFromStart);
	}

	if (j.contains("monetizationDetails")) {
		const auto &md = j.at("monetizationDetails");
		if (md.contains("cuepointSchedule")) {
			const auto &cs = md.at("cuepointSchedule");
			if (cs.contains("pauseAdsUntil"))
				cs.at("pauseAdsUntil").get_to(p.monetizationDetails.cuepointSchedule.pauseAdsUntil);
		}
	}
}

} // namespace KaitoTokyo::YouTubeApi
