add_library(CurlHelper INTERFACE)
# gersemi: off
target_link_libraries(
  CurlHelper
  INTERFACE
    CURL::libcurl
)
target_sources(
  CurlHelper
  INTERFACE
  FILE_SET HEADERS
  FILES
    KaitoTokyo/CurlHelper/CurlHandle.hpp
    KaitoTokyo/CurlHelper/CurlReadCallback.hpp
    KaitoTokyo/CurlHelper/CurlSlistHandle.hpp
    KaitoTokyo/CurlHelper/CurlUrlHandle.hpp
    KaitoTokyo/CurlHelper/CurlUrlSearchParams.hpp
    KaitoTokyo/CurlHelper/CurlWriteCallback.hpp
)
# gersemi: on
