#pragma once

#include <string>
#include <mutex>
#include <memory>
#include "GoogleAuthState.hpp"
#include "TokenStorage.hpp"
#include "../../Logger/ILogger.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

class GoogleTokenManager {
public:
	GoogleTokenManager(std::shared_ptr<Logger::ILogger> logger,
			   std::unique_ptr<TokenStorage> storage = std::make_unique<TokenStorage>());
	~GoogleTokenManager() = default;

	// std::stringを使用
	void setCredentials(const std::string &clientId, const std::string &clientSecret);

	bool isAuthenticated() const;
	std::string currentChannelName() const;

	void updateAuthState(const GoogleAuthState &state);
	void clear();

	std::string getAccessToken();

private:
	void performRefresh();

	mutable std::mutex mutex_;
	GoogleAuthState state_;
	std::string clientId_;
	std::string clientSecret_;

	std::unique_ptr<TokenStorage> storage_;
	std::shared_ptr<Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
