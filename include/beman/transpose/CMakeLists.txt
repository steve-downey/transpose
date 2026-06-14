# include/beman/transpose/CMakeLists.txt                            -*-cmake-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

target_sources(
    beman.transpose
    INTERFACE
        FILE_SET HEADERS
            FILES
                config.hpp
                apply.hpp
                dual_monoid.hpp
                fold.hpp
                functor.hpp
                monad.hpp
                monoid.hpp
                sender.hpp
                sequence.hpp
                transpose.hpp
                traverse.hpp
                zip_list.hpp
                detail/typeclass_base.hpp
                "${PROJECT_BINARY_DIR}/include/beman/transpose/config_generated.hpp"
)
