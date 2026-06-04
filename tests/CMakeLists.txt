add_executable(banking_tests
    test_account.cpp
    test_transaction.cpp
)

target_include_directories(banking_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(banking_tests PRIVATE
    banking
    gtest_main
    gmock
)

include(GoogleTest)
gtest_discover_tests(banking_tests)
