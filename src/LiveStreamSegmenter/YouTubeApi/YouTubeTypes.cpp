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

void to_json(nlohmann::json &j, const YouTubeStreamKey &p)
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

void from_json(const nlohmann::json &j, YouTubeStreamKey &p)
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
void to_json(json &j, const YouTubeLiveBroadcast_Thumbnail &p)
{
       j = json{{"url", p.url}};
       if (p.width)
	       j["width"] = *p.width;
       if (p.height)
	       j["height"] = *p.height;
}
void from_json(const json &j, YouTubeLiveBroadcast_Thumbnail &p)
{
       j.at("url").get_to(p.url);
       if (j.contains("width"))
	       j.at("width").get_to(p.width.emplace());
       if (j.contains("height"))
	       j.at("height").get_to(p.height.emplace());
}

// Snippet
void to_json(json &j, const YouTubeLiveBroadcast::Snippet &p)
{
	j = json{{"publishedAt", p.publishedAt},
		 {"channelId", p.channelId},
		 {"title", p.title},
		 {"description", p.description},
		 {"thumbnails", p.thumbnails},
		 {"scheduledStartTime", p.scheduledStartTime}};
	if (p.scheduledEndTime)
		j["scheduledEndTime"] = *p.scheduledEndTime;
	if (p.actualStartTime)
		j["actualStartTime"] = *p.actualStartTime;
	if (p.actualEndTime)
		j["actualEndTime"] = *p.actualEndTime;
	if (p.isDefaultBroadcast)
		j["isDefaultBroadcast"] = *p.isDefaultBroadcast;
	if (p.liveChatId)
		j["liveChatId"] = *p.liveChatId;
}
void from_json(const json &j, YouTubeLiveBroadcast::Snippet &p)
{
	j.at("publishedAt").get_to(p.publishedAt);
	j.at("channelId").get_to(p.channelId);
	j.at("title").get_to(p.title);
	j.at("description").get_to(p.description);
	j.at("thumbnails").get_to(p.thumbnails);
	j.at("scheduledStartTime").get_to(p.scheduledStartTime);
	if (j.contains("scheduledEndTime"))
		j.at("scheduledEndTime").get_to(p.scheduledEndTime.emplace());
	if (j.contains("actualStartTime"))
		j.at("actualStartTime").get_to(p.actualStartTime.emplace());
	if (j.contains("actualEndTime"))
		j.at("actualEndTime").get_to(p.actualEndTime.emplace());
	if (j.contains("isDefaultBroadcast"))
		j.at("isDefaultBroadcast").get_to(p.isDefaultBroadcast.emplace());
	if (j.contains("liveChatId"))
		j.at("liveChatId").get_to(p.liveChatId.emplace());
}

// Status
void to_json(json &j, const YouTubeLiveBroadcast::Status &p)
{
	j = json{{"lifeCycleStatus", p.lifeCycleStatus},
		 {"privacyStatus", p.privacyStatus},
		 {"recordingStatus", p.recordingStatus}};
	if (p.madeForKids)
		j["madeForKids"] = *p.madeForKids;
	if (p.selfDeclaredMadeForKids)
		j["selfDeclaredMadeForKids"] = *p.selfDeclaredMadeForKids;
}
void from_json(const json &j, YouTubeLiveBroadcast::Status &p)
{
	j.at("lifeCycleStatus").get_to(p.lifeCycleStatus);
	j.at("privacyStatus").get_to(p.privacyStatus);
	j.at("recordingStatus").get_to(p.recordingStatus);
	if (j.contains("madeForKids"))
		j.at("madeForKids").get_to(p.madeForKids.emplace());
	if (j.contains("selfDeclaredMadeForKids"))
		j.at("selfDeclaredMadeForKids").get_to(p.selfDeclaredMadeForKids.emplace());
}

// MonitorStream
void to_json(json &j, const YouTubeLiveBroadcast::MonitorStream &p)
{
	if (p.enableMonitorStream)
		j["enableMonitorStream"] = *p.enableMonitorStream;
	if (p.broadcastStreamDelayMs)
		j["broadcastStreamDelayMs"] = *p.broadcastStreamDelayMs;
	if (p.embedHtml)
		j["embedHtml"] = *p.embedHtml;
}
void from_json(const json &j, YouTubeLiveBroadcast::MonitorStream &p)
{
	if (j.contains("enableMonitorStream"))
		j.at("enableMonitorStream").get_to(p.enableMonitorStream.emplace());
	if (j.contains("broadcastStreamDelayMs"))
		j.at("broadcastStreamDelayMs").get_to(p.broadcastStreamDelayMs.emplace());
	if (j.contains("embedHtml"))
		j.at("embedHtml").get_to(p.embedHtml.emplace());
}

// ContentDetails
void to_json(json &j, const YouTubeLiveBroadcast::ContentDetails &p)
{
	if (p.boundStreamId)
		j["boundStreamId"] = *p.boundStreamId;
	if (p.boundStreamLastUpdateTimeMs)
		j["boundStreamLastUpdateTimeMs"] = *p.boundStreamLastUpdateTimeMs;
	if (p.monitorStream)
		j["monitorStream"] = *p.monitorStream;
	if (p.enableEmbed)
		j["enableEmbed"] = *p.enableEmbed;
	if (p.enableDvr)
		j["enableDvr"] = *p.enableDvr;
	if (p.recordFromStart)
		j["recordFromStart"] = *p.recordFromStart;
	if (p.enableClosedCaptions)
		j["enableClosedCaptions"] = *p.enableClosedCaptions;
	if (p.closedCaptionsType)
		j["closedCaptionsType"] = *p.closedCaptionsType;
	if (p.projection)
		j["projection"] = *p.projection;
	if (p.enableLowLatency)
		j["enableLowLatency"] = *p.enableLowLatency;
	if (p.latencyPreference)
		j["latencyPreference"] = *p.latencyPreference;
	if (p.enableAutoStart)
		j["enableAutoStart"] = *p.enableAutoStart;
	if (p.enableAutoStop)
		j["enableAutoStop"] = *p.enableAutoStop;
}
void from_json(const json &j, YouTubeLiveBroadcast::ContentDetails &p)
{
	if (j.contains("boundStreamId"))
		j.at("boundStreamId").get_to(p.boundStreamId.emplace());
	if (j.contains("boundStreamLastUpdateTimeMs"))
		j.at("boundStreamLastUpdateTimeMs").get_to(p.boundStreamLastUpdateTimeMs.emplace());
	if (j.contains("monitorStream"))
		j.at("monitorStream").get_to(p.monitorStream.emplace());
	if (j.contains("enableEmbed"))
		j.at("enableEmbed").get_to(p.enableEmbed.emplace());
	if (j.contains("enableDvr"))
		j.at("enableDvr").get_to(p.enableDvr.emplace());
	if (j.contains("recordFromStart"))
		j.at("recordFromStart").get_to(p.recordFromStart.emplace());
	if (j.contains("enableClosedCaptions"))
		j.at("enableClosedCaptions").get_to(p.enableClosedCaptions.emplace());
	if (j.contains("closedCaptionsType"))
		j.at("closedCaptionsType").get_to(p.closedCaptionsType.emplace());
	if (j.contains("projection"))
		j.at("projection").get_to(p.projection.emplace());
	if (j.contains("enableLowLatency"))
		j.at("enableLowLatency").get_to(p.enableLowLatency.emplace());
	if (j.contains("latencyPreference"))
		j.at("latencyPreference").get_to(p.latencyPreference.emplace());
	if (j.contains("enableAutoStart"))
		j.at("enableAutoStart").get_to(p.enableAutoStart.emplace());
	if (j.contains("enableAutoStop"))
		j.at("enableAutoStop").get_to(p.enableAutoStop.emplace());
}

// Statistics
void to_json(json &j, const YouTubeLiveBroadcast::Statistics &p)
{
	if (p.totalChatCount)
		j["totalChatCount"] = *p.totalChatCount;
}
void from_json(const json &j, YouTubeLiveBroadcast::Statistics &p)
{
	if (j.contains("totalChatCount"))
		j.at("totalChatCount").get_to(p.totalChatCount.emplace());
}

// CuepointSchedule
void to_json(json &j, const YouTubeLiveBroadcast::CuepointSchedule &p)
{
	if (p.enabled)
		j["enabled"] = *p.enabled;
	if (p.pauseAdsUntil)
		j["pauseAdsUntil"] = *p.pauseAdsUntil;
	if (p.scheduleStrategy)
		j["scheduleStrategy"] = *p.scheduleStrategy;
	if (p.repeatIntervalSecs)
		j["repeatIntervalSecs"] = *p.repeatIntervalSecs;
}
void from_json(const json &j, YouTubeLiveBroadcast::CuepointSchedule &p)
{
	if (j.contains("enabled"))
		j.at("enabled").get_to(p.enabled.emplace());
	if (j.contains("pauseAdsUntil"))
		j.at("pauseAdsUntil").get_to(p.pauseAdsUntil.emplace());
	if (j.contains("scheduleStrategy"))
		j.at("scheduleStrategy").get_to(p.scheduleStrategy.emplace());
	if (j.contains("repeatIntervalSecs"))
		j.at("repeatIntervalSecs").get_to(p.repeatIntervalSecs.emplace());
}

// MonetizationDetails
void to_json(json &j, const YouTubeLiveBroadcast::MonetizationDetails &p)
{
	if (p.cuepointSchedule)
		j["cuepointSchedule"] = *p.cuepointSchedule;
}
void from_json(const json &j, YouTubeLiveBroadcast::MonetizationDetails &p)
{
	if (j.contains("cuepointSchedule"))
		j.at("cuepointSchedule").get_to(p.cuepointSchedule.emplace());
}

// YouTubeLiveBroadcast (flat)
void to_json(json &j, const YouTubeLiveBroadcast &p)
{
       j = json{
	       {"kind", p.kind},
	       {"etag", p.etag},
	       {"id", p.id},
	       {"snippet", {
		       {"publishedAt", p.snippet_publishedAt},
		       {"channelId", p.snippet_channelId},
		       {"title", p.snippet_title},
		       {"description", p.snippet_description},
		       {"thumbnails", p.snippet_thumbnails},
		       {"scheduledStartTime", p.snippet_scheduledStartTime}
	       }},
	       {"status", {
		       {"lifeCycleStatus", p.status_lifeCycleStatus},
		       {"privacyStatus", p.status_privacyStatus},
		       {"recordingStatus", p.status_recordingStatus}
	       }},
	       {"contentDetails", json::object()},
	       {"statistics", json::object()},
	       {"monetizationDetails", json::object()}
       };
       // snippet optionals
       if (p.snippet_scheduledEndTime) j["snippet"]["scheduledEndTime"] = *p.snippet_scheduledEndTime;
       if (p.snippet_actualStartTime) j["snippet"]["actualStartTime"] = *p.snippet_actualStartTime;
       if (p.snippet_actualEndTime) j["snippet"]["actualEndTime"] = *p.snippet_actualEndTime;
       if (p.snippet_isDefaultBroadcast) j["snippet"]["isDefaultBroadcast"] = *p.snippet_isDefaultBroadcast;
       if (p.snippet_liveChatId) j["snippet"]["liveChatId"] = *p.snippet_liveChatId;
       // status optionals
       if (p.status_madeForKids) j["status"]["madeForKids"] = *p.status_madeForKids;
       if (p.status_selfDeclaredMadeForKids) j["status"]["selfDeclaredMadeForKids"] = *p.status_selfDeclaredMadeForKids;
       // contentDetails
       if (p.contentDetails_boundStreamId) j["contentDetails"]["boundStreamId"] = *p.contentDetails_boundStreamId;
       if (p.contentDetails_boundStreamLastUpdateTimeMs) j["contentDetails"]["boundStreamLastUpdateTimeMs"] = *p.contentDetails_boundStreamLastUpdateTimeMs;
       if (p.contentDetails_monitorStream_enableMonitorStream) j["contentDetails"]["monitorStream"]["enableMonitorStream"] = *p.contentDetails_monitorStream_enableMonitorStream;
       if (p.contentDetails_monitorStream_broadcastStreamDelayMs) j["contentDetails"]["monitorStream"]["broadcastStreamDelayMs"] = *p.contentDetails_monitorStream_broadcastStreamDelayMs;
       if (p.contentDetails_monitorStream_embedHtml) j["contentDetails"]["monitorStream"]["embedHtml"] = *p.contentDetails_monitorStream_embedHtml;
       if (p.contentDetails_enableEmbed) j["contentDetails"]["enableEmbed"] = *p.contentDetails_enableEmbed;
       if (p.contentDetails_enableDvr) j["contentDetails"]["enableDvr"] = *p.contentDetails_enableDvr;
       if (p.contentDetails_recordFromStart) j["contentDetails"]["recordFromStart"] = *p.contentDetails_recordFromStart;
       if (p.contentDetails_enableClosedCaptions) j["contentDetails"]["enableClosedCaptions"] = *p.contentDetails_enableClosedCaptions;
       if (p.contentDetails_closedCaptionsType) j["contentDetails"]["closedCaptionsType"] = *p.contentDetails_closedCaptionsType;
       if (p.contentDetails_projection) j["contentDetails"]["projection"] = *p.contentDetails_projection;
       if (p.contentDetails_enableLowLatency) j["contentDetails"]["enableLowLatency"] = *p.contentDetails_enableLowLatency;
       if (p.contentDetails_latencyPreference) j["contentDetails"]["latencyPreference"] = *p.contentDetails_latencyPreference;
       if (p.contentDetails_enableAutoStart) j["contentDetails"]["enableAutoStart"] = *p.contentDetails_enableAutoStart;
       if (p.contentDetails_enableAutoStop) j["contentDetails"]["enableAutoStop"] = *p.contentDetails_enableAutoStop;
       // statistics
       if (p.statistics_totalChatCount) j["statistics"]["totalChatCount"] = *p.statistics_totalChatCount;
       // monetizationDetails
       if (p.monetizationDetails_cuepointSchedule_enabled) j["monetizationDetails"]["cuepointSchedule"]["enabled"] = *p.monetizationDetails_cuepointSchedule_enabled;
       if (p.monetizationDetails_cuepointSchedule_pauseAdsUntil) j["monetizationDetails"]["cuepointSchedule"]["pauseAdsUntil"] = *p.monetizationDetails_cuepointSchedule_pauseAdsUntil;
       if (p.monetizationDetails_cuepointSchedule_scheduleStrategy) j["monetizationDetails"]["cuepointSchedule"]["scheduleStrategy"] = *p.monetizationDetails_cuepointSchedule_scheduleStrategy;
       if (p.monetizationDetails_cuepointSchedule_repeatIntervalSecs) j["monetizationDetails"]["cuepointSchedule"]["repeatIntervalSecs"] = *p.monetizationDetails_cuepointSchedule_repeatIntervalSecs;
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
	       if (s.contains("scheduledEndTime")) s.at("scheduledEndTime").get_to(p.snippet_scheduledEndTime.emplace());
	       if (s.contains("actualStartTime")) s.at("actualStartTime").get_to(p.snippet_actualStartTime.emplace());
	       if (s.contains("actualEndTime")) s.at("actualEndTime").get_to(p.snippet_actualEndTime.emplace());
	       if (s.contains("isDefaultBroadcast")) s.at("isDefaultBroadcast").get_to(p.snippet_isDefaultBroadcast.emplace());
	       if (s.contains("liveChatId")) s.at("liveChatId").get_to(p.snippet_liveChatId.emplace());
       }
       // status
       if (j.contains("status")) {
	       const auto &s = j.at("status");
	       s.at("lifeCycleStatus").get_to(p.status_lifeCycleStatus);
	       s.at("privacyStatus").get_to(p.status_privacyStatus);
	       s.at("recordingStatus").get_to(p.status_recordingStatus);
	       if (s.contains("madeForKids")) s.at("madeForKids").get_to(p.status_madeForKids.emplace());
	       if (s.contains("selfDeclaredMadeForKids")) s.at("selfDeclaredMadeForKids").get_to(p.status_selfDeclaredMadeForKids.emplace());
       }
       // contentDetails
       if (j.contains("contentDetails")) {
	       const auto &c = j.at("contentDetails");
	       if (c.contains("boundStreamId")) c.at("boundStreamId").get_to(p.contentDetails_boundStreamId.emplace());
	       if (c.contains("boundStreamLastUpdateTimeMs")) c.at("boundStreamLastUpdateTimeMs").get_to(p.contentDetails_boundStreamLastUpdateTimeMs.emplace());
	       if (c.contains("monitorStream")) {
		       const auto &m = c.at("monitorStream");
		       if (m.contains("enableMonitorStream")) m.at("enableMonitorStream").get_to(p.contentDetails_monitorStream_enableMonitorStream.emplace());
		       if (m.contains("broadcastStreamDelayMs")) m.at("broadcastStreamDelayMs").get_to(p.contentDetails_monitorStream_broadcastStreamDelayMs.emplace());
		       if (m.contains("embedHtml")) m.at("embedHtml").get_to(p.contentDetails_monitorStream_embedHtml.emplace());
	       }
	       if (c.contains("enableEmbed")) c.at("enableEmbed").get_to(p.contentDetails_enableEmbed.emplace());
	       if (c.contains("enableDvr")) c.at("enableDvr").get_to(p.contentDetails_enableDvr.emplace());
	       if (c.contains("recordFromStart")) c.at("recordFromStart").get_to(p.contentDetails_recordFromStart.emplace());
	       if (c.contains("enableClosedCaptions")) c.at("enableClosedCaptions").get_to(p.contentDetails_enableClosedCaptions.emplace());
	       if (c.contains("closedCaptionsType")) c.at("closedCaptionsType").get_to(p.contentDetails_closedCaptionsType.emplace());
	       if (c.contains("projection")) c.at("projection").get_to(p.contentDetails_projection.emplace());
	       if (c.contains("enableLowLatency")) c.at("enableLowLatency").get_to(p.contentDetails_enableLowLatency.emplace());
	       if (c.contains("latencyPreference")) c.at("latencyPreference").get_to(p.contentDetails_latencyPreference.emplace());
	       if (c.contains("enableAutoStart")) c.at("enableAutoStart").get_to(p.contentDetails_enableAutoStart.emplace());
	       if (c.contains("enableAutoStop")) c.at("enableAutoStop").get_to(p.contentDetails_enableAutoStop.emplace());
       }
       // statistics
       if (j.contains("statistics")) {
	       const auto &s = j.at("statistics");
	       if (s.contains("totalChatCount")) s.at("totalChatCount").get_to(p.statistics_totalChatCount.emplace());
       }
       // monetizationDetails
       if (j.contains("monetizationDetails")) {
	       const auto &m = j.at("monetizationDetails");
	       if (m.contains("cuepointSchedule")) {
		       const auto &c = m.at("cuepointSchedule");
		       if (c.contains("enabled")) c.at("enabled").get_to(p.monetizationDetails_cuepointSchedule_enabled.emplace());
		       if (c.contains("pauseAdsUntil")) c.at("pauseAdsUntil").get_to(p.monetizationDetails_cuepointSchedule_pauseAdsUntil.emplace());
		       if (c.contains("scheduleStrategy")) c.at("scheduleStrategy").get_to(p.monetizationDetails_cuepointSchedule_scheduleStrategy.emplace());
		       if (c.contains("repeatIntervalSecs")) c.at("repeatIntervalSecs").get_to(p.monetizationDetails_cuepointSchedule_repeatIntervalSecs.emplace());
	       }
       }
}

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
