// tests/beman/transpose/monoid.test.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/monoid.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace bt = beman::transpose;

namespace {

// Checks the three monoid laws for a handful of values under an instance
// `m`. Local to this test file: it speaks plain monoid vocabulary, not the
// graded vocabulary of laws.hpp, so it does not belong there.
template <class M, class T>
constexpr auto check_monoid_laws(const M &m, const T &a, const T &b, const T &c)
    -> bool {
    const auto e = m.identity();
    const bool left_identity = m.combine(e, a) == a;
    const bool right_identity = m.combine(a, e) == a;
    const bool associative =
        m.combine(m.combine(a, b), c) == m.combine(a, m.combine(b, c));
    return left_identity && right_identity && associative;
}

static_assert(check_monoid_laws(bt::sum_monoid<int>{}, 2, 3, 5));
static_assert(check_monoid_laws(bt::product_monoid<int>{}, 2, 3, 5));
static_assert(check_monoid_laws(bt::max_monoid<int>{}, 2, 3, 5));
static_assert(check_monoid_laws(bt::min_monoid<int>{}, 2, 3, 5));
static_assert(check_monoid_laws(bt::any_monoid{}, false, true, false));
static_assert(check_monoid_laws(bt::all_monoid{}, true, false, true));
static_assert(check_monoid_laws(bt::first_monoid<int>{}, std::optional<int>{},
                                std::optional<int>{3}, std::optional<int>{5}));
static_assert(check_monoid_laws(bt::dual_monoid<bt::sum_monoid<int>>{}, 2, 3,
                                5));
static_assert(check_monoid_laws(
    bt::tuple_monoid<bt::sum_monoid<int>, bt::max_monoid<int>>{},
    std::tuple{2, 2}, std::tuple{3, 3}, std::tuple{5, 5}));

// The instances are monoid objects over their bare value type, and over
// nothing else.
static_assert(bt::monoid_object<bt::sum_monoid<int>, int>);
static_assert(!bt::monoid_object<bt::sum_monoid<int>, std::string>);
static_assert(bt::monoid_object<bt::any_monoid, bool>);
static_assert(bt::monoid_object<bt::first_monoid<int>, std::optional<int>>);
static_assert(bt::monoid_object<bt::Monoid<std::string>, std::string>);
static_assert(
    bt::monoid_object<bt::tuple_monoid<bt::sum_monoid<int>, bt::any_monoid>,
                      std::tuple<int, bool>>);
static_assert(
    std::is_same_v<bt::monoid_value_t<bt::max_monoid<double>>, double>);
static_assert(
    std::is_same_v<bt::monoid_value_t<bt::dual_monoid<bt::Monoid<std::string>>>,
                   std::string>);

// Sentinel: no bare-numeric or boolean Monoid is registered, so the choice
// among addition, product, max, min, ... must be spelled by passing an
// instance (docs/decisions.md#monoid-selection). A future re-registration
// of Monoid<int> (or long, size_t, bool) fails these static_asserts rather
// than silently restoring an unnamed default.
// Probed on the specialization itself, never through monoid_v: naming
// monoid_v<T> declares a variable of the undefined type Monoid<T>, and that
// failure is outside the requires-expression's immediate context -- a hard
// error, not `false`.
template <class T>
concept has_monoid = requires {
    sizeof(beman::transpose::Monoid<T>);
} && requires(const beman::transpose::Monoid<T> &m) { m.identity(); };

static_assert(!has_monoid<int>);
static_assert(!has_monoid<long>);
static_assert(!has_monoid<std::size_t>);
static_assert(!has_monoid<bool>);
static_assert(has_monoid<std::string>);
static_assert(has_monoid<std::vector<int>>);
static_assert(bt::detail::has_registered_monoid<std::string>);
static_assert(!bt::detail::has_registered_monoid<int>);

} // namespace

TEST_CASE("monoid: string concatenation") {
    REQUIRE(bt::monoid_identity<std::string>().empty());
    REQUIRE(bt::monoid_combine(std::string{"ab"}, std::string{"cd"}) == "abcd");
}

TEST_CASE("monoid: vector concatenation") {
    std::vector<int> lhs{1, 2};
    std::vector<int> rhs{3, 4};
    REQUIRE(bt::monoid_combine(lhs, rhs) == std::vector<int>{1, 2, 3, 4});
    REQUIRE(bt::monoid_identity<std::vector<int>>().empty());
}

TEST_CASE("monoid: sum_monoid combines by addition") {
    bt::sum_monoid<int> m;
    REQUIRE(check_monoid_laws(m, 2, 3, 5));
    REQUIRE(m.identity() == 0);
    REQUIRE(m.combine(2, 5) == 7);
}

TEST_CASE("monoid: product_monoid combines by multiplication") {
    bt::product_monoid<int> m;
    REQUIRE(check_monoid_laws(m, 2, 3, 5));
    REQUIRE(m.identity() == 1);
    REQUIRE(m.combine(2, 5) == 10);
}

TEST_CASE("monoid: max_monoid combines by taking the larger value") {
    bt::max_monoid<int> m;
    REQUIRE(check_monoid_laws(m, 2, 3, 5));
    REQUIRE(m.combine(2, 5) == 5);
}

TEST_CASE("monoid: min_monoid combines by taking the smaller value") {
    bt::min_monoid<int> m;
    REQUIRE(check_monoid_laws(m, 2, 3, 5));
    REQUIRE(m.combine(2, 5) == 2);
}

TEST_CASE("monoid: max_monoid identity is the saturating lower bound of T") {
    REQUIRE(bt::max_monoid<int>{}.identity() ==
            std::numeric_limits<int>::lowest());
    REQUIRE(bt::max_monoid<double>{}.identity() ==
            -std::numeric_limits<double>::infinity());
}

TEST_CASE("monoid: min_monoid identity is the saturating upper bound of T") {
    REQUIRE(bt::min_monoid<int>{}.identity() ==
            std::numeric_limits<int>::max());
    REQUIRE(bt::min_monoid<double>{}.identity() ==
            std::numeric_limits<double>::infinity());
}

TEST_CASE("monoid: any_monoid combines by logical or") {
    bt::any_monoid m;
    REQUIRE(check_monoid_laws(m, false, true, false));
    REQUIRE(m.identity() == false);
    REQUIRE(m.combine(false, true) == true);
}

TEST_CASE("monoid: all_monoid combines by logical and") {
    bt::all_monoid m;
    REQUIRE(check_monoid_laws(m, true, false, true));
    REQUIRE(m.identity() == true);
    REQUIRE(m.combine(false, true) == false);
}

TEST_CASE("monoid: first_monoid keeps the first engaged operand") {
    bt::first_monoid<int> m;
    REQUIRE(m.identity() == std::nullopt);
    REQUIRE(m.combine(std::optional<int>{}, std::optional<int>{4}) == 4);
    REQUIRE(m.combine(std::optional<int>{3}, std::optional<int>{4}) == 3);
    REQUIRE(m.combine(std::optional<int>{}, std::optional<int>{}) ==
            std::nullopt);
}

TEST_CASE("monoid: dual_monoid flips combine argument order") {
    bt::dual_monoid<bt::Monoid<std::string>> d;
    REQUIRE(d.combine(std::string{"ab"}, std::string{"cd"}) == "cdab");
    REQUIRE(d.identity().empty());
    REQUIRE(check_monoid_laws(d, std::string{"a"}, std::string{"b"},
                              std::string{"c"}));
}

TEST_CASE("monoid: tuple_monoid is elementwise over std::tuple") {
    bt::tuple_monoid<bt::sum_monoid<int>, bt::max_monoid<int>,
                     bt::Monoid<std::string>>
        m;
    REQUIRE((m.identity() ==
             std::tuple{0, std::numeric_limits<int>::lowest(), std::string{}}));
    REQUIRE((m.combine(std::tuple{2, 2, std::string{"a"}},
                       std::tuple{5, 5, std::string{"b"}}) ==
             std::tuple{7, 5, std::string{"ab"}}));
}

TEST_CASE("monoid: a combinator holds a stateful operand by value") {
    // A monoid instance may carry state (docs/decisions.md#monoid-selection,
    // point 4); the combinator stores its operand as given, copies included.
    struct modular_sum {
        int d_modulus;
        auto identity() const -> int { return 0; }
        auto combine(int a, int b) const -> int { return (a + b) % d_modulus; }
    };
    bt::dual_monoid<modular_sum> d{modular_sum{5}};
    REQUIRE(d.combine(3, 4) == 2);
    bt::tuple_monoid<modular_sum, modular_sum> t{
        {modular_sum{5}, modular_sum{7}}};
    REQUIRE(
        (t.combine(std::tuple{3, 3}, std::tuple{4, 4}) == std::tuple{2, 0}));
}
