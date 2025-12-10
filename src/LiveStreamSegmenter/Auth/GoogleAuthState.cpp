#include "GoogleAuthState.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

bool GoogleAuthState::isAuthorized() const
{
	return !refreshToken_.empty();
}

bool GoogleAuthState::isAccessTokenFresh() const
{
	if (accessToken_.empty() || tokenExpiration_.time_since_epoch().count() == 0) {
		return false;
	}

	// 60秒のマージン
	auto now = std::chrono::system_clock::now();
	auto margin = std::chrono::seconds(60);
	return (now + margin) < tokenExpiration_;
}

void GoogleAuthState::updateFromTokenResponse(const json &j)
{
	if (j.contains("access_token")) {
		accessToken_ = j["access_token"].get<std::string>();
	}

	if (j.contains("refresh_token")) {
		std::string newRefresh = j["refresh_token"].get<std::string>();
		if (!newRefresh.empty()) {
			refreshToken_ = newRefresh;
		}
	}

	if (j.contains("expires_in")) {
		int expiresInSeconds = j["expires_in"].get<int>();
		tokenExpiration_ = std::chrono::system_clock::now() + std::chrono::seconds(expiresInSeconds);
	}

	if (j.contains("scope")) {
		scope_ = j["scope"].get<std::string>();
	}
}

void GoogleAuthState::setUserEmail(const std::string &email)
{
	email_ = email;
}

void GoogleAuthState::clear()
{
	accessToken_.clear();
	refreshToken_.clear();
	email_.clear();
	scope_.clear();
	tokenExpiration_ = std::chrono::system_clock::time_point();
}

json GoogleAuthState::toJson() const
{
	json j;
	j["refresh_token"] = refreshToken_;
	j["email"] = email_;
	j["access_token"] = accessToken_;
	j["scope"] = scope_;

	if (tokenExpiration_.time_since_epoch().count() != 0) {
		auto expiresAt =
			std::chrono::duration_cast<std::chrono::seconds>(tokenExpiration_.time_since_epoch()).count();
		j["expires_at"] = expiresAt;
	}

	return j;
}

GoogleAuthState GoogleAuthState::fromJson(const json &j)
{
	GoogleAuthState state;
	state.refreshToken_ = j.value("refresh_token", "");
	state.email_ = j.value("email", "");
	state.accessToken_ = j.value("access_token", "");
	state.scope_ = j.value("scope", "");

	if (j.contains("expires_at")) {
		int64_t expiresAt = j["expires_at"].get<int64_t>();
		state.tokenExpiration_ = std::chrono::system_clock::time_point(std::chrono::seconds(expiresAt));
	}

	return state;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
