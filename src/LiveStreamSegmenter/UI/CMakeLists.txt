add_library(${CMAKE_PROJECT_NAME}_UI STATIC JsonDropArea.cpp SettingsDialog.cpp StreamSegmenterDock.cpp)
target_include_directories(${CMAKE_PROJECT_NAME}_UI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_UI
  PUBLIC
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ObsBridgeUtils
    Logger
    GoogleAuth
    YouTubeApi
    ${CMAKE_PROJECT_NAME}_Store
    ${CMAKE_PROJECT_NAME}_Scripting
)
target_sources(
  ${CMAKE_PROJECT_NAME}_UI
  PRIVATE
    GoogleOAuth2FlowCallbackServer.cpp
    GoogleOAuth2FlowCallbackServer.hpp
    JsonDropArea.cpp
    JsonDropArea.hpp
    SettingsDialog.cpp
    SettingsDialog.hpp
    StreamSegmenterDock.cpp
    StreamSegmenterDock.hpp
    fmt_qstring_formatter.hpp
)

target_compile_options(
  ${CMAKE_PROJECT_NAME}_UI
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
set_target_properties(${CMAKE_PROJECT_NAME}_UI PROPERTIES AUTOMOC ON AUTORCC ON)
