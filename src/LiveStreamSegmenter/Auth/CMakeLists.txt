add_library(${CMAKE_PROJECT_NAME}_Auth INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_Auth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_Auth INTERFACE httplib::httplib CurlHelper Logger)
target_sources(
  ${CMAKE_PROJECT_NAME}_Auth
  PRIVATE
    GoogleAuthManager.hpp
    GoogleAuthResponse.hpp
    GoogleOAuth2ClientCredentials.hpp
    GoogleOAuth2Flow.hpp
    GoogleTokenState.hpp
)
