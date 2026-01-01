add_library(${CMAKE_PROJECT_NAME}_Controller OBJECT)
target_include_directories(${CMAKE_PROJECT_NAME}_Controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Controller
  PUBLIC ${CMAKE_PROJECT_NAME}_Store ${CMAKE_PROJECT_NAME}_UI GoogleAuth Logger Async AsyncQt
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Controller
  PRIVATE
    MainPluginContext.cpp
    MainPluginContext.hpp
    ProfileContext.cpp
    ProfileContext.hpp
    YouTubeStreamSegmenterMainLoop.cpp
    YouTubeStreamSegmenterMainLoop.hpp
)
set_target_properties(${CMAKE_PROJECT_NAME}_Controller PROPERTIES AUTOMOC ON)
