add_library(YouTubeApi OBJECT)
target_include_directories(YouTubeApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(YouTubeApi PUBLIC CurlHelper Logger)
target_sources(
  YouTubeApi
  PRIVATE YouTubeApiClient.cpp YouTubeApiClient.hpp YouTubeTypes.cpp YouTubeTypes.hpp
)
