# tests/beman/transpose/CMakeLists.txt                              -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(transpose_tests
    baseline_deduction
    monoid
    dual_monoid
    error_set
    crtp_absorption
    accumulating_object
    functor
    apply
    expected
    fold
    grade
    laws
    monad
    induced_monoid
    objects
    traverse
    transpose
    type_requirements
    sequence
    sender
    simd_lanes
    zip_list
    probe_harness
)

# The simd test self-gates on C++26 + <simd>; only build it when the header
# exists at all (mirrors examples/CMakeLists.txt).
include(CheckIncludeFileCXX)
check_include_file_cxx(simd BEMAN_TRANSPOSE_TESTS_HAS_SIMD)
if(BEMAN_TRANSPOSE_TESTS_HAS_SIMD)
    list(APPEND transpose_tests simd)
endif()

foreach(test ${transpose_tests})
    add_executable(beman.transpose.tests.${test})
    target_sources(beman.transpose.tests.${test} PRIVATE ${test}.test.cpp)
    target_include_directories(
        beman.transpose.tests.${test}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(
        beman.transpose.tests.${test}
        PRIVATE beman::transpose Catch2::Catch2WithMain
    )
    catch_discover_tests(beman.transpose.tests.${test})
endforeach()

# The P2300 evidence is opt-in: it is the only test here with an external
# dependency. Its adapter lives under examples/ rather than include/, so that
# nothing an installation of this library pulls in depends on beman.execution.
if(BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE)
    add_executable(beman.transpose.tests.p2300)
    target_sources(beman.transpose.tests.p2300 PRIVATE p2300.test.cpp)
    target_include_directories(
        beman.transpose.tests.p2300
        PRIVATE ${PROJECT_SOURCE_DIR}/examples
    )
    target_link_libraries(
        beman.transpose.tests.p2300
        PRIVATE beman::transpose beman::execution Catch2::Catch2WithMain
    )
    catch_discover_tests(beman.transpose.tests.p2300)
endif()

# The probe harness carries a second translation unit: whether two TUs that
# spell an error_set's pack differently agree on the type is a link-time
# fact, and this is the only test that needs one.
target_sources(
    beman.transpose.tests.probe_harness
    PRIVATE probe_harness_cross_tu.cpp
)

# detail/ support headers carry their own tests.
add_executable(beman.transpose.tests.detail.typeclass_base)
target_sources(
    beman.transpose.tests.detail.typeclass_base
    PRIVATE detail/typeclass_base.test.cpp
)
target_link_libraries(
    beman.transpose.tests.detail.typeclass_base
    PRIVATE beman::transpose Catch2::Catch2WithMain
)
catch_discover_tests(beman.transpose.tests.detail.typeclass_base)
