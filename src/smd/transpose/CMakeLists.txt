include(GNUInstallDirs)

target_sources(
    transpose.transpose
    PUBLIC FILE_SET transpose_transpose_headers TYPE HEADERS FILES transpose.hpp
    PRIVATE transpose.cpp
)

## Tests
if(TRANSPOSE_ENABLE_TESTING AND NOT GTest_FOUND)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        include(GoogleTest)
    else()
        message(
            WARNING
            "
No provider for GTest. Unable to build tests.
Consider using CMAKE_PROJECT_TOP_LEVEL_INCLUDES=./cmake/use-fetch-content.cmake as documented in the README.md
"
        )
    endif()
endif()

if(TRANSPOSE_ENABLE_TESTING)
    add_executable(transpose_test)

    target_sources(transpose_test PRIVATE transpose.test.cpp)

    target_link_libraries(transpose_test transpose.transpose)
    target_link_libraries(transpose_test GTest::gtest)
    target_link_libraries(transpose_test GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(transpose_test)
endif()
