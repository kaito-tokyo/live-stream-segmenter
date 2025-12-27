add_library(${CMAKE_PROJECT_NAME}_Controller STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Controller
  PUBLIC ${CMAKE_PROJECT_NAME}_Store ${CMAKE_PROJECT_NAME}_UI GoogleAuth Logger
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Controller
  PRIVATE MainPluginContext.hpp ProfileContext.hpp YouTubeStreamSegmenter.cpp YouTubeStreamSegmenter.hpp
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}_Controller
  PROPERTIES AUTOMOC ON
)
