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

void to_json(nlohmann::json &j, const InsertingYouTubeLiveBroadcast &p)
{
	j["snippet"] = {{"title", p.snippet.title}, {"scheduledStartTime", p.snippet.scheduledStartTime}};
	if (!p.snippet.description.empty()) {
		j["snippet"]["description"] = p.snippet.description;
	}

	j["status"] = {{"privacyStatus", p.status.privacyStatus},
		       {"selfDeclaredMadeForKids", p.status.selfDeclaredMadeForKids}};

	j["contentDetails"] = {{"enableAutoStart", p.contentDetails.enableAutoStart},
			       {"enableAutoStop", p.contentDetails.enableAutoStop},
			       {"enableDvr", p.contentDetails.enableDvr},
			       {"enableEmbed", p.contentDetails.enableEmbed},
			       {"recordFromStart", p.contentDetails.recordFromStart},
			       {"latencyPreference", p.contentDetails.latencyPreference},
			       {"monitorStream",
				{{"enableMonitorStream", p.contentDetails.monitorStream.enableMonitorStream}}}};
}

void from_json(const nlohmann::json &j, InsertingYouTubeLiveBroadcast &p)
{
	const auto &snippet = j.at("snippet");
	snippet.at("title").get_to(p.snippet.title);
	if (snippet.contains("description")) {
		snippet.at("description").get_to(p.snippet.description);
	} else {
		p.snippet.description.clear();
	}
	snippet.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);

	const auto &status = j.at("status");
	status.at("privacyStatus").get_to(p.status.privacyStatus);
	status.at("selfDeclaredMadeForKids").get_to(p.status.selfDeclaredMadeForKids);

	const auto &contentDetails = j.at("contentDetails");
	contentDetails.at("enableAutoStart").get_to(p.contentDetails.enableAutoStart);
	contentDetails.at("enableAutoStop").get_to(p.contentDetails.enableAutoStop);
	contentDetails.at("enableDvr").get_to(p.contentDetails.enableDvr);
	contentDetails.at("enableEmbed").get_to(p.contentDetails.enableEmbed);
	contentDetails.at("recordFromStart").get_to(p.contentDetails.recordFromStart);
	contentDetails.at("latencyPreference").get_to(p.contentDetails.latencyPreference);
	if (contentDetails.contains("monitorStream")) {
		const auto &monitorStream = contentDetails.at("monitorStream");
		monitorStream.at("enableMonitorStream").get_to(p.contentDetails.monitorStream.enableMonitorStream);
	} else {
		p.contentDetails.monitorStream.enableMonitorStream = false;
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
	j = nlohmann::json{{"kind", p.kind},
			   {"etag", p.etag},
			   {"id", p.id},
			   {"snippet",
			    {{"publishedAt", p.snippet.publishedAt},
			     {"channelId", p.snippet.channelId},
			     {"title", p.snippet.title},
			     {"description", p.snippet.description},
			     {"thumbnails", p.snippet.thumbnails},
			     {"scheduledStartTime", p.snippet.scheduledStartTime}}},
			   {"status",
			    {{"lifeCycleStatus", p.status.lifeCycleStatus},
			     {"privacyStatus", p.status.privacyStatus},
			     {"recordingStatus", p.status.recordingStatus}}},
			   {"contentDetails", nlohmann::json::object()},
			   {"statistics", nlohmann::json::object()},
			   {"monetizationDetails", nlohmann::json::object()}};
	// snippet optionals
	if (p.snippet.scheduledEndTime)
		j["snippet"]["scheduledEndTime"] = *p.snippet.scheduledEndTime;
	if (p.snippet.actualStartTime)
		j["snippet"]["actualStartTime"] = *p.snippet.actualStartTime;
	if (p.snippet.actualEndTime)
		j["snippet"]["actualEndTime"] = *p.snippet.actualEndTime;
	if (p.snippet.isDefaultBroadcast)
		j["snippet"]["isDefaultBroadcast"] = *p.snippet.isDefaultBroadcast;
	if (p.snippet.liveChatId)
		j["snippet"]["liveChatId"] = *p.snippet.liveChatId;
	// status optionals
	if (p.status.madeForKids)
		j["status"]["madeForKids"] = *p.status.madeForKids;
	if (p.status.selfDeclaredMadeForKids)
		j["status"]["selfDeclaredMadeForKids"] = *p.status.selfDeclaredMadeForKids;
	// contentDetails
	if (p.contentDetails.boundStreamId)
		j["contentDetails"]["boundStreamId"] = *p.contentDetails.boundStreamId;
	if (p.contentDetails.boundStreamLastUpdateTimeMs)
		j["contentDetails"]["boundStreamLastUpdateTimeMs"] = *p.contentDetails.boundStreamLastUpdateTimeMs;
	if (p.contentDetails.monitorStream.enableMonitorStream)
		j["contentDetails"]["monitorStream"]["enableMonitorStream"] =
			*p.contentDetails.monitorStream.enableMonitorStream;
	if (p.contentDetails.monitorStream.broadcastStreamDelayMs)
		j["contentDetails"]["monitorStream"]["broadcastStreamDelayMs"] =
			*p.contentDetails.monitorStream.broadcastStreamDelayMs;
	if (p.contentDetails.monitorStream.embedHtml)
		j["contentDetails"]["monitorStream"]["embedHtml"] = *p.contentDetails.monitorStream.embedHtml;
	if (p.contentDetails.enableEmbed)
		j["contentDetails"]["enableEmbed"] = *p.contentDetails.enableEmbed;
	if (p.contentDetails.enableDvr)
		j["contentDetails"]["enableDvr"] = *p.contentDetails.enableDvr;
	if (p.contentDetails.recordFromStart)
		j["contentDetails"]["recordFromStart"] = *p.contentDetails.recordFromStart;
	if (p.contentDetails.enableClosedCaptions)
		j["contentDetails"]["enableClosedCaptions"] = *p.contentDetails.enableClosedCaptions;
	if (p.contentDetails.closedCaptionsType)
		j["contentDetails"]["closedCaptionsType"] = *p.contentDetails.closedCaptionsType;
	if (p.contentDetails.projection)
		j["contentDetails"]["projection"] = *p.contentDetails.projection;
	if (p.contentDetails.enableLowLatency)
		j["contentDetails"]["enableLowLatency"] = *p.contentDetails.enableLowLatency;
	if (p.contentDetails.latencyPreference)
		j["contentDetails"]["latencyPreference"] = *p.contentDetails.latencyPreference;
	if (p.contentDetails.enableAutoStart)
		j["contentDetails"]["enableAutoStart"] = *p.contentDetails.enableAutoStart;
	if (p.contentDetails.enableAutoStop)
		j["contentDetails"]["enableAutoStop"] = *p.contentDetails.enableAutoStop;
	// statistics
	if (p.statistics.totalChatCount)
		j["statistics"]["totalChatCount"] = *p.statistics.totalChatCount;
	// monetizationDetails
	if (p.monetizationDetails.cuepointSchedule.enabled)
		j["monetizationDetails"]["cuepointSchedule"]["enabled"] =
			*p.monetizationDetails.cuepointSchedule.enabled;
	if (p.monetizationDetails.cuepointSchedule.pauseAdsUntil)
		j["monetizationDetails"]["cuepointSchedule"]["pauseAdsUntil"] =
			*p.monetizationDetails.cuepointSchedule.pauseAdsUntil;
	if (p.monetizationDetails.cuepointSchedule.scheduleStrategy)
		j["monetizationDetails"]["cuepointSchedule"]["scheduleStrategy"] =
			*p.monetizationDetails.cuepointSchedule.scheduleStrategy;
	if (p.monetizationDetails.cuepointSchedule.repeatIntervalSecs)
		j["monetizationDetails"]["cuepointSchedule"]["repeatIntervalSecs"] =
			*p.monetizationDetails.cuepointSchedule.repeatIntervalSecs;
}

void from_json(const nlohmann::json &j, YouTubeLiveBroadcast &p)
{
	j.at("kind").get_to(p.kind);
	j.at("etag").get_to(p.etag);
	j.at("id").get_to(p.id);
	// snippet
	if (j.contains("snippet")) {
		const auto &s = j.at("snippet");
		s.at("publishedAt").get_to(p.snippet.publishedAt);
		s.at("channelId").get_to(p.snippet.channelId);
		s.at("title").get_to(p.snippet.title);
		s.at("description").get_to(p.snippet.description);
		s.at("thumbnails").get_to(p.snippet.thumbnails);
		s.at("scheduledStartTime").get_to(p.snippet.scheduledStartTime);
		if (s.contains("scheduledEndTime"))
			s.at("scheduledEndTime").get_to(p.snippet.scheduledEndTime.emplace());
		if (s.contains("actualStartTime"))
			s.at("actualStartTime").get_to(p.snippet.actualStartTime.emplace());
		if (s.contains("actualEndTime"))
			s.at("actualEndTime").get_to(p.snippet.actualEndTime.emplace());
		if (s.contains("isDefaultBroadcast"))
			s.at("isDefaultBroadcast").get_to(p.snippet.isDefaultBroadcast.emplace());
		if (s.contains("liveChatId"))
			s.at("liveChatId").get_to(p.snippet.liveChatId.emplace());
	}
	// status
	if (j.contains("status")) {
		const auto &s = j.at("status");
		s.at("lifeCycleStatus").get_to(p.status.lifeCycleStatus);
		s.at("privacyStatus").get_to(p.status.privacyStatus);
		s.at("recordingStatus").get_to(p.status.recordingStatus);
		if (s.contains("madeForKids"))
			s.at("madeForKids").get_to(p.status.madeForKids.emplace());
		if (s.contains("selfDeclaredMadeForKids"))
			s.at("selfDeclaredMadeForKids").get_to(p.status.selfDeclaredMadeForKids.emplace());
	}
	// contentDetails
	if (j.contains("contentDetails")) {
		const auto &c = j.at("contentDetails");
		if (c.contains("boundStreamId"))
			c.at("boundStreamId").get_to(p.contentDetails.boundStreamId.emplace());
		if (c.contains("boundStreamLastUpdateTimeMs"))
			c.at("boundStreamLastUpdateTimeMs")
				.get_to(p.contentDetails.boundStreamLastUpdateTimeMs.emplace());
		if (c.contains("monitorStream")) {
			const auto &m = c.at("monitorStream");
			if (m.contains("enableMonitorStream"))
				m.at("enableMonitorStream")
					.get_to(p.contentDetails.monitorStream.enableMonitorStream.emplace());
			if (m.contains("broadcastStreamDelayMs"))
				m.at("broadcastStreamDelayMs")
					.get_to(p.contentDetails.monitorStream.broadcastStreamDelayMs.emplace());
			if (m.contains("embedHtml"))
				m.at("embedHtml").get_to(p.contentDetails.monitorStream.embedHtml.emplace());
		}
		if (c.contains("enableEmbed"))
			c.at("enableEmbed").get_to(p.contentDetails.enableEmbed.emplace());
		if (c.contains("enableDvr"))
			c.at("enableDvr").get_to(p.contentDetails.enableDvr.emplace());
		if (c.contains("recordFromStart"))
			c.at("recordFromStart").get_to(p.contentDetails.recordFromStart.emplace());
		if (c.contains("enableClosedCaptions"))
			c.at("enableClosedCaptions").get_to(p.contentDetails.enableClosedCaptions.emplace());
		if (c.contains("closedCaptionsType"))
			c.at("closedCaptionsType").get_to(p.contentDetails.closedCaptionsType.emplace());
		if (c.contains("projection"))
			c.at("projection").get_to(p.contentDetails.projection.emplace());
		if (c.contains("enableLowLatency"))
			c.at("enableLowLatency").get_to(p.contentDetails.enableLowLatency.emplace());
		if (c.contains("latencyPreference"))
			c.at("latencyPreference").get_to(p.contentDetails.latencyPreference.emplace());
		if (c.contains("enableAutoStart"))
			c.at("enableAutoStart").get_to(p.contentDetails.enableAutoStart.emplace());
		if (c.contains("enableAutoStop"))
			c.at("enableAutoStop").get_to(p.contentDetails.enableAutoStop.emplace());
	}
	// statistics
	if (j.contains("statistics")) {
		const auto &s = j.at("statistics");
		if (s.contains("totalChatCount"))
			s.at("totalChatCount").get_to(p.statistics.totalChatCount.emplace());
	}
	// monetizationDetails
	if (j.contains("monetizationDetails")) {
		const auto &m = j.at("monetizationDetails");
		if (m.contains("cuepointSchedule")) {
			const auto &c = m.at("cuepointSchedule");
			if (c.contains("enabled"))
				c.at("enabled").get_to(p.monetizationDetails.cuepointSchedule.enabled.emplace());
			if (c.contains("pauseAdsUntil"))
				c.at("pauseAdsUntil")
					.get_to(p.monetizationDetails.cuepointSchedule.pauseAdsUntil.emplace());
			if (c.contains("scheduleStrategy"))
				c.at("scheduleStrategy")
					.get_to(p.monetizationDetails.cuepointSchedule.scheduleStrategy.emplace());
			if (c.contains("repeatIntervalSecs"))
				c.at("repeatIntervalSecs")
					.get_to(p.monetizationDetails.cuepointSchedule.repeatIntervalSecs.emplace());
		}
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
	j["status"] = std::move(statusJson);

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
	monetizationDetailsJson["cuepointSchedule"] = std::move(cuepointScheduleJson);
	j["monetizationDetails"] = std::move(monetizationDetailsJson);
}

} // namespace KaitoTokyo::YouTubeApi
