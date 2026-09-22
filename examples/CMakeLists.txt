# examples/CMakeLists.txt                                           -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES
    transpose_example
    traverse_example
    apply_example
    fold_example
    soa_aos_example
    binary_tree_example
    algorithm_object_example
    lookup_modes_example
    graded_gadgets_example
    transpose_post_example
    traverse_post_example
)

include(CheckIncludeFileCXX)
check_include_file_cxx(simd BEMAN_TRANSPOSE_HAS_SIMD)
if(BEMAN_TRANSPOSE_HAS_SIMD)
    list(APPEND ALL_EXAMPLES simd_example)
endif()

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

# The receipts for the deferred domain: the same front door over genuine
# std::execution senders, with concurrency actually happening. Opt-in for the
# reason the adapter it uses is. See
# docs/transpose-execution-plan.md#transpose-receipts.
if(BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE)
    add_executable(beman.transpose.examples.transpose_execution_example)
    target_sources(
        beman.transpose.examples.transpose_execution_example
        PRIVATE transpose_execution_example.cpp
    )
    target_include_directories(
        beman.transpose.examples.transpose_execution_example
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(
        beman.transpose.examples.transpose_execution_example
        PRIVATE beman::transpose beman::execution
    )
    add_test(
        NAME beman.transpose.examples.transpose_execution_example
        COMMAND beman.transpose.examples.transpose_execution_example
    )
endif()
