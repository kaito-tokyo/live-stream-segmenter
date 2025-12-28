add_library(${CMAKE_PROJECT_NAME}_Store INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME}_Store INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME}_Store INTERFACE OBS::libobs OBS::obs-frontend-api Logger GoogleAuth)
target_sources(${CMAKE_PROJECT_NAME}_Store PRIVATE AuthStore.hpp EventHandlerStore.hpp YouTubeStore.hpp)
