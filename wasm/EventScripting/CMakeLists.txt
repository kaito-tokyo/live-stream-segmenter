cmake_minimum_required(VERSION 3.28)

project(EventScripting)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

include(FetchContent)

# --- fmt ---
FetchContent_Declare(
  fmt
  URL https://github.com/fmtlib/fmt/archive/refs/tags/12.1.0.tar.gz
  URL_HASH SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
)
FetchContent_MakeAvailable(fmt)

# --- SQLite3 ---
FetchContent_Declare(
  sqlite_amalgamation
  URL https://www.sqlite.org/2025/sqlite-amalgamation-3510100.zip
  URL_HASH SHA256=84a85d6a1b920234349f01720912c12391a4f0cb5cb998087e641dee3ef8ef2e
)
FetchContent_MakeAvailable(sqlite_amalgamation)

add_library(sqlite3 STATIC "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c")
target_include_directories(sqlite3 PUBLIC "${sqlite_amalgamation_SOURCE_DIR}")
target_compile_options(
  sqlite3
  PRIVATE
    -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_DISABLE_LFS
    -DSQLITE_OMIT_COMPLETE
    -DSQLITE_OMIT_DEPRECATED
    -DSQLITE_OMIT_LOAD_EXTENSION
    -DSQLITE_OMIT_PROGRESS_CALLBACK
    -DSQLITE_OMIT_SHARED_CACHE
    -DSQLITE_OMIT_UTF16
    -DSQLITE_TEMP_STORE=2
    -DSQLITE_THREADSAFE=0
)
add_library(unofficial::sqlite3::sqlite3 ALIAS sqlite3)

# --- quickjs-ng ---
FetchContent_Declare(
  quickjs-ng
  URL https://github.com/quickjs-ng/quickjs/archive/refs/tags/v0.11.0.tar.gz
  URL_HASH SHA256=b456e6aa05522eed9cbf9dec1e947ba1ba6578fd09386391e581339ddabaa641
)
FetchContent_MakeAvailable(quickjs-ng)
add_library(qjs::qjs ALIAS qjs)

add_subdirectory(../../src/Logger Logger EXCLUDE_FROM_ALL)
add_subdirectory(../../src/LiveStreamSegmenter/Scripting Scripting EXCLUDE_FROM_ALL)

add_executable(${CMAKE_PROJECT_NAME} EventScripting.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_Scripting)
target_link_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    -sSTACK_SIZE=2097152
    --bind
    -sMODULARIZE=1
    -sEXPORT_NAME='createEventScripting'
    -sALLOW_MEMORY_GROWTH=1
    -sEXPORT_ES6=1
)
