cmake_minimum_required(VERSION 3.28)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
include(init)

project(${PLUGIN_NAME} VERSION ${PLUGIN_VERSION} LANGUAGES C CXX)

option(BUILD_TESTING "Build test cases" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

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

if(NOT QT6KEYCHAIN_FOUND)
  pkg_check_modules(PC_QT6KEYCHAIN libqt6keychain)
  if(PC_QT6KEYCHAIN_FOUND)
    add_library(Qt6Keychain::Qt6Keychain INTERFACE IMPORTED)
    target_link_libraries(Qt6Keychain::Qt6Keychain INTERFACE ${PC_QT6KEYCHAIN_LIBRARIES})
    target_include_directories(Qt6Keychain::Qt6Keychain INTERFACE ${PC_QT6KEYCHAIN_INCLUDE_DIRS})
    target_compile_definitions(Qt6Keychain::Qt6Keychain INTERFACE ${PC_QT6KEYCHAIN_CFLAGS_OTHER})
    target_link_directories(Qt6Keychain::Qt6Keychain INTERFACE ${PC_QT6KEYCHAIN_LIBRARY_DIRS})
    set(QT6KEYCHAIN_FOUND TRUE)
  endif()
endif()

if(NOT QT6KEYCHAIN_FOUND)
  block()
    set(BUILD_WITH_QT6 ON)
    set(BUILD_TRANSLATIONS OFF)
    set(BUILD_TESTING OFF)
    add_subdirectory(vendor/qtkeychain EXCLUDE_FROM_ALL)
  endblock()
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
  block()
    add_subdirectory(vendor/googletest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
  endblock()
endif()
