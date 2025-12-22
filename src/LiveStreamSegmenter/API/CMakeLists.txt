add_library(${CMAKE_PROJECT_NAME}_API INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_API INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_API INTERFACE CurlHelper Logger)
target_sources(${CMAKE_PROJECT_NAME}_API PRIVATE YouTubeApiClient.hpp YouTubeTypes.hpp)
