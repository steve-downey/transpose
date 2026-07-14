// tests/beman/transpose/monad.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/monad.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>

namespace bt = beman::transpose;

TEST_CASE("monad: optional bind short-circuits on empty") {
    auto half_if_even = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    REQUIRE(bt::mbind(std::optional<int>{8}, half_if_even) ==
            std::optional<int>{4});
    REQUIRE(bt::mbind(std::optional<int>{7}, half_if_even) ==
            std::optional<int>{});
    REQUIRE(bt::mbind(std::optional<int>{}, half_if_even) ==
            std::optional<int>{});
}

TEST_CASE("monad: join flattens nested optional") {
    std::optional<std::optional<int>> nested{std::optional<int>{5}};
    REQUIRE(bt::join(nested) == std::optional<int>{5});
}

TEST_CASE("monad: invoke synthesized from bind and pure") {
    const auto &m = bt::monad_typeclass<std::optional<int>>;
    auto add3 = [](int a, int b, int c) { return a + b + c; };

    REQUIRE(m.invoke(add3, std::optional<int>{1}, std::optional<int>{2},
                     std::optional<int>{3}) == std::optional<int>{6});
    REQUIRE(m.invoke(add3, std::optional<int>{1}, std::optional<int>{},
                     std::optional<int>{3}) == std::optional<int>{});
}

TEST_CASE("monad: invoke coheres with the applicative invoke") {
    const auto &monad = bt::monad_typeclass<std::optional<int>>;
    const auto &applicative = bt::applicative_typeclass<std::optional<int>>;
    auto add = [](int a, int b) { return a + b; };

    REQUIRE(
        monad.invoke(add, std::optional<int>{4}, std::optional<int>{5}) ==
        applicative.invoke(add, std::optional<int>{4}, std::optional<int>{5}));
    REQUIRE(
        monad.invoke(add, std::optional<int>{}, std::optional<int>{5}) ==
        applicative.invoke(add, std::optional<int>{}, std::optional<int>{5}));
}
