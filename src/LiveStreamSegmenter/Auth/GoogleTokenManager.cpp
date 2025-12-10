#include "GoogleTokenManager.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <vector>

namespace KaitoTokyo::LiveStreamSegmenter::Auth {

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	auto *str = static_cast<std::string *>(userp);
	str->append(static_cast<char *>(contents), realsize);
	return realsize;
}

GoogleTokenManager::GoogleTokenManager(std::shared_ptr<Logger::ILogger> logger, std::unique_ptr<TokenStorage> storage)
	: storage_(std::move(storage)),
	  logger_(std::move(logger))
{
	if (auto loadedState = storage_->load()) {
		state_ = *loadedState;
	}
}

void GoogleTokenManager::setCredentials(const std::string &clientId, const std::string &clientSecret)
{
	std::lock_guard<std::mutex> lock(mutex_);
	clientId_ = clientId;
	clientSecret_ = clientSecret;
}

bool GoogleTokenManager::isAuthenticated() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return state_.isAuthorized();
}

std::string GoogleTokenManager::currentChannelName() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return state_.email();
}

void GoogleTokenManager::updateAuthState(const GoogleAuthState &newState)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		state_ = newState;
	}
	storage_->save(newState);
}

void GoogleTokenManager::clear()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		state_.clear();
	}
	storage_->clear();
}

std::string GoogleTokenManager::getAccessToken()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!state_.isAuthorized()) {
			throw std::runtime_error("Not logged in.");
		}
		if (state_.isAccessTokenFresh()) {
			return state_.accessToken();
		}
		if (clientId_.empty() || clientSecret_.empty()) {
			throw std::runtime_error("Client ID/Secret missing for refresh.");
		}
	}

	performRefresh();

	std::lock_guard<std::mutex> lock(mutex_);
	return state_.accessToken();
}

void GoogleTokenManager::performRefresh()
{
	std::string cid, csec, rtoken;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		cid = clientId_;
		csec = clientSecret_;
		rtoken = state_.refreshToken();
	}

	CURL *curl = curl_easy_init();
	if (!curl)
		throw std::runtime_error("Failed to init curl");

	// URL Encoding helper
	auto urlEncode = [&](const std::string &s) -> std::string {
		char *output = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.length()));
		if (output) {
			std::string result(output);
			curl_free(output);
			return result;
		}
		return "";
	};

	std::string readBuffer;
	std::string postDataStr = "client_id=" + urlEncode(cid) + "&client_secret=" + urlEncode(csec) +
				  "&refresh_token=" + urlEncode(rtoken) + "&grant_type=refresh_token";

	curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postDataStr.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		std::string err = curl_easy_strerror(res);
		curl_easy_cleanup(curl);
		throw std::runtime_error("Network error during refresh: " + err);
	}
	curl_easy_cleanup(curl);

	json j = json::parse(readBuffer, nullptr, false);
	if (j.is_discarded()) {
		throw std::runtime_error("JSON parse error during refresh");
	}

	if (j.contains("error")) {
		std::string err = j["error_description"].get<std::string>();
		logger_->error("[TokenManager] Refresh failed: {}", err);

		if (j["error"].get<std::string>() == "invalid_grant") {
			clear();
			throw std::runtime_error("Session expired. Please login again.");
		}
		throw std::runtime_error("API Error: " + err);
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		state_.updateFromTokenResponse(j);
		storage_->save(state_);
	}
	logger_->info("[TokenManager] Token refreshed.");
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Auth
