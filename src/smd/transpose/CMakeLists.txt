include(GNUInstallDirs)

target_sources(
    transpose.transpose
    PUBLIC FILE_SET transpose_transpose_headers TYPE HEADERS FILES transpose.hpp
    PRIVATE transpose.cpp
)

## Tests
if(TRANSPOSE_ENABLE_TESTING AND NOT Catch2_FOUND)
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        include(Catch)
    else()
        message(
            WARNING
            "
No provider for Catch2. Unable to build tests.
Consider using CMAKE_PROJECT_TOP_LEVEL_INCLUDES=./cmake/use-fetch-content.cmake as documented in the README.md
"
        )
    endif()
endif()

if(TRANSPOSE_ENABLE_TESTING)
    add_executable(transpose_test)

    target_sources(transpose_test PRIVATE transpose.test.cpp)

    target_link_libraries(transpose_test transpose.transpose)
    target_link_libraries(transpose_test Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(transpose_test)
endif()
