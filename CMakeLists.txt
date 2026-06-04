cmake_minimum_required(VERSION 3.10)

project(lab05)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(banking)

option(BUILD_TESTS "Build unit tests" ON)
option(CODE_COVERAGE "Coverage report" ON)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(third-party/gtest)
  file(GLOB ${PROJECT_NAME}_TEST tests/*.cpp)
  add_executable(banking_test ${${PROJECT_NAME}_TEST})
  target_include_directories(banking_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/banking ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(banking_test ${PROJECT} banking gtest gtest_main gmock)
    if(CODE_COVERAGE)
      target_compile_options(banking_test PRIVATE --coverage -O0 -g)
      target_link_options(banking_test PRIVATE --coverage)
    endif()
  add_test(NAME banking_test COMMAND banking_test)
endif()
