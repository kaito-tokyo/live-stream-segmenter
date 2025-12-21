add_subdirectory(Logger)
add_subdirectory(BridgeUtils)
add_subdirectory(CurlHelper)
add_subdirectory(UpdateChecker)
add_subdirectory(LiveStreamSegmenter)

add_library(${CMAKE_PROJECT_NAME} MODULE)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE plugin-main.cpp)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PUBLIC OBS::libobs OBS::obs-frontend-api ${CMAKE_PROJECT_NAME}_Controller Logger BridgeUtils
)

target_compile_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)
target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PUBLIC PLUGIN_NAME="${CMAKE_PROJECT_NAME}" PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}"
)
set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON POSITION_INDEPENDENT_CODE ON
)
if(HAVE_IMMINTRIN_H AND UNIX AND NOT APPLE)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -include immintrin.h)
endif()

if(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exported_symbols_macos.txt"
  )
elseif(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/exported_symbols_macos_windows.def")
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/exported_symbols_linux.map"
  )
endif()
