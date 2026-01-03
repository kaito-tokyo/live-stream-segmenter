# gersemi: off
add_library(${CMAKE_PROJECT_NAME}_Controller STATIC)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Controller
  PUBLIC
    Qt6::Core
    OBS::obs-frontend-api
    Async
    AsyncQt
    GoogleAuth
    CurlHelper
    Logger
    ObsBridgeUtils
    YouTubeApi
    ${CMAKE_PROJECT_NAME}_Scripting
    ${CMAKE_PROJECT_NAME}_Store
    ${CMAKE_PROJECT_NAME}_UI
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Controller
  PUBLIC
  FILE_SET HEADERS
  FILES
    MainPluginContext.hpp
    ProfileContext.hpp
    YouTubeStreamSegmenterMainLoop.hpp
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Controller
  PRIVATE
    MainPluginContext.cpp
    ProfileContext.cpp
    YouTubeStreamSegmenterMainLoop.cpp
)
set_target_properties(${CMAKE_PROJECT_NAME}_Controller PROPERTIES AUTOMOC ON)
# gersemi: on
