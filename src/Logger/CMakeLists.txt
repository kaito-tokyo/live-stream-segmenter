# gersemi: off
add_library(Logger INTERFACE)
target_sources(
  Logger
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/Logger/ILogger.hpp
    KaitoTokyo/Logger/NullLogger.hpp
    KaitoTokyo/Logger/MultiLogger.hpp
    KaitoTokyo/Logger/PrintLogger.hpp
)
# gersemi: on
