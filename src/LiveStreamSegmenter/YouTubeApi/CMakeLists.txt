add_library(${CMAKE_PROJECT_NAME}_YouTubeApi STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_YouTubeApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_YouTubeApi PUBLIC ${JTHREAD_LIBRARIES} CurlHelper Logger)
target_sources(
  ${CMAKE_PROJECT_NAME}_YouTubeApi
  PRIVATE YouTubeApiClient.cpp YouTubeApiClient.hpp YouTubeTypes.cpp YouTubeTypes.hpp
)
