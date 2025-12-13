/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT" 
 * in the distribution root.
 */

#pragma once

#include <string>

namespace KaitoTokyo::LiveStreamSegmenter::API {

/**
 * @brief Represents a YouTube stream key and its associated metadata.
 * Contains stream ID, title, stream name, resolution, and frame rate.
 */
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

} // namespace KaitoTokyo::LiveStreamSegmenter::API
