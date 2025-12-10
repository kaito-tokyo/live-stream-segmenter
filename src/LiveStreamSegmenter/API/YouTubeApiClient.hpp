#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::API {
class YouTubeApiClient {
public:
	YouTubeApiClient() = default;
	~YouTubeApiClient() = default;

private:
	std::string doGet(const char *url, const std::string &accessToken) const;
    std::vector<nlohmann::json> performList(const std::string &url, const std::string &accessToken) const;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
