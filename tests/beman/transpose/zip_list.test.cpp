// tests/beman/transpose/zip_list.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/zip_list.hpp>

#include <catch2/catch_test_macros.hpp>

#include <vector>

namespace bt = beman::transpose;

TEST_CASE("zip_list: pure repeats infinitely") {
    auto r = bt::applicative_typeclass<bt::zip_list<int>>.pure(7);
    REQUIRE(r.is_repeating());
    REQUIRE(r.repeated == 7);
}

TEST_CASE("zip_list: apply zips positionally and truncates to shortest") {
    const auto &app = bt::applicative_typeclass<bt::zip_list<int>>;
    auto result =
        app.invoke([](int a, int b) { return a + b; },
                   bt::zip_list<int>{{1, 2, 3}}, bt::zip_list<int>{{10, 20}});
    REQUIRE(result.data == std::vector<int>{11, 22});
}

TEST_CASE("zip_list: pure operand acts as identity for truncation") {
    const auto &app = bt::applicative_typeclass<bt::zip_list<int>>;
    auto result =
        app.invoke([](int a, int b) { return a * b; },
                   bt::zip_list<int>{{1, 2, 3}}, bt::zip_list<int>::repeat(10));
    REQUIRE(result.data == std::vector<int>{10, 20, 30});
}
