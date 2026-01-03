# gersemi: off
add_library(YouTubeApi_common OBJECT)
target_link_libraries(
  YouTubeApi_common
  PUBLIC
    nlohmann_json::nlohmann_json
    CurlHelper
    Logger
  PRIVATE
    CURL::libcurl
)
target_sources(
  YouTubeApi_common
  PUBLIC
  FILE_SET HEADERS
  FILES
    KaitoTokyo/YouTubeApi/YouTubeApiClient.hpp
    KaitoTokyo/YouTubeApi/YouTubeTypes.hpp
)
target_sources(
  YouTubeApi_common
  PRIVATE
    KaitoTokyo/YouTubeApi/YouTubeTypes.cpp
)

add_library(YouTubeApi_mock STATIC)
target_sources(
  YouTubeApi_mock
  PRIVATE
    KaitoTokyo/YouTubeApi/YouTubeApiClient_mock.cpp
)
target_link_libraries(YouTubeApi_mock PUBLIC YouTubeApi_common)

add_library(YouTubeApi_impl STATIC)
target_sources(
  YouTubeApi_impl
  PRIVATE
    KaitoTokyo/YouTubeApi/YouTubeApiClient.cpp
)
target_link_libraries(YouTubeApi_impl PUBLIC YouTubeApi_common)

if(MOCK_YOUTUBE_API)
  add_library(YouTubeApi ALIAS YouTubeApi_mock)
else()
  add_library(YouTubeApi ALIAS YouTubeApi_impl)
endif()
# gersemi: on
