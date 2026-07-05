// tests/beman/transpose/apply.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/apply.hpp>
#include <beman/transpose/zip_list.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

namespace bt = beman::transpose;

TEST_CASE("apply: optional invoke combines effectful arguments") {
    const auto &app = bt::applicative_typeclass<std::optional<int>>;
    auto add = [](int a, int b, int c) { return a + b + c; };
    REQUIRE(app.invoke(add, std::optional<int>{1}, std::optional<int>{2},
                       std::optional<int>{3}) == std::optional<int>{6});
    REQUIRE(app.invoke(add, std::optional<int>{1}, std::optional<int>{},
                       std::optional<int>{3}) == std::optional<int>{});
}

TEST_CASE("apply: pure and ap on optional") {
    // optional is invoke-native; ap here exercises the base's DERIVED apply,
    // invoke(applicative_eval, cf, cx) -- GHC's (<*>) = liftA2 id.
    const auto &app = bt::applicative_typeclass<std::optional<int>>;
    auto lifted = app.pure([](int x) { return x * 10; });
    REQUIRE(app.ap(lifted, std::optional<int>{5}) == std::optional<int>{50});
    REQUIRE(app.ap(app.pure([](int x) { return x * 10; }),
                   std::optional<int>{}) == std::optional<int>{});
}

TEST_CASE("apply: identity and homomorphism laws on optional") {
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    REQUIRE(check_applicative_identity_law(std::optional<int>{7}));
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int x) { return x + 1; }, 41));
    // Generalized to the n-ary core: invoke(f, pure(x)...) == pure(f(x...)).
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int a, int b) { return a + b; }, 20, 22));
    REQUIRE(check_applicative_homomorphism_law<std::optional<int>>(
        [](int a, int b, int c) { return a * b * c; }, 2, 3, 7));
}

TEST_CASE("apply: interchange and composition laws on optional") {
    using bt::test::check_applicative_interchange_law;
    using bt::test::check_functor_composition_law;
    std::optional functions{[](int x) { return x - 5; }};
    REQUIRE(check_applicative_interchange_law(functions, 12));
    REQUIRE(check_functor_composition_law([](int x) { return x * 2; },
                                          [](int x) { return x + 1; },
                                          std::optional<int>{20}));
    REQUIRE(check_functor_composition_law([](int x) { return x * 2; },
                                          [](int x) { return x + 1; },
                                          std::optional<int>{}));
}

TEST_CASE("apply: laws hold for the Identity context") {
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    using bt::test::check_functor_composition_law;
    using Identity = bt::test::Identity<int>;
    REQUIRE(check_applicative_identity_law(Identity{9}));
    REQUIRE(check_applicative_homomorphism_law<Identity>(
        [](int a, int b) { return a - b; }, 50, 8));
    REQUIRE(check_functor_composition_law(
        [](int x) { return x / 2; }, [](int x) { return x + 6; }, Identity{4}));
}

TEST_CASE("apply: invoke_with delegates to another applicative map") {
    const auto &optional_map = bt::applicative_typeclass<std::optional<int>>;
    const auto &zip_map = bt::applicative_typeclass<bt::zip_list<int>>;
    auto result = optional_map.invoke_with(
        zip_map, [](int a, int b) { return a + b; },
        bt::zip_list<int>{{1, 2, 3}}, bt::zip_list<int>{{10, 20, 30}});
    REQUIRE(result.data == std::vector<int>{11, 22, 33});
}
