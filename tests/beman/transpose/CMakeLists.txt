# tests/beman/transpose/CMakeLists.txt                              -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(transpose_tests
    baseline_deduction
    demo_sender_golden
    monoid
    dual_monoid
    error_set
    crtp_absorption
    accumulating_object
    collect_hook
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
    # beman.execution's inline default parallel backend uses std::thread but
    # does not yet propagate the platform threads library from its target.
    find_package(Threads REQUIRED)
    # The dependency compile probe: beman.execution and nothing of ours, so a
    # broken dependency fails here rather than inside the adapter's
    # diagnostics. See docs/transpose-execution-plan.md#execution-baseline.
    add_executable(beman.transpose.tests.execution_probe)
    target_sources(
        beman.transpose.tests.execution_probe
        PRIVATE execution_probe.test.cpp
    )
    target_link_libraries(
        beman.transpose.tests.execution_probe
        PRIVATE beman::execution Catch2::Catch2WithMain Threads::Threads
    )
    catch_discover_tests(beman.transpose.tests.execution_probe)

    # all_of: when_all at runtime arity, plus the allocation measurement that
    # docs/decisions.md#erasure-boundary names as its sentinel. Its own
    # executable because it replaces global operator new.
    add_executable(beman.transpose.tests.all_of)
    target_sources(beman.transpose.tests.all_of PRIVATE all_of.test.cpp)
    target_include_directories(
        beman.transpose.tests.all_of
        PRIVATE ${PROJECT_SOURCE_DIR}/examples
    )
    target_link_libraries(
        beman.transpose.tests.all_of
        PRIVATE
            beman::transpose
            beman::execution
            Catch2::Catch2WithMain
            Threads::Threads
    )
    catch_discover_tests(beman.transpose.tests.all_of)

    # The allocation measurement is a separate executable because it replaces
    # global operator new, which does not link against the ThreadSanitizer
    # runtime. Keeping it apart is what lets all_of.test.cpp's concurrency
    # case be checked under TSan.
    add_executable(beman.transpose.tests.all_of_allocation)
    target_sources(
        beman.transpose.tests.all_of_allocation
        PRIVATE all_of_allocation.test.cpp
    )
    target_include_directories(
        beman.transpose.tests.all_of_allocation
        PRIVATE ${PROJECT_SOURCE_DIR}/examples
    )
    target_link_libraries(
        beman.transpose.tests.all_of_allocation
        PRIVATE
            beman::transpose
            beman::execution
            Catch2::Catch2WithMain
            Threads::Threads
    )
    catch_discover_tests(beman.transpose.tests.all_of_allocation)

    add_executable(beman.transpose.tests.p2300)
    target_sources(beman.transpose.tests.p2300 PRIVATE p2300.test.cpp)
    target_include_directories(
        beman.transpose.tests.p2300
        PRIVATE ${PROJECT_SOURCE_DIR}/examples
    )
    target_link_libraries(
        beman.transpose.tests.p2300
        PRIVATE
            beman::transpose
            beman::execution
            Catch2::Catch2WithMain
            Threads::Threads
    )
    catch_discover_tests(beman.transpose.tests.p2300)

    # The front door over real senders: the `collect` hook's sender half.
    # Its generic half is collect_hook.test.cpp, which needs no dependency
    # and is built unconditionally. See
    # docs/transpose-execution-plan.md#collect-hook.
    add_executable(beman.transpose.tests.transpose_senders)
    target_sources(
        beman.transpose.tests.transpose_senders
        PRIVATE transpose_senders.test.cpp
    )
    target_include_directories(
        beman.transpose.tests.transpose_senders
        PRIVATE ${PROJECT_SOURCE_DIR}/examples
    )
    target_link_libraries(
        beman.transpose.tests.transpose_senders
        PRIVATE
            beman::transpose
            beman::execution
            Catch2::Catch2WithMain
            Threads::Threads
    )
    catch_discover_tests(beman.transpose.tests.transpose_senders)
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
