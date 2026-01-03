add_library(GoogleAuth STATIC)
# gersemi: off
target_link_libraries(
  GoogleAuth
  PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    nlohmann_json::nlohmann_json
    CurlHelper
    Logger
)
target_link_libraries(
  GoogleAuth
  PRIVATE
    fmt::fmt
)
target_sources(
  GoogleAuth
  PUBLIC
  FILE_SET HEADERS
  FILES
    KaitoTokyo/GoogleAuth/GoogleAuthManager.hpp
    KaitoTokyo/GoogleAuth/GoogleAuthResponse.hpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2ClientCredentials.hpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow.hpp
    KaitoTokyo/GoogleAuth/GoogleTokenState.hpp
)
target_sources(
  GoogleAuth
  PRIVATE
    KaitoTokyo/GoogleAuth/GoogleAuthManager.cpp
    KaitoTokyo/GoogleAuth/GoogleAuthResponse.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2ClientCredentials.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow.cpp
    KaitoTokyo/GoogleAuth/GoogleTokenState.cpp
)
# gersemi: on
