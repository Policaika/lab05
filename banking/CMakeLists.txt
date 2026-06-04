add_library(banking STATIC Account.cpp Transaction.cpp)
target_include_directories(banking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CODE_COVERAGE)
    target_compile_options(banking PRIVATE -O0 -g --coverage)
    target_link_options(banking PRIVATE --coverage)
endif()
