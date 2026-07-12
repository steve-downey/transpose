# tests/beman/transpose/CMakeLists.txt                              -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(transpose_tests
    monoid
    dual_monoid
    functor
    apply
    fold
    monad
    traverse
    transpose
    sequence
    sender
    zip_list
)

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

# Real-async test over beman.execution. Built only when the vendored subtree is
# present (set in the top-level CMakeLists); links beman::execution_headers.
if(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION)
    add_executable(beman.transpose.tests.exec)
    target_sources(beman.transpose.tests.exec PRIVATE exec.test.cpp)
    target_include_directories(
        beman.transpose.tests.exec
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(
        beman.transpose.tests.exec
        PRIVATE beman::transpose beman::execution_headers Catch2::Catch2WithMain
    )
    catch_discover_tests(beman.transpose.tests.exec)
endif()

# Real std::simd lanewise applicative test. Requires C++26 + <simd>; built only
# where the feature check in the top-level CMakeLists succeeded, and at C++26.
if(BEMAN_TRANSPOSE_HAS_STD_SIMD)
    add_executable(beman.transpose.tests.simd)
    target_sources(beman.transpose.tests.simd PRIVATE simd.test.cpp)
    target_include_directories(
        beman.transpose.tests.simd
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(
        beman.transpose.tests.simd
        PRIVATE beman::transpose Catch2::Catch2WithMain
    )
    set_target_properties(
        beman.transpose.tests.simd
        PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON
    )
    catch_discover_tests(beman.transpose.tests.simd)
endif()

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
