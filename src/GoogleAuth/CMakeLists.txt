add_library(GoogleAuth OBJECT)
target_include_directories(GoogleAuth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GoogleAuth PUBLIC Async CurlHelper Logger)
target_sources(
  GoogleAuth
  PRIVATE
    GoogleAuthManager.cpp
    GoogleAuthManager.hpp
    GoogleAuthResponse.cpp
    GoogleAuthResponse.hpp
    GoogleOAuth2ClientCredentials.cpp
    GoogleOAuth2ClientCredentials.hpp
    GoogleOAuth2Flow.cpp
    GoogleOAuth2Flow.hpp
    GoogleTokenState.cpp
    GoogleTokenState.hpp
)
