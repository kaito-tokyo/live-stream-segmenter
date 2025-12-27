add_library(AsyncQt INTERFACE)
target_include_directories(AsyncQt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AsyncQt INTERFACE Qt6::Core Async)
target_sources(AsyncQt PRIVATE ResumeOnQObject.hpp ResumeOnQThreadPool.hpp)
