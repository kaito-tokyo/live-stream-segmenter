cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(BUILD_TESTING "Build tests" OFF)
option(USE_PKGCONFIG "Use pkg-config to find dependencies" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

if(VCPKG_TARGET_TRIPLET)
  list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

  find_package(Backward CONFIG)

  find_package(CURL CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)
  find_package(httplib CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)
elseif(USE_PKGCONFIG)
  find_package(PkgConfig REQUIRED)

  # --- CURL ---
  pkg_check_modules(PC_CURL REQUIRED libcurl)
  add_library(CURL::libcurl INTERFACE IMPORTED)
  target_link_libraries(CURL::libcurl INTERFACE ${PC_CURL_LIBRARIES})
  target_include_directories(CURL::libcurl INTERFACE ${PC_CURL_INCLUDE_DIRS})
  target_compile_definitions(CURL::libcurl INTERFACE ${PC_CURL_CFLAGS_OTHER})
  target_link_directories(CURL::libcurl INTERFACE ${PC_CURL_LIBRARY_DIRS})

  # --- fmt ---
  pkg_check_modules(PC_FMT REQUIRED fmt)
  add_library(fmt::fmt INTERFACE IMPORTED)
  target_link_libraries(fmt::fmt INTERFACE ${PC_FMT_LIBRARIES})
  target_include_directories(fmt::fmt INTERFACE ${PC_FMT_INCLUDE_DIRS})
  target_compile_definitions(fmt::fmt INTERFACE ${PC_FMT_CFLAGS_OTHER})
  target_link_directories(fmt::fmt INTERFACE ${PC_FMT_LIBRARY_DIRS})

  # --- httplib ---
  find_package(httplib CONFIG REQUIRED)

  # --- nlohmann_json ---
  pkg_check_modules(PC_NLOHMANN_JSON REQUIRED nlohmann_json)
  add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
  target_link_libraries(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_LIBRARIES})
  target_include_directories(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_INCLUDE_DIRS})
  target_compile_definitions(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_CFLAGS_OTHER})
  target_link_directories(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_LIBRARY_DIRS})
else()
  message(FATAL_ERROR "Either USE_PKGCONFIG or VCPKG_TARGET_TRIPLET must be set.")
endif()

add_library(Logger INTERFACE)
target_include_directories(Logger INTERFACE ${CMAKE_SOURCE_DIR}/src/Logger)
target_link_libraries(Logger INTERFACE fmt::fmt)
target_sources(Logger PRIVATE src/Logger/ILogger.hpp)

add_library(BridgeUtils INTERFACE)
target_include_directories(BridgeUtils INTERFACE ${CMAKE_SOURCE_DIR}/src/BridgeUtils)
target_link_libraries(BridgeUtils INTERFACE OBS::libobs Logger)
target_sources(
  BridgeUtils
  PRIVATE
    src/BridgeUtils/AsyncTextureReader.hpp
    src/BridgeUtils/GsUnique.hpp
    src/BridgeUtils/ObsLogger.hpp
    src/BridgeUtils/ObsUnique.hpp
)

add_library(CurlHelper INTERFACE)
target_include_directories(CurlHelper INTERFACE ${CMAKE_SOURCE_DIR}/src/CurlHelper)
target_link_libraries(CurlHelper INTERFACE CURL::libcurl)
target_sources(
  CurlHelper
  PRIVATE src/CurlHelper/CurlUrlHandler.hpp src/CurlHelper/CurlUrlSearchParams.hpp src/CurlHelper/CurlVectorWriter.hpp
)

add_library(UpdateChecker INTERFACE)
target_include_directories(UpdateChecker INTERFACE ${CMAKE_SOURCE_DIR}/src/UpdateChecker)
target_link_libraries(UpdateChecker INTERFACE CurlHelper)
target_sources(UpdateChecker PRIVATE src/UpdateChecker/UpdateChecker.hpp)

# --- Plugin modules ---

add_library(${CMAKE_PROJECT_NAME}_Auth INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_Auth INTERFACE ${CMAKE_SOURCE_DIR}/src/LiveStreamSegmenter/Auth)
target_link_libraries(${CMAKE_PROJECT_NAME}_Auth INTERFACE httplib::httplib CurlHelper Logger)
target_sources(
  ${CMAKE_PROJECT_NAME}_Auth
  PRIVATE
    src/LiveStreamSegmenter/Auth/GoogleAuthManager.hpp
    src/LiveStreamSegmenter/Auth/GoogleAuthResponse.hpp
    src/LiveStreamSegmenter/Auth/GoogleOAuth2Flow.hpp
    src/LiveStreamSegmenter/Auth/GoogleTokenState.hpp
)

add_library(${CMAKE_PROJECT_NAME}_API INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_API INTERFACE ${CMAKE_SOURCE_DIR}/src/LiveStreamSegmenter/API)
target_link_libraries(${CMAKE_PROJECT_NAME}_API INTERFACE CurlHelper Logger)
target_sources(
  ${CMAKE_PROJECT_NAME}_API
  PRIVATE src/LiveStreamSegmenter/API/YouTubeApiClient.hpp src/LiveStreamSegmenter/API/YouTubeTypes.hpp
)

add_library(${CMAKE_PROJECT_NAME}_Service INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_Service INTERFACE ${CMAKE_SOURCE_DIR}/src/LiveStreamSegmenter/Service)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Service
  INTERFACE OBS::libobs OBS::obs-frontend-api Logger ${CMAKE_PROJECT_NAME}_Auth
)
target_sources(${CMAKE_PROJECT_NAME}_Service PRIVATE src/LiveStreamSegmenter/Service/AuthService.hpp)

add_library(${CMAKE_PROJECT_NAME}_UI STATIC src/LiveStreamSegmenter/UI/StreamSegmenterDock.cpp)
target_include_directories(${CMAKE_PROJECT_NAME}_UI PUBLIC ${CMAKE_SOURCE_DIR}/src/LiveStreamSegmenter/UI)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_UI
  PUBLIC
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
    BridgeUtils
    Logger
    ${CMAKE_PROJECT_NAME}_Auth
    ${CMAKE_PROJECT_NAME}_API
    ${CMAKE_PROJECT_NAME}_Service
)
target_sources(${CMAKE_PROJECT_NAME}_UI PRIVATE src/LiveStreamSegmenter/UI/StreamSegmenterDock.hpp)

add_library(${CMAKE_PROJECT_NAME}_Controller INTERFACE)
target_include_directories(
  ${CMAKE_PROJECT_NAME}_Controller
  INTERFACE ${CMAKE_SOURCE_DIR}/src/LiveStreamSegmenter/Controller
)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Controller
  INTERFACE ${CMAKE_PROJECT_NAME}_Auth ${CMAKE_PROJECT_NAME}_Service ${CMAKE_PROJECT_NAME}_UI Logger
)
target_sources(${CMAKE_PROJECT_NAME}_Controller PRIVATE src/LiveStreamSegmenter/Controller/MainPluginContext.hpp)

foreach(TARGET ${CMAKE_PROJECT_NAME}_UI ${CMAKE_PROJECT_NAME})
  target_compile_options(
    ${TARGET}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  target_compile_definitions(
    ${TARGET}
    PUBLIC PLUGIN_NAME="${CMAKE_PROJECT_NAME}" PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}" NOMINMAX
  )
  set_target_properties(
    ${TARGET}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON POSITION_INDEPENDENT_CODE ON
  )
endforeach()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC OBS::libobs OBS::obs-frontend-api ${CMAKE_PROJECT_NAME}_Controller)

if(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/cmake/macos/exported_symbols_list.txt"
  )
elseif(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/cmake/windows/export_symbols.def")
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_SOURCE_DIR}/cmake/linux/version_script.map"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(vendor/googletest)
  add_subdirectory(tests)
endif()
