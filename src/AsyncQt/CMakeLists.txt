add_library(AsyncQt STATIC)
target_include_directories(AsyncQt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AsyncQt PUBLIC Qt6::Core Logger)
target_sources(AsyncQt PRIVATE TaskLauncher.hpp TaskLauncher.cpp)
