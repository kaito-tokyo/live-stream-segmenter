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

#include "YouTubeTypes.hpp"

#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

void to_json(nlohmann::json &j, const YouTubeLiveStream &p)
{
	j = nlohmann::json{{"id", p.id},
			   {"kind", p.kind},
			   {"snippet",
			    {{"title", p.snippet_title},
			     {"description", p.snippet_description},
			     {"channelId", p.snippet_channelId},
			     {"publishedAt", p.snippet_publishedAt},
			     {"privacyStatus", p.snippet_privacyStatus}}},
			   {"cdn",
			    {{"ingestionType", p.cdn_ingestionType},
			     {"resolution", p.cdn_resolution},
			     {"frameRate", p.cdn_frameRate},
			     {"region", p.cdn_region},
			     {"ingestionInfo",
			      {{"streamName", p.cdn_ingestionInfo_streamName},
			       {"ingestionAddress", p.cdn_ingestionInfo_ingestionAddress},
			       {"backupIngestionAddress", p.cdn_ingestionInfo_backupIngestionAddress}}}}},
			   {"contentDetails", {{"isReusable", p.cdn_isReusable}}}};
}

void from_json(const nlohmann::json &j, YouTubeLiveStream &p)
{
	j.at("id").get_to(p.id);
	j.at("kind").get_to(p.kind);
	const auto &snippet = j.at("snippet");
	snippet.at("title").get_to(p.snippet_title);
	snippet.at("description").get_to(p.snippet_description);
	snippet.at("channelId").get_to(p.snippet_channelId);
	snippet.at("publishedAt").get_to(p.snippet_publishedAt);
	snippet.at("privacyStatus").get_to(p.snippet_privacyStatus);
	const auto &cdn = j.at("cdn");
	cdn.at("ingestionType").get_to(p.cdn_ingestionType);
	cdn.at("resolution").get_to(p.cdn_resolution);
	cdn.at("frameRate").get_to(p.cdn_frameRate);
	cdn.at("region").get_to(p.cdn_region);
	const auto &ingestionInfo = cdn.at("ingestionInfo");
	ingestionInfo.at("streamName").get_to(p.cdn_ingestionInfo_streamName);
	ingestionInfo.at("ingestionAddress").get_to(p.cdn_ingestionInfo_ingestionAddress);
	ingestionInfo.at("backupIngestionAddress").get_to(p.cdn_ingestionInfo_backupIngestionAddress);
	const auto &contentDetails = j.at("contentDetails");
	contentDetails.at("isReusable").get_to(p.cdn_isReusable);
}

void to_json(nlohmann::json &j, const YouTubeLiveBroadcastSettings &p)
{
	j["snippet"] = {{"title", p.snippet_title}, {"scheduledStartTime", p.snippet_scheduledStartTime}};
	if (!p.snippet_description.empty()) {
		j["snippet"]["description"] = p.snippet_description;
	}

	j["status"] = {{"privacyStatus", p.status_privacyStatus},
		       {"selfDeclaredMadeForKids", p.status_selfDeclaredMadeForKids}};

	j["contentDetails"] = {{"enableAutoStart", p.contentDetails_enableAutoStart},
			       {"enableAutoStop", p.contentDetails_enableAutoStop},
			       {"enableDvr", p.contentDetails_enableDvr},
			       {"enableEmbed", p.contentDetails_enableEmbed},
			       {"recordFromStart", p.contentDetails_recordFromStart},
			       {"latencyPreference", p.contentDetails_latencyPreference},
			       {"monitorStream",
				{{"enableMonitorStream", p.contentDetails_monitorStream_enableMonitorStream}}}};
}

void from_json(const nlohmann::json &j, YouTubeLiveBroadcastSettings &p)
{
	const auto &snippet = j.at("snippet");
	snippet.at("title").get_to(p.snippet_title);
	if (snippet.contains("description")) {
		snippet.at("description").get_to(p.snippet_description);
	} else {
		p.snippet_description.clear();
	}
	snippet.at("scheduledStartTime").get_to(p.snippet_scheduledStartTime);

	const auto &status = j.at("status");
	status.at("privacyStatus").get_to(p.status_privacyStatus);
	status.at("selfDeclaredMadeForKids").get_to(p.status_selfDeclaredMadeForKids);

	const auto &contentDetails = j.at("contentDetails");
	contentDetails.at("enableAutoStart").get_to(p.contentDetails_enableAutoStart);
	contentDetails.at("enableAutoStop").get_to(p.contentDetails_enableAutoStop);
	contentDetails.at("enableDvr").get_to(p.contentDetails_enableDvr);
	contentDetails.at("enableEmbed").get_to(p.contentDetails_enableEmbed);
	contentDetails.at("recordFromStart").get_to(p.contentDetails_recordFromStart);
	contentDetails.at("latencyPreference").get_to(p.contentDetails_latencyPreference);
	if (contentDetails.contains("monitorStream")) {
		const auto &monitorStream = contentDetails.at("monitorStream");
		monitorStream.at("enableMonitorStream").get_to(p.contentDetails_monitorStream_enableMonitorStream);
	} else {
		p.contentDetails_monitorStream_enableMonitorStream = false;
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi

// --- YouTubeLiveBroadcast JSON conversion ---
namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi {

using nlohmann::json;

// Thumbnail (flat)
void to_json(json &j, const YouTubeLiveBroadcastThumbnail &p)
{
	j = json{{"url", p.url}};
	if (p.width)
		j["width"] = *p.width;
	if (p.height)
		j["height"] = *p.height;
}
void from_json(const json &j, YouTubeLiveBroadcastThumbnail &p)
{
	j.at("url").get_to(p.url);
	if (j.contains("width"))
		j.at("width").get_to(p.width.emplace());
	if (j.contains("height"))
		j.at("height").get_to(p.height.emplace());
}

void to_json(json &j, const YouTubeLiveBroadcast &p)
{
	j = json{{"kind", p.kind},
		 {"etag", p.etag},
		 {"id", p.id},
		 {"snippet",
		  {{"publishedAt", p.snippet_publishedAt},
		   {"channelId", p.snippet_channelId},
		   {"title", p.snippet_title},
		   {"description", p.snippet_description},
		   {"thumbnails", p.snippet_thumbnails},
		   {"scheduledStartTime", p.snippet_scheduledStartTime}}},
		 {"status",
		  {{"lifeCycleStatus", p.status_lifeCycleStatus},
		   {"privacyStatus", p.status_privacyStatus},
		   {"recordingStatus", p.status_recordingStatus}}},
		 {"contentDetails", json::object()},
		 {"statistics", json::object()},
		 {"monetizationDetails", json::object()}};
	// snippet optionals
	if (p.snippet_scheduledEndTime)
		j["snippet"]["scheduledEndTime"] = *p.snippet_scheduledEndTime;
	if (p.snippet_actualStartTime)
		j["snippet"]["actualStartTime"] = *p.snippet_actualStartTime;
	if (p.snippet_actualEndTime)
		j["snippet"]["actualEndTime"] = *p.snippet_actualEndTime;
	if (p.snippet_isDefaultBroadcast)
		j["snippet"]["isDefaultBroadcast"] = *p.snippet_isDefaultBroadcast;
	if (p.snippet_liveChatId)
		j["snippet"]["liveChatId"] = *p.snippet_liveChatId;
	// status optionals
	if (p.status_madeForKids)
		j["status"]["madeForKids"] = *p.status_madeForKids;
	if (p.status_selfDeclaredMadeForKids)
		j["status"]["selfDeclaredMadeForKids"] = *p.status_selfDeclaredMadeForKids;
	// contentDetails
	if (p.contentDetails_boundStreamId)
		j["contentDetails"]["boundStreamId"] = *p.contentDetails_boundStreamId;
	if (p.contentDetails_boundStreamLastUpdateTimeMs)
		j["contentDetails"]["boundStreamLastUpdateTimeMs"] = *p.contentDetails_boundStreamLastUpdateTimeMs;
	if (p.contentDetails_monitorStream_enableMonitorStream)
		j["contentDetails"]["monitorStream"]["enableMonitorStream"] =
			*p.contentDetails_monitorStream_enableMonitorStream;
	if (p.contentDetails_monitorStream_broadcastStreamDelayMs)
		j["contentDetails"]["monitorStream"]["broadcastStreamDelayMs"] =
			*p.contentDetails_monitorStream_broadcastStreamDelayMs;
	if (p.contentDetails_monitorStream_embedHtml)
		j["contentDetails"]["monitorStream"]["embedHtml"] = *p.contentDetails_monitorStream_embedHtml;
	if (p.contentDetails_enableEmbed)
		j["contentDetails"]["enableEmbed"] = *p.contentDetails_enableEmbed;
	if (p.contentDetails_enableDvr)
		j["contentDetails"]["enableDvr"] = *p.contentDetails_enableDvr;
	if (p.contentDetails_recordFromStart)
		j["contentDetails"]["recordFromStart"] = *p.contentDetails_recordFromStart;
	if (p.contentDetails_enableClosedCaptions)
		j["contentDetails"]["enableClosedCaptions"] = *p.contentDetails_enableClosedCaptions;
	if (p.contentDetails_closedCaptionsType)
		j["contentDetails"]["closedCaptionsType"] = *p.contentDetails_closedCaptionsType;
	if (p.contentDetails_projection)
		j["contentDetails"]["projection"] = *p.contentDetails_projection;
	if (p.contentDetails_enableLowLatency)
		j["contentDetails"]["enableLowLatency"] = *p.contentDetails_enableLowLatency;
	if (p.contentDetails_latencyPreference)
		j["contentDetails"]["latencyPreference"] = *p.contentDetails_latencyPreference;
	if (p.contentDetails_enableAutoStart)
		j["contentDetails"]["enableAutoStart"] = *p.contentDetails_enableAutoStart;
	if (p.contentDetails_enableAutoStop)
		j["contentDetails"]["enableAutoStop"] = *p.contentDetails_enableAutoStop;
	// statistics
	if (p.statistics_totalChatCount)
		j["statistics"]["totalChatCount"] = *p.statistics_totalChatCount;
	// monetizationDetails
	if (p.monetizationDetails_cuepointSchedule_enabled)
		j["monetizationDetails"]["cuepointSchedule"]["enabled"] =
			*p.monetizationDetails_cuepointSchedule_enabled;
	if (p.monetizationDetails_cuepointSchedule_pauseAdsUntil)
		j["monetizationDetails"]["cuepointSchedule"]["pauseAdsUntil"] =
			*p.monetizationDetails_cuepointSchedule_pauseAdsUntil;
	if (p.monetizationDetails_cuepointSchedule_scheduleStrategy)
		j["monetizationDetails"]["cuepointSchedule"]["scheduleStrategy"] =
			*p.monetizationDetails_cuepointSchedule_scheduleStrategy;
	if (p.monetizationDetails_cuepointSchedule_repeatIntervalSecs)
		j["monetizationDetails"]["cuepointSchedule"]["repeatIntervalSecs"] =
			*p.monetizationDetails_cuepointSchedule_repeatIntervalSecs;
}

void from_json(const json &j, YouTubeLiveBroadcast &p)
{
	j.at("kind").get_to(p.kind);
	j.at("etag").get_to(p.etag);
	j.at("id").get_to(p.id);
	// snippet
	if (j.contains("snippet")) {
		const auto &s = j.at("snippet");
		s.at("publishedAt").get_to(p.snippet_publishedAt);
		s.at("channelId").get_to(p.snippet_channelId);
		s.at("title").get_to(p.snippet_title);
		s.at("description").get_to(p.snippet_description);
		s.at("thumbnails").get_to(p.snippet_thumbnails);
		s.at("scheduledStartTime").get_to(p.snippet_scheduledStartTime);
		if (s.contains("scheduledEndTime"))
			s.at("scheduledEndTime").get_to(p.snippet_scheduledEndTime.emplace());
		if (s.contains("actualStartTime"))
			s.at("actualStartTime").get_to(p.snippet_actualStartTime.emplace());
		if (s.contains("actualEndTime"))
			s.at("actualEndTime").get_to(p.snippet_actualEndTime.emplace());
		if (s.contains("isDefaultBroadcast"))
			s.at("isDefaultBroadcast").get_to(p.snippet_isDefaultBroadcast.emplace());
		if (s.contains("liveChatId"))
			s.at("liveChatId").get_to(p.snippet_liveChatId.emplace());
	}
	// status
	if (j.contains("status")) {
		const auto &s = j.at("status");
		s.at("lifeCycleStatus").get_to(p.status_lifeCycleStatus);
		s.at("privacyStatus").get_to(p.status_privacyStatus);
		s.at("recordingStatus").get_to(p.status_recordingStatus);
		if (s.contains("madeForKids"))
			s.at("madeForKids").get_to(p.status_madeForKids.emplace());
		if (s.contains("selfDeclaredMadeForKids"))
			s.at("selfDeclaredMadeForKids").get_to(p.status_selfDeclaredMadeForKids.emplace());
	}
	// contentDetails
	if (j.contains("contentDetails")) {
		const auto &c = j.at("contentDetails");
		if (c.contains("boundStreamId"))
			c.at("boundStreamId").get_to(p.contentDetails_boundStreamId.emplace());
		if (c.contains("boundStreamLastUpdateTimeMs"))
			c.at("boundStreamLastUpdateTimeMs")
				.get_to(p.contentDetails_boundStreamLastUpdateTimeMs.emplace());
		if (c.contains("monitorStream")) {
			const auto &m = c.at("monitorStream");
			if (m.contains("enableMonitorStream"))
				m.at("enableMonitorStream")
					.get_to(p.contentDetails_monitorStream_enableMonitorStream.emplace());
			if (m.contains("broadcastStreamDelayMs"))
				m.at("broadcastStreamDelayMs")
					.get_to(p.contentDetails_monitorStream_broadcastStreamDelayMs.emplace());
			if (m.contains("embedHtml"))
				m.at("embedHtml").get_to(p.contentDetails_monitorStream_embedHtml.emplace());
		}
		if (c.contains("enableEmbed"))
			c.at("enableEmbed").get_to(p.contentDetails_enableEmbed.emplace());
		if (c.contains("enableDvr"))
			c.at("enableDvr").get_to(p.contentDetails_enableDvr.emplace());
		if (c.contains("recordFromStart"))
			c.at("recordFromStart").get_to(p.contentDetails_recordFromStart.emplace());
		if (c.contains("enableClosedCaptions"))
			c.at("enableClosedCaptions").get_to(p.contentDetails_enableClosedCaptions.emplace());
		if (c.contains("closedCaptionsType"))
			c.at("closedCaptionsType").get_to(p.contentDetails_closedCaptionsType.emplace());
		if (c.contains("projection"))
			c.at("projection").get_to(p.contentDetails_projection.emplace());
		if (c.contains("enableLowLatency"))
			c.at("enableLowLatency").get_to(p.contentDetails_enableLowLatency.emplace());
		if (c.contains("latencyPreference"))
			c.at("latencyPreference").get_to(p.contentDetails_latencyPreference.emplace());
		if (c.contains("enableAutoStart"))
			c.at("enableAutoStart").get_to(p.contentDetails_enableAutoStart.emplace());
		if (c.contains("enableAutoStop"))
			c.at("enableAutoStop").get_to(p.contentDetails_enableAutoStop.emplace());
	}
	// statistics
	if (j.contains("statistics")) {
		const auto &s = j.at("statistics");
		if (s.contains("totalChatCount"))
			s.at("totalChatCount").get_to(p.statistics_totalChatCount.emplace());
	}
	// monetizationDetails
	if (j.contains("monetizationDetails")) {
		const auto &m = j.at("monetizationDetails");
		if (m.contains("cuepointSchedule")) {
			const auto &c = m.at("cuepointSchedule");
			if (c.contains("enabled"))
				c.at("enabled").get_to(p.monetizationDetails_cuepointSchedule_enabled.emplace());
			if (c.contains("pauseAdsUntil"))
				c.at("pauseAdsUntil")
					.get_to(p.monetizationDetails_cuepointSchedule_pauseAdsUntil.emplace());
			if (c.contains("scheduleStrategy"))
				c.at("scheduleStrategy")
					.get_to(p.monetizationDetails_cuepointSchedule_scheduleStrategy.emplace());
			if (c.contains("repeatIntervalSecs"))
				c.at("repeatIntervalSecs")
					.get_to(p.monetizationDetails_cuepointSchedule_repeatIntervalSecs.emplace());
		}
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
