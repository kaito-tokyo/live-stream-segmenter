add_library(CurlHelper INTERFACE)
target_include_directories(CurlHelper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CurlHelper INTERFACE CURL::libcurl)
target_sources(
  CurlHelper
  PRIVATE
    CurlHandle.hpp
    CurlReadCallback.hpp
    CurlSlistHandle.hpp
    CurlUrlHandle.hpp
    CurlUrlSearchParams.hpp
    CurlWriteCallback.hpp
)
