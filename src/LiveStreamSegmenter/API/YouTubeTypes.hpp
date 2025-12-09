#pragma once

#include <string>
#include <vector>

namespace KaitoTokyo::LiveStreamSegmenter::API {

/**
 * @brief ストリームキー情報 (Pure C++ DTO)
 */
struct YouTubeStreamKey {
	std::string id;         // API ID
	std::string title;      // 表示名
	std::string streamName; // RTMP Key

	// 補足情報
	std::string resolution;
	std::string frameRate;

	// デバッグやログ用の文字列表現
	std::string toString() const { return title + " (" + id + ")"; }
};

/**
 * @brief 配信枠作成パラメータ (Pure C++ DTO)
 */
struct BroadcastParams {
	std::string title;
	std::string description;
	std::string scheduledStartTime; // ISO 8601 string
	std::string privacyStatus;      // "private", "unlisted", "public"

	BroadcastParams() : privacyStatus("private") {}
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
