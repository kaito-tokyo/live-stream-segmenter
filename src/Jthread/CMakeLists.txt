# gersemi: off
add_library(Jthread INTERFACE)
target_sources(
  Jthread
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/Jthread/Jthread.hpp
)
target_link_libraries(Jthread INTERFACE jthread::jthread)
# gersemi: on
