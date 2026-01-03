add_library(AsyncQt INTERFACE)
# gersemi: off
target_link_libraries(
  AsyncQt
  INTERFACE
    Qt6::Core
    Async
)
target_sources(
  AsyncQt
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/AsyncQt/ResumeOnQObject.hpp
    KaitoTokyo/AsyncQt/ResumeOnQThreadPool.hpp
    KaitoTokyo/AsyncQt/ResumeOnQTimerSingleShot.hpp
)
# gersemi: on
