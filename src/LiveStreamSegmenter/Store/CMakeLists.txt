add_library(${CMAKE_PROJECT_NAME}_Store OBJECT)
target_include_directories(${CMAKE_PROJECT_NAME}_Store PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Store
  PUBLIC OBS::libobs OBS::obs-frontend-api
  Logger GoogleAuth ObsBridgeUtils YouTubeApi
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Store
  PRIVATE AuthStore.cpp AuthStore.hpp EventHandlerStore.cpp EventHandlerStore.hpp YouTubeStore.cpp YouTubeStore.hpp
)
