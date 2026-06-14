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
