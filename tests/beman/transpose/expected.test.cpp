// tests/beman/transpose/expected.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/expected.hpp>

#include <beman/transpose/sequence.hpp>
#include <beman/transpose/transpose.hpp>
#include <beman/transpose/traverse.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <string>
#include <system_error>
#include <vector>

namespace bt = beman::transpose;

namespace {

using errc = std::errc;
using exp_int = std::expected<int, errc>;

auto fail(errc code) -> exp_int { return exp_int{std::unexpect, code}; }

/** Only positive inputs succeed; the rest carry a distinguishable error. */
auto positive(int x) -> exp_int {
    if (x > 0) {
        return exp_int{x * 2};
    }
    return fail(errc::invalid_argument);
}

} // namespace

TEST_CASE("expected: applicative pure and invoke") {
    const auto &app = bt::applicative_typeclass<exp_int>;

    REQUIRE(app.pure(7) == std::expected<int, errc>{7});

    auto sum =
        app.invoke([](int a, int b) { return a + b; }, exp_int{2}, exp_int{3});
    REQUIRE(sum == exp_int{5});
}

TEST_CASE("expected: invoke short-circuits and the leftmost error wins") {
    const auto &app = bt::applicative_typeclass<exp_int>;
    auto add = [](int a, int b, int c) { return a + b + c; };

    // Effect order is observable and part of the contract: with two failing
    // operands, the LEFTMOST error is the one propagated.
    auto both_fail = app.invoke(add, exp_int{1}, fail(errc::invalid_argument),
                                fail(errc::result_out_of_range));
    REQUIRE_FALSE(both_fail.has_value());
    REQUIRE(both_fail.error() == errc::invalid_argument);

    auto later_fails = app.invoke(add, exp_int{1}, exp_int{2},
                                  fail(errc::result_out_of_range));
    REQUIRE_FALSE(later_fails.has_value());
    REQUIRE(later_fails.error() == errc::result_out_of_range);
}

TEST_CASE("expected: applicative map and the derived operations") {
    const auto &app = bt::applicative_typeclass<exp_int>;

    auto mapped = app.map([](int x) { return x * 1.5; }, exp_int{4});
    REQUIRE(mapped == std::expected<double, errc>{6.0});

    auto mapped_error =
        app.map([](int x) { return x * 1.5; }, fail(errc::invalid_argument));
    REQUIRE_FALSE(mapped_error.has_value());
    REQUIRE(mapped_error.error() == errc::invalid_argument);

    REQUIRE(app.discard_first(exp_int{1}, exp_int{2}) == exp_int{2});
    REQUIRE(app.discard_second(exp_int{1}, exp_int{2}) == exp_int{1});
}

TEST_CASE("expected: monad bind short-circuits") {
    const auto &monad = bt::monad_typeclass<exp_int>;

    auto to_text = [](int x) {
        return std::expected<std::string, errc>{std::to_string(x)};
    };

    REQUIRE(monad.bind(exp_int{12}, to_text) ==
            std::expected<std::string, errc>{"12"});

    auto short_circuited = monad.bind(fail(errc::invalid_argument), to_text);
    REQUIRE_FALSE(short_circuited.has_value());
    REQUIRE(short_circuited.error() == errc::invalid_argument);
}

TEST_CASE("expected: monad join flattens") {
    auto nested = std::expected<exp_int, errc>{exp_int{9}};
    REQUIRE(bt::join(nested) == exp_int{9});

    auto outer_failed =
        std::expected<exp_int, errc>{std::unexpect, errc::invalid_argument};
    REQUIRE(bt::join(outer_failed).error() == errc::invalid_argument);
}

TEST_CASE("expected: traverse over a vector transposes into one expected") {
    auto ok = bt::traverse(positive, std::vector<int>{1, 2, 3});
    REQUIRE(ok == std::expected<std::vector<int>, errc>{{2, 4, 6}});

    auto bad = bt::traverse(positive, std::vector<int>{1, -1, 3});
    REQUIRE_FALSE(bad.has_value());
    REQUIRE(bad.error() == errc::invalid_argument);
}

TEST_CASE("expected: traverse preserves shape and empty is pure") {
    auto empty = bt::traverse(positive, std::vector<int>{});
    REQUIRE(empty == std::expected<std::vector<int>, errc>{std::vector<int>{}});

    auto result = bt::traverse(positive, std::vector<int>{5, 4, 3, 2, 1});
    REQUIRE(result.has_value());
    REQUIRE(*result == std::vector<int>{10, 8, 6, 4, 2});
}

TEST_CASE("expected: transpose a vector of expected") {
    auto all_ok =
        bt::transpose(std::vector<exp_int>{exp_int{1}, exp_int{2}, exp_int{3}});
    REQUIRE(all_ok == std::expected<std::vector<int>, errc>{{1, 2, 3}});

    auto one_bad = bt::transpose(std::vector<exp_int>{
        exp_int{1}, fail(errc::result_out_of_range), exp_int{3}});
    REQUIRE_FALSE(one_bad.has_value());
    REQUIRE(one_bad.error() == errc::result_out_of_range);
}

TEST_CASE("expected: for_each over the Identity traversable") {
    const auto &t = bt::traversable_typeclass<bt::test::Identity<int>>;
    auto result = t.for_each(bt::test::Identity<int>{20}, positive);
    REQUIRE(result == std::expected<bt::test::Identity<int>, errc>{
                          bt::test::Identity<int>{40}});
}
