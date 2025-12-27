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

} // namespace KaitoTokyo::LiveStreamSegmenter::YouTubeApi
