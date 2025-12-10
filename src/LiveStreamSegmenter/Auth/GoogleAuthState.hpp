#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

using json = nlohmann::json;

class GoogleAuthState {
public:
	GoogleAuthState() = default;
	~GoogleAuthState() = default;

	bool isAuthorized() const;
	bool isAccessTokenFresh() const;

	void updateFromTokenResponse(const json &j);
	void setUserEmail(const std::string &email);
	void clear();

	json toJson() const;
	static GoogleAuthState fromJson(const json &j);

	// Getters
	std::string accessToken() const { return accessToken_; }
	std::string refreshToken() const { return refreshToken_; }
	std::string email() const { return email_; }

	std::chrono::system_clock::time_point expirationDate() const { return tokenExpiration_; }

private:
	std::string accessToken_;
	std::string refreshToken_;
	std::string email_;
	std::string scope_;

	// 有効期限 (Epoch time 0 means invalid)
	std::chrono::system_clock::time_point tokenExpiration_ = std::chrono::system_clock::time_point();
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
