// tests/beman/transpose/induced_monoid.test.cpp                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/induced_monoid.hpp>

#include <beman/transpose/apply.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/monoid.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <optional>

namespace bt = beman::transpose;

namespace {

// Sentinel: the raw carriers these monoids are induced over do not
// themselves gain a registered Monoid. An induced monoid is an instance the
// caller passes, never a registration on the context type; operationally,
// this concept stays false for the context.
// Probed on the specialization itself, never through monoid_v: naming
// monoid_v<T> declares a variable of the undefined type Monoid<T>, and that
// failure is outside the requires-expression's immediate context -- a hard
// error, not `false` (same shape as the sentinel in monoid.test.cpp).
template <class T>
concept has_monoid = requires { sizeof(bt::Monoid<T>); } &&
                     requires(const bt::Monoid<T> &m) { m.identity(); };

static_assert(!has_monoid<std::optional<int>>);

} // namespace

// ============================================================================
// kleisli_monoid: the Kleisli endomorphism monoid over std::optional<int>.
// Identity is pure, combine is Kleisli composition (>=>). The Kleisli-form
// monad laws (pure is the two-sided unit of >=>, and >=> is associative) are
// exactly the three monoid laws checked below, applied "as functions": two
// arrows are compared by applying both to the same inputs and comparing
// results, since the erased value type is a std::function and has no
// operator==.
// ============================================================================

namespace {

using MonadObj = bt::OptionalMonadMap<int>;
using Kleisli = bt::kleisli_monoid<MonadObj, int>;
using Arrow = Kleisli::value_type;

static_assert(bt::monoid_object<Kleisli, Arrow>);

// Arrows exercising both the engaged and disengaged branches of the
// underlying std::optional context.
auto increment_below_100(const int &a) -> std::optional<int> {
    return a < 100 ? std::optional<int>{a + 1} : std::optional<int>{};
}

auto doubled(const int &a) -> std::optional<int> {
    return std::optional<int>{a * 2};
}

auto decrement_above_zero(const int &a) -> std::optional<int> {
    return a > 0 ? std::optional<int>{a - 1} : std::optional<int>{};
}

constexpr std::array sample_inputs{-3, 0, 1, 50, 99, 100};

// Compares two arrows "as functions": applies both to every sample input and
// checks that the results agree.
template <class F, class G>
auto agree_on_samples(const F &f, const G &g) -> bool {
    for (int x : sample_inputs) {
        if (f(x) != g(x)) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST_CASE("induced_monoid: kleisli_monoid identity is pure") {
    auto id = Kleisli{}.identity();
    for (int x : sample_inputs) {
        REQUIRE(id(x) == std::optional<int>{x});
    }
}

TEST_CASE("induced_monoid: kleisli_monoid left identity -- combine(pure, f) "
          "agrees with f") {
    Kleisli k;
    Arrow f{increment_below_100};
    auto composed = k.combine(k.identity(), f);
    REQUIRE(agree_on_samples(composed, f));
}

TEST_CASE("induced_monoid: kleisli_monoid right identity -- combine(f, pure) "
          "agrees with f") {
    Kleisli k;
    Arrow f{increment_below_100};
    auto composed = k.combine(f, k.identity());
    REQUIRE(agree_on_samples(composed, f));
}

TEST_CASE("induced_monoid: kleisli_monoid combine is >=> -- Kleisli forward "
          "composition") {
    Kleisli k;
    Arrow f{increment_below_100};
    Arrow g{doubled};
    auto composed = k.combine(f, g);
    for (int x : sample_inputs) {
        auto expected = bt::mbind(f(x), doubled);
        REQUIRE(composed(x) == expected);
    }
}

TEST_CASE("induced_monoid: kleisli_monoid associativity -- the Kleisli-form "
          "monad law") {
    Kleisli k;
    Arrow f{increment_below_100};
    Arrow g{doubled};
    Arrow h{decrement_above_zero};

    auto left = k.combine(k.combine(f, g), h);
    auto right = k.combine(f, k.combine(g, h));

    REQUIRE(agree_on_samples(left, right));
}

// ============================================================================
// lifted_monoid: the applicative-lifted monoid, F<A> from an instance over
// A. Lifted here over std::optional<int> under addition: sum_monoid<int>
// supplies the element monoid, std::optional<int> -- unwrapped -- is the
// F<A> that becomes a monoid.
// ============================================================================

namespace {

using ApplicativeObj = bt::OptionalApplicativeMap<int>;
using Lifted =
    bt::lifted_monoid<ApplicativeObj, std::optional<int>, bt::sum_monoid<int>>;

static_assert(bt::monoid_object<Lifted, std::optional<int>>);
static_assert(std::is_same_v<bt::monoid_value_t<Lifted>, std::optional<int>>);

// The element instance defaults to the registration where one exists.
static_assert(
    std::is_same_v<bt::lifted_monoid<bt::OptionalApplicativeMap<std::string>,
                                     std::optional<std::string>>,
                   bt::lifted_monoid<bt::OptionalApplicativeMap<std::string>,
                                     std::optional<std::string>,
                                     bt::Monoid<std::string>>>);

// Checks the three monoid laws for a handful of values under the lifted
// instance, including a disengaged operand -- the same shape as
// monoid.test.cpp's local law-checker.
auto check_lifted_laws(const std::optional<int> &a, const std::optional<int> &b,
                       const std::optional<int> &c) -> bool {
    Lifted m;
    const auto e = m.identity();
    const bool left_identity = m.combine(e, a) == a;
    const bool right_identity = m.combine(a, e) == a;
    const bool associative =
        m.combine(m.combine(a, b), c) == m.combine(a, m.combine(b, c));
    return left_identity && right_identity && associative;
}

} // namespace

TEST_CASE("induced_monoid: lifted_monoid identity is pure(identity_A)") {
    REQUIRE(Lifted{}.identity() == std::optional<int>{0});
}

TEST_CASE("induced_monoid: lifted_monoid combine is invoke(combine_A) over "
          "engaged operands") {
    REQUIRE(Lifted{}.combine(std::optional<int>{3}, std::optional<int>{4}) ==
            std::optional<int>{7});
}

TEST_CASE("induced_monoid: lifted_monoid combine propagates a disengaged "
          "operand") {
    Lifted m;
    REQUIRE(m.combine(std::optional<int>{3}, std::optional<int>{}) ==
            std::optional<int>{});
    REQUIRE(m.combine(std::optional<int>{}, std::optional<int>{3}) ==
            std::optional<int>{});
}

TEST_CASE("induced_monoid: lifted_monoid satisfies the monoid laws, engaged "
          "operands") {
    REQUIRE(check_lifted_laws(2, 3, 5));
}

TEST_CASE("induced_monoid: lifted_monoid satisfies the monoid laws with a "
          "disengaged operand") {
    REQUIRE(check_lifted_laws(2, std::nullopt, 5));
}

TEST_CASE("induced_monoid: lifted_monoid holds its element instance by "
          "value") {
    // The element monoid is state the lift carries, not a type it looks up:
    // the same context lifts under max by passing a different instance.
    bt::lifted_monoid<ApplicativeObj, std::optional<int>, bt::max_monoid<int>>
        m;
    REQUIRE(m.combine(std::optional<int>{3}, std::optional<int>{4}) ==
            std::optional<int>{4});
}
