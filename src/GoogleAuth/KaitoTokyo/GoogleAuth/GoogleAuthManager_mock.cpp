#include "GoogleAuthManager.hpp"

namespace KaitoTokyo::GoogleAuth {

GoogleAuthManager::GoogleAuthManager(std::shared_ptr<CurlHelper::CurlHandle> curl,
				     const GoogleOAuth2ClientCredentials &clientCredentials,
				     std::shared_ptr<const Logger::ILogger> logger)
	: curl_(std::move(curl)),
	  clientCredentials_(clientCredentials),
	  logger_(std::move(logger))
{
}

GoogleAuthManager::~GoogleAuthManager() noexcept = default;

GoogleAuthResponse GoogleAuthManager::fetchFreshAuthResponse(std::string refreshToken) const
{
	GoogleAuthResponse resp;
	resp.access_token = "mocked_access_token";
	resp.expires_in = 3600;
	resp.token_type = "Bearer";
	resp.refresh_token = refreshToken;
	resp.scope = std::string{"mocked_scope"};
	return resp;
}

} // namespace KaitoTokyo::GoogleAuth
