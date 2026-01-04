cmake_minimum_required(VERSION 3.28)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
include(init)

project(${PLUGIN_NAME} VERSION ${PLUGIN_VERSION} LANGUAGES C CXX)

option(BUILD_TESTING "Build test cases" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(ENABLE_FETCH_CONTENT "Enable content fetching features" OFF)
option(MOCK_GOOGLE_AUTH "Enable mocking for GoogleAuth in tests" OFF)
option(MOCK_YOUTUBE_API "Enable mocking for YouTubeApi in tests" OFF)

if(MSVC)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
elseif(APPLE)
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
else()
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
endif()

include(compilerconfig)
include(defaults)
include(helpers)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BUILD_SHARED_LIBS OFF)

if(MSVC)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
  set(PLUGIN_COMPILE_OPTIONS /experimental:deterministic "/pathmap:${CMAKE_SOURCE_DIR}=.")
else()
  add_compile_options(-fno-omit-frame-pointer -fstack-protector-strong)
  set(PLUGIN_COMPILE_OPTIONS "-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.")
endif()

if(MSVC)
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-x64"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-x64"
  )
elseif(APPLE)
  list(
    PREPEND
    CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/Frameworks"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-universal"
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-universal"
  )
endif()

if(ENABLE_ASAN)
  if(MSVC)
    message(
      WARNING
      "ENABLE_ASAN is enabled, but ASan flags are not applied for MSVC due to limited support and incompatible flag syntax."
    )
  else()
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
endif()

if(ENABLE_UBSAN)
  if(MSVC)
    message(
      WARNING
      "ENABLE_UBSAN is enabled, but UBSan flags are not applied for MSVC due to limited support and incompatible flag syntax."
    )
  else()
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Network Widgets Core)

if(VCPKG_TARGET_TRIPLET)
  list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
endif()

find_package(PkgConfig QUIET)

# --- CURL ---
find_package(CURL CONFIG QUIET)
if(NOT CURL_FOUND)
  find_package(CURL QUIET)
endif()

if(NOT CURL_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_CURL libcurl)
  if(PC_CURL_FOUND)
    add_library(CURL::libcurl INTERFACE IMPORTED)
    target_link_libraries(CURL::libcurl INTERFACE ${PC_CURL_LIBRARIES})
    target_include_directories(CURL::libcurl INTERFACE ${PC_CURL_INCLUDE_DIRS})
    target_compile_definitions(CURL::libcurl INTERFACE ${PC_CURL_CFLAGS_OTHER})
    target_link_directories(CURL::libcurl INTERFACE ${PC_CURL_LIBRARY_DIRS})
    set(CURL_FOUND TRUE)
  endif()
endif()

if(NOT CURL_FOUND)
  message(FATAL_ERROR "CURL not found via CMake Config or PkgConfig.")
endif()

# --- fmt ---
find_package(fmt CONFIG QUIET)

if(NOT fmt_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_FMT fmt)
  if(PC_FMT_FOUND)
    add_library(fmt::fmt INTERFACE IMPORTED)
    target_link_libraries(fmt::fmt INTERFACE ${PC_FMT_LIBRARIES})
    target_include_directories(fmt::fmt INTERFACE ${PC_FMT_INCLUDE_DIRS})
    target_compile_definitions(fmt::fmt INTERFACE ${PC_FMT_CFLAGS_OTHER})
    target_link_directories(fmt::fmt INTERFACE ${PC_FMT_LIBRARY_DIRS})
    set(fmt_FOUND TRUE)
  endif()
endif()

if(NOT fmt_FOUND)
  message(FATAL_ERROR "fmt not found via CMake Config or PkgConfig.")
endif()

# --- nlohmann_json ---
find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_NLOHMANN_JSON nlohmann_json)
  if(PC_NLOHMANN_JSON_FOUND)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    target_link_libraries(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_LIBRARIES})
    target_include_directories(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_INCLUDE_DIRS})
    target_compile_definitions(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_CFLAGS_OTHER})
    target_link_directories(nlohmann_json::nlohmann_json INTERFACE ${PC_NLOHMANN_JSON_LIBRARY_DIRS})
    set(nlohmann_json_FOUND TRUE)
  endif()
endif()

if(NOT nlohmann_json_FOUND)
  message(FATAL_ERROR "nlohmann_json not found via CMake Config or PkgConfig.")
endif()

# --- qjs (QuickJS) ---
find_package(qjs NAMES qjs quickjs CONFIG QUIET)

if(NOT qjs_FOUND AND PkgConfig_FOUND)
  pkg_check_modules(PC_QJS qjs)
  if(PC_QJS_FOUND)
    add_library(qjs INTERFACE IMPORTED)
    target_link_libraries(qjs INTERFACE ${PC_QJS_LIBRARIES})
    target_include_directories(qjs INTERFACE ${PC_QJS_INCLUDE_DIRS})
    target_compile_definitions(qjs INTERFACE ${PC_QJS_CFLAGS_OTHER})
    target_link_directories(qjs INTERFACE ${PC_QJS_LIBRARY_DIRS})
    set(qjs_FOUND TRUE)
  endif()
endif()

if(NOT qjs_FOUND)
  message(FATAL_ERROR "qjs not found.")
endif()

if(TARGET qjs AND NOT TARGET qjs::qjs)
  add_library(qjs::qjs ALIAS qjs)
endif()

# --- sqlite3 ---
find_package(unofficial-sqlite3 CONFIG QUIET)

if(NOT unofficial-sqlite3_FOUND)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_SQLITE3 sqlite3)
    if(PC_SQLITE3_FOUND)
      add_library(unofficial::sqlite3::sqlite3 INTERFACE IMPORTED)
      target_link_libraries(unofficial::sqlite3::sqlite3 INTERFACE ${PC_SQLITE3_LIBRARIES})
      target_include_directories(unofficial::sqlite3::sqlite3 INTERFACE ${PC_SQLITE3_INCLUDE_DIRS})
      target_compile_definitions(unofficial::sqlite3::sqlite3 INTERFACE ${PC_SQLITE3_CFLAGS_OTHER})
      target_link_directories(unofficial::sqlite3::sqlite3 INTERFACE ${PC_SQLITE3_LIBRARY_DIRS})
      set(unofficial-sqlite3_FOUND TRUE)
    endif()
  endif()
endif()

if(NOT unofficial-sqlite3_FOUND)
  message(FATAL_ERROR "unofficial-sqlite3 or sqlite3 not found.")
endif()

# --- qtkeychain ---
find_package(Qt6Keychain CONFIG QUIET)

if(NOT Qt6Keychain_FOUND AND ENABLE_FETCH_CONTENT)
  include(FetchContent)
  FetchContent_Declare(
    qtkeychain
    URL https://github.com/frankosterfeld/qtkeychain/archive/refs/tags/0.15.0.tar.gz
    URL_HASH SHA256=f4254dc8f0933b06d90672d683eab08ef770acd8336e44dfa030ce041dc2ca22
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  set(old_BUILD_TESTING ${BUILD_TESTING})
  block()
    if(APPLE)
      add_compile_options(-Wno-quoted-include-in-framework-header -fno-objc-arc)
    elseif(NOT MSVC)
      add_compile_options(-Wno-cast-function-type)
    endif()
    set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
    set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(qtkeychain)
  endblock()
  set(BUILD_TESTING ${old_BUILD_TESTING})
  set(Qt6Keychain_FOUND ON)
endif()

if(NOT Qt6Keychain_FOUND)
  message(FATAL_ERROR "Qt6Keychain not found.")
endif()

if(TARGET qt6keychain)
  if(APPLE)
    target_compile_options(qt6keychain PRIVATE -Wno-quoted-include-in-framework-header -fno-objc-arc)
  elseif(MSVC)
  else()
    target_compile_options(qt6keychain PRIVATE -Wno-cast-function-type)
  endif()

  add_library(Qt6Keychain::Qt6Keychain ALIAS qt6keychain)
endif()

# --- QCoro ---
find_package(QCoro6 CONFIG QUIET COMPONENTS Core)

if(NOT QCoro6_FOUND AND ENABLE_FETCH_CONTENT)
  include(FetchContent)
  FetchContent_Declare(
    QCoro
    URL https://github.com/qcoro/qcoro/archive/refs/tags/v0.12.0.tar.gz
    URL_HASH SHA256=809afafab61593f994c005ca6e242300e1e3e7f4db8b5d41f8c642aab9450fbc
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  block()
    set(QCORO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QCORO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTDBUS OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTNETWORK OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTWEBSOCKETS OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTQUICK OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QML OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTTEST OFF CACHE BOOL "" FORCE)
    if(NOT MSVC)
      add_compile_options(-Wno-newline-eof)
    endif()
    FetchContent_MakeAvailable(QCoro)
  endblock()
  set(QCoro6_FOUND ON)
endif()

if(NOT QCoro6_FOUND)
  message(FATAL_ERROR "QCoro6 not found.")
endif()
add_subdirectory(src)

add_library(${CMAKE_PROJECT_NAME} MODULE)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp)
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE OBS::libobs OBS::obs-frontend-api Logger ${CMAKE_PROJECT_NAME}_Controller
)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}")

if(APPLE)
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_macos.txt"
  )
elseif(MSVC)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_windows.def")
else()
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/src/exported_symbols_linux.map"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  block()
    FetchContent_MakeAvailable(googletest)
  endblock()

  add_subdirectory(tests)
endif()
