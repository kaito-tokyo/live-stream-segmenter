include(GoogleTest)

set(TEST_LIST "")

add_executable(EventScriptingContext_test Scripting/EventScriptingContext_test.cpp)
target_link_libraries(EventScriptingContext_test PRIVATE GTest::gtest_main ${CMAKE_PROJECT_NAME}_Scripting)
list(APPEND TEST_LIST EventScriptingContext_test)

foreach(TEST_NAME IN LISTS TEST_LIST)
  set_target_properties(
    ${TEST_NAME}
    PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist"
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
      XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "${DERIVED_DATA_PATH}"
  )
  gtest_discover_tests(${TEST_NAME} DISCOVERY_MODE PRE_TEST)
endforeach()
