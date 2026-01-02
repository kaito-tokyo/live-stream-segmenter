add_library(GoogleAuth OBJECT)
target_include_directories(GoogleAuth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GoogleAuth PUBLIC Async CurlHelper Logger)
target_sources(
  GoogleAuth
  PRIVATE
    KaitoTokyo/GoogleAuth/GoogleAuthManager.cpp
    KaitoTokyo/GoogleAuth/GoogleAuthManager.hpp
    KaitoTokyo/GoogleAuth/GoogleAuthResponse.cpp
    KaitoTokyo/GoogleAuth/GoogleAuthResponse.hpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2ClientCredentials.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2ClientCredentials.hpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow.cpp
    KaitoTokyo/GoogleAuth/GoogleOAuth2Flow.hpp
    KaitoTokyo/GoogleAuth/GoogleTokenState.cpp
    KaitoTokyo/GoogleAuth/GoogleTokenState.hpp
)
