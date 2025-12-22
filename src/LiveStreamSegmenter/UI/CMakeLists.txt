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
    BridgeUtils
    Logger
    GoogleAuth
    ${CMAKE_PROJECT_NAME}_API
    ${CMAKE_PROJECT_NAME}_Store
)
target_sources(
  ${CMAKE_PROJECT_NAME}_UI
  PRIVATE
    fmt_qstring_formatter.hpp
    GoogleOAuth2FlowCallbackServer.hpp
    JsonDropArea.hpp
    SettingsDialog.hpp
    StreamSegmenterDock.hpp
)

target_compile_options(
  ${CMAKE_PROJECT_NAME}_UI
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)

set_target_properties(
  ${CMAKE_PROJECT_NAME}_UI
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON POSITION_INDEPENDENT_CODE ON
)
if(HAVE_IMMINTRIN_H AND UNIX AND NOT APPLE)
  target_compile_options(${CMAKE_PROJECT_NAME}_UI PUBLIC -include immintrin.h)
endif()
