// tests/beman/transpose/simd_lanes.test.cpp                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/simd_lanes.hpp>
#include <beman/transpose/transpose.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <vector>

namespace bt = beman::transpose;

using lanes4 = bt::simd_lanes<int, 4>;

TEST_CASE("simd_lanes: pure broadcasts to every lane") {
    auto r = bt::applicative_typeclass<lanes4>.pure(7);
    REQUIRE(r == lanes4::repeat(7));
}

TEST_CASE("simd_lanes: invoke applies lane by lane") {
    const auto &app = bt::applicative_typeclass<lanes4>;
    lanes4 a{{1, 2, 3, 4}};
    lanes4 b{{10, 20, 30, 40}};
    auto sum = app.invoke([](int x, int y) { return x + y; }, a, b);
    REQUIRE(sum == lanes4{{11, 22, 33, 44}});
}

TEST_CASE("simd_lanes: applicative laws hold") {
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    using bt::test::check_applicative_interchange_law;
    using bt::test::check_functor_composition_law;

    REQUIRE(check_applicative_identity_law(lanes4{{5, 6, 7, 8}}));
    REQUIRE(check_applicative_homomorphism_law<lanes4>(
        [](int a, int b) { return a * b; }, 6, 7));

    auto add_one = [](int x) { return x + 1; };
    bt::simd_lanes<decltype(add_one), 4> functions;
    functions.data.fill(add_one);
    REQUIRE(check_applicative_interchange_law(functions, 41));
    REQUIRE(check_functor_composition_law([](int x) { return x * 3; },
                                          [](int x) { return x - 1; },
                                          lanes4{{2, 4, 6, 8}}));
}

TEST_CASE("simd_lanes: dual cores cohere") {
    using bt::test::check_apply_invoke_coherence;
    using bt::test::check_invoke_ap_chain_coherence;

    auto square = [](int x) { return x * x; };
    bt::simd_lanes<decltype(square), 4> functions;
    functions.data.fill(square);
    REQUIRE(check_apply_invoke_coherence(functions, lanes4{{1, 2, 3, 4}}));
    REQUIRE(check_invoke_ap_chain_coherence([](int a, int b) { return a - b; },
                                            lanes4{{9, 8, 7, 6}},
                                            lanes4{{1, 2, 3, 4}}));
}

TEST_CASE("simd_lanes: transpose round-trips structure and lanes") {
    std::vector<lanes4> structure{lanes4{{1, 2, 3, 4}}, lanes4{{10, 20, 30, 40}},
                                  lanes4{{100, 200, 300, 400}}};
    auto transposed = bt::transpose(structure);

    REQUIRE(transposed.width == 4);
    REQUIRE(transposed.data[0] == std::vector<int>{1, 10, 100});
    REQUIRE(transposed.data[3] == std::vector<int>{4, 40, 400});
}
