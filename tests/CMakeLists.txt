include(GoogleTest)

set(TEST_LIST "")

# UpdateChecker_test
add_executable(UpdateChecker_test UpdateChecker/UpdateChecker_test.cpp)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main UpdateChecker)
list(APPEND TEST_LIST UpdateChecker_test)

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
