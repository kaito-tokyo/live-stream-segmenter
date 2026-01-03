add_library(YouTubeApi STATIC)
# gersemi: off
target_link_libraries(
  YouTubeApi
  PUBLIC
    nlohmann_json::nlohmann_json
    CurlHelper
    Logger
  PRIVATE
    CURL::libcurl
)
target_sources(
  YouTubeApi
  PUBLIC
  FILE_SET HEADERS
  FILES
    KaitoTokyo/YouTubeApi/YouTubeApiClient.hpp
    KaitoTokyo/YouTubeApi/YouTubeTypes.hpp
)
target_sources(
  YouTubeApi
  PRIVATE
    KaitoTokyo/YouTubeApi/YouTubeApiClient.cpp
    KaitoTokyo/YouTubeApi/YouTubeTypes.cpp
)
# gersemi: on
