# gersemi: off
add_library(GoogleAuth_common OBJECT)
target_link_libraries(
  GoogleAuth_common
  PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    nlohmann_json::nlohmann_json
    CurlHelper
    Logger
  PRIVATE
    fmt::fmt
)
target_sources(
  GoogleAuth_common
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
  GoogleAuth_common
  PRIVATE
    KaitoTokyo/GoogleAuth/GoogleAuthResponse.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2ClientCredentials.cpp
    KaitoTokyo/GoogleAuth/GoogleTokenState.cpp
)
add_library(GoogleAuth_mock STATIC)
target_sources(
  GoogleAuth_mock
  PRIVATE
    KaitoTokyo/GoogleAuth/GoogleAuthManager_mock.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow_mock.cpp
)
target_link_libraries(GoogleAuth_mock PUBLIC GoogleAuth_common)

add_library(GoogleAuth_impl STATIC)
target_sources(
  GoogleAuth_impl
  PRIVATE
    KaitoTokyo/GoogleAuth/GoogleAuthManager.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow.cpp
)
target_link_libraries(GoogleAuth_impl PUBLIC GoogleAuth_common)

if(MOCK_GOOGLE_AUTH)
  add_library(GoogleAuth ALIAS GoogleAuth_mock)
else()
  add_library(GoogleAuth ALIAS GoogleAuth_impl)
endif()
# gersemi: on
