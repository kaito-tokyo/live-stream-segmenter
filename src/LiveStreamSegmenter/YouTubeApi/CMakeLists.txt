add_library(${CMAKE_PROJECT_NAME}_YouTubeApi INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_YouTubeApi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_YouTubeApi INTERFACE ${JTHREAD_LIBRARIES} CurlHelper Logger)
target_sources(${CMAKE_PROJECT_NAME}_YouTubeApi PRIVATE YouTubeApiClient.hpp YouTubeTypes.hpp)
