add_library(Logger INTERFACE)
target_include_directories(Logger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(Logger PRIVATE ILogger.hpp NullLogger.hpp)
