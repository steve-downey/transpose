# CMakeLists.txt                                                     -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

project(
    beman.transpose
    DESCRIPTION
        "Shape-preserving traversal and transposition for contextual computations (Paper A / P3200)"
    LANGUAGES CXX
    VERSION 0.0.0
)

include(CTest)

# Detect a C++26 standard library that ships <simd> (e.g. GCC 16). The std::simd
# example and test are non-normative demonstrations of the lanewise applicative
# on real hardware; they are gated out on toolchains without <simd>.
include(CheckCXXSourceCompiles)
set(_beman_transpose_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "-std=c++26")
check_cxx_source_compiles(
    "#include <simd>
     int main() { return static_cast<int>(std::simd::vec<float>::size()); }"
    BEMAN_TRANSPOSE_HAS_STD_SIMD
)
set(CMAKE_REQUIRED_FLAGS "${_beman_transpose_saved_required_flags}")

option(
    BEMAN_TRANSPOSE_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_TRANSPOSE_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

configure_file(
    "${PROJECT_SOURCE_DIR}/include/beman/transpose/config_generated.hpp.in"
    "${PROJECT_BINARY_DIR}/include/beman/transpose/config_generated.hpp"
    @ONLY
)

# Real-async demonstration via vendored beman.execution (a std::execution /
# P2300 implementation). Built only when the vendored subtree is present;
# consumed headers-only (modules off, its own tests/examples off), the same way
# callcc vendors it. The exec example/test link beman::execution_headers.
set(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION OFF)
if(EXISTS "${PROJECT_SOURCE_DIR}/vendored/beman.execution/CMakeLists.txt")
    set(BEMAN_USE_MODULES OFF CACHE BOOL "consume beman.execution as headers" FORCE)
    set(BEMAN_EXECUTION_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BEMAN_EXECUTION_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendored/beman.execution)
    set(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION ON)
endif()

# beman.transpose is a header-only library.
add_library(beman.transpose INTERFACE)
add_library(beman::transpose ALIAS beman.transpose)

target_compile_features(beman.transpose INTERFACE cxx_std_23)

target_sources(
    beman.transpose
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS
                "${PROJECT_SOURCE_DIR}/include"
                "${PROJECT_BINARY_DIR}/include"
)

set_target_properties(
    beman.transpose
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
)

add_subdirectory(include/beman/transpose)

include(infra/cmake/beman-install-library.cmake)
beman_install_library(beman.transpose TARGETS beman.transpose)

if(BEMAN_TRANSPOSE_BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)
    include(Catch)
    enable_testing()
    add_subdirectory(tests/beman/transpose)
endif()

if(BEMAN_TRANSPOSE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Coverage helper retained for the Makefile Gcov configuration.
configure_file("cmake/gcovr.cfg.in" gcovr.cfg @ONLY)

add_custom_target(
    process_coverage
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running gcovr to process coverage results"
    COMMAND mkdir -p coverage
    COMMAND gcovr --config gcovr.cfg .
)
