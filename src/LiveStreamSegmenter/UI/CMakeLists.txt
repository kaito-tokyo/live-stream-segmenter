# gersemi: off
add_library(${CMAKE_PROJECT_NAME}_UI STATIC)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_UI
  PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    OBS::libobs
    OBS::obs-frontend-api
    GoogleAuth
    Logger
    ObsBridgeUtils
    YouTubeApi
    ${CMAKE_PROJECT_NAME}_Store
    ${CMAKE_PROJECT_NAME}_Scripting
)
target_sources(
  ${CMAKE_PROJECT_NAME}_UI
  PUBLIC
  FILE_SET HEADERS
  FILES
    GoogleOAuth2FlowCallbackServer.hpp
    JsonDropArea.hpp
    SettingsDialog.hpp
    StreamSegmenterDock.hpp
    fmt_qstring_formatter.hpp
)
target_sources(
  ${CMAKE_PROJECT_NAME}_UI
  PRIVATE
    GoogleOAuth2FlowCallbackServer.cpp
    JsonDropArea.cpp
    SettingsDialog.cpp
    StreamSegmenterDock.cpp
)
set_target_properties(${CMAKE_PROJECT_NAME}_UI PROPERTIES AUTOMOC ON)
# gersemi: on
