add_library(ObsBridgeUtils INTERFACE)
# gersemi: off
target_link_libraries(
  ObsBridgeUtils
  INTERFACE
    fmt::fmt
    OBS::libobs
    Logger
)
target_sources(
  ObsBridgeUtils
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/ObsBridgeUtils/AsyncTextureReader.hpp
    KaitoTokyo/ObsBridgeUtils/GsUnique.hpp
    KaitoTokyo/ObsBridgeUtils/ObsLogger.hpp
    KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp
)
# gersemi: on
