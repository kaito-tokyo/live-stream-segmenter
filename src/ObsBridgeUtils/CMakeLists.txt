add_library(ObsBridgeUtils INTERFACE)
target_include_directories(ObsBridgeUtils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ObsBridgeUtils INTERFACE OBS::libobs Logger)
target_sources(ObsBridgeUtils PRIVATE AsyncTextureReader.hpp GsUnique.hpp ObsLogger.hpp ObsUnique.hpp)
