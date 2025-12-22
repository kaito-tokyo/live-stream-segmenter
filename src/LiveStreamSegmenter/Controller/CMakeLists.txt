add_library(${CMAKE_PROJECT_NAME}_Controller INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_Controller INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Controller
  INTERFACE ${CMAKE_PROJECT_NAME}_Store ${CMAKE_PROJECT_NAME}_UI GoogleAuth Logger
)
target_sources(${CMAKE_PROJECT_NAME}_Controller PRIVATE MainPluginContext.hpp ProfileContext.hpp)
