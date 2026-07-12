// tests/beman/transpose/simd.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/simd.hpp>

#include <catch2/catch_test_macros.hpp>

#if !defined(BEMAN_TRANSPOSE_HAS_STD_SIMD)

TEST_CASE("simd: <simd> unavailable in this toolchain") {
    SUCCEED("std::simd not available; simd applicative test skipped");
}

#else

#include <beman/transpose/zip_list.hpp>

#include <cstddef>
#include <simd>
#include <vector>

namespace bt = beman::transpose;
namespace dp = std::simd;

TEST_CASE("simd: pure broadcasts a scalar to every lane") {
    const auto &app = bt::applicative_typeclass<dp::vec<int>>;
    auto r = app.pure(7);
    for (std::size_t lane = 0; lane < r.size(); ++lane) {
        REQUIRE(r[lane] == 7);
    }
}

TEST_CASE("simd: invoke applies a scalar function lanewise") {
    const auto &app = bt::applicative_typeclass<dp::vec<int>>;
    dp::vec<int> xs([](auto i) { return int(i) + 1; }); // 1 2 3 ...
    dp::vec<int> ys = 10;                                // broadcast
    auto sum = app.invoke([](int a, int b) { return a + b; }, xs, ys);
    for (std::size_t lane = 0; lane < sum.size(); ++lane) {
        REQUIRE(sum[lane] == int(lane) + 1 + 10);
    }
}

TEST_CASE("simd: zip_with matches the zip_list model on the same lanes") {
    const std::size_t width = dp::vec<int>::size();

    // Real hardware lanes.
    const auto &simd_app = bt::applicative_typeclass<dp::vec<int>>;
    dp::vec<int> xs([](auto i) { return int(i) + 1; });
    dp::vec<int> ys = 10;
    auto simd_result =
        simd_app.zip_with([](int a, int b) { return a * b; }, xs, ys);

    // The zip_list model over identical inputs.
    std::vector<int> xs_lanes, ys_lanes;
    for (std::size_t lane = 0; lane < width; ++lane) {
        xs_lanes.push_back(int(lane) + 1);
        ys_lanes.push_back(10);
    }
    const auto &zl_app = bt::applicative_typeclass<bt::zip_list<int>>;
    auto zl_result = zl_app.invoke([](int a, int b) { return a * b; },
                                   bt::zip_list<int>{xs_lanes},
                                   bt::zip_list<int>{ys_lanes});

    REQUIRE(zl_result.data.size() == width);
    for (std::size_t lane = 0; lane < width; ++lane) {
        REQUIRE(simd_result[lane] == zl_result.data[lane]);
    }
}

#endif
