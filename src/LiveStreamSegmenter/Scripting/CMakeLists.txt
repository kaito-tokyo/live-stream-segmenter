add_library(${CMAKE_PROJECT_NAME}_Scripting OBJECT)
# gersemi: off
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Scripting
  PUBLIC
    qjs::qjs
    unofficial::sqlite3::sqlite3
    fmt::fmt
    Logger
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Scripting
  PUBLIC
  FILE_SET HEADERS
  FILES
    EventScriptingContext.hpp
    ScriptingDatabase.hpp
    ScriptingRuntime.hpp
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Scripting
  PRIVATE
    dayjs_bundle.c
    EventScriptingContext.cpp
    ini_bundle.c
    localstorage_bundle.c
    ScriptingDatabase.cpp
    ScriptingRuntime.cpp
    youtube_bundle.c
)
# gersemi: on
