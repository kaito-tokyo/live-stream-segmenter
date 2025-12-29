add_library(${CMAKE_PROJECT_NAME}_Scripting STATIC)
target_include_directories(${CMAKE_PROJECT_NAME}_Scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_Scripting PUBLIC qjs::qjs unofficial::sqlite3::sqlite3 Logger)
target_sources(
  ${CMAKE_PROJECT_NAME}_Scripting
  PRIVATE
    dayjs_bundle.c
    EventScriptingContext.cpp
    EventScriptingContext.hpp
    ini_bundle.c
    localstorage_bundle.c
    ScriptingDatabase.cpp
    ScriptingDatabase.hpp
    ScriptingRuntime.cpp
    ScriptingRuntime.hpp
    youtube_bundle.c
)
