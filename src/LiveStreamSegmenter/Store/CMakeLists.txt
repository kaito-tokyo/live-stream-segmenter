add_library(${CMAKE_PROJECT_NAME}_Store STATIC)
set_target_properties(${CMAKE_PROJECT_NAME}_Store PROPERTIES AUTOMOC ON)
# gersemi: off
target_link_libraries(
  ${CMAKE_PROJECT_NAME}_Store
  PUBLIC
    Qt6::Core
    OBS::libobs
    OBS::obs-frontend-api
    Qt6Keychain::Qt6Keychain
    Logger
    GoogleAuth
    ObsBridgeUtils
    YouTubeApi
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Store
  PUBLIC
  FILE_SET HEADERS
  FILES
    AuthStore.hpp
    EventHandlerStore.hpp
    YouTubeStore.hpp
)
target_sources(
  ${CMAKE_PROJECT_NAME}_Store
  PRIVATE
    AuthStore.cpp
    EventHandlerStore.cpp
    YouTubeStore.cpp
)
# gersemi: on
