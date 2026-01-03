# gersemi: off
add_library(Async INTERFACE)
target_sources(
  Async
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/Async/Channel.hpp
    KaitoTokyo/Async/Join.hpp
    KaitoTokyo/Async/Task.hpp
)
# gersemi: on
