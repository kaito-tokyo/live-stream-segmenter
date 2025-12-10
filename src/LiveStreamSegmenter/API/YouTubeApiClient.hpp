#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "YouTubeTypes.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::API {

class YouTubeApiClient {
public:
	using AccessTokenFunc = std::function<std::future<std::string>()>;

	YouTubeApiClient(AccessTokenFunc accessTokenFunc) : accessTokenFunc_(std::move(accessTokenFunc)) {}
	~YouTubeApiClient() = default;

	std::vector<YouTubeStreamKey> listStreamKeys();

private:
	AccessTokenFunc accessTokenFunc_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
