# examples/CMakeLists.txt                                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES transpose_example traverse_example apply_example fold_example)

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.transpose.examples.${example})
    target_sources(beman.transpose.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(
        beman.transpose.examples.${example}
        PRIVATE beman::transpose
    )
    add_test(
        NAME beman.transpose.examples.${example}
        COMMAND beman.transpose.examples.${example}
    )
endforeach()

# Real-async example over beman.execution. Built only when the vendored subtree
# is present (set in the top-level CMakeLists); links beman::execution_headers.
if(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION)
    add_executable(beman.transpose.examples.exec_example)
    target_sources(
        beman.transpose.examples.exec_example
        PRIVATE exec_example.cpp
    )
    target_link_libraries(
        beman.transpose.examples.exec_example
        PRIVATE beman::transpose beman::execution_headers
    )
    add_test(
        NAME beman.transpose.examples.exec_example
        COMMAND beman.transpose.examples.exec_example
    )
endif()

# Real std::simd lanewise example. Requires C++26 + <simd>; built only where the
# feature check in the top-level CMakeLists succeeded, and at C++26.
if(BEMAN_TRANSPOSE_HAS_STD_SIMD)
    add_executable(beman.transpose.examples.simd_example)
    target_sources(
        beman.transpose.examples.simd_example
        PRIVATE simd_example.cpp
    )
    target_link_libraries(
        beman.transpose.examples.simd_example
        PRIVATE beman::transpose
    )
    set_target_properties(
        beman.transpose.examples.simd_example
        PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON
    )
    add_test(
        NAME beman.transpose.examples.simd_example
        COMMAND beman.transpose.examples.simd_example
    )
endif()
