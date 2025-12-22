add_library(GoogleAuth INTERFACE)
target_include_directories(GoogleAuth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GoogleAuth INTERFACE Async CurlHelper Logger)
target_sources(
  GoogleAuth
  PRIVATE
    GoogleAuthManager.hpp
    GoogleAuthResponse.hpp
    GoogleOAuth2ClientCredentials.hpp
    GoogleOAuth2Flow.hpp
    GoogleTokenState.hpp
)
