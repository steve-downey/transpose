// tests/beman/transpose/baseline_deduction.test.cpp                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// GOLDEN DEDUCTION TESTS -- stage baseline-capture of the grading plan
// (docs/transpose-grading-plan.md#baseline-capture).
//
// This file holds two kinds of assertion, and they are kept apart on
// purpose, because a failure has to mean one unambiguous thing.
//
// GOLDENS (sections 1-8) pin the EXACT type deduced by the library, and are
// expected to hold unchanged through every remaining stage. What makes a
// golden gold is precisely that nothing in the plan will legitimately change
// it: the additive-compatibility claim of
// docs/decisions.md#grading-footprint -- "for any input combination that
// deduces a type today, the graded framework deduces the same type" -- is
// proved by keeping them green, not by argument. A golden that fails is a
// regression. Never update one to make a stage pass; that inverts the
// sensor (divergence protocol rule 4).
//
// SCHEDULED ASSERTIONS (section 9) record facts that a NAMED later stage is
// planned to reverse. They are ordinary tests with an expiry date, not
// goldens -- a golden with a scheduled flip is a to-do wearing a sensor's
// clothes, and it teaches the reader to treat a red build as routine. Each
// one names the stage that retires it. When that stage lands, flipping the
// assertion is the expected outcome, not a caught regression.
//
// Everything here lives in unevaluated context, so the file is a
// compile-time artifact: if it compiles, the assertions hold.

#include <beman/transpose/transpose.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <expected>
#include <optional>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

namespace {

// -- Registration probes ---------------------------------------------------
// Availability is a NAMED CONCEPT on purpose, matching test_support.hpp: a
// bare requires-expression at block scope hard-errors on an invalid
// expression instead of yielding false ([expr.prim.req]). An unregistered
// typeclass lookup yields std::false_type; a registered one yields the map.

template <class T>
concept applicative_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::applicative_typeclass<T>)>,
                    std::false_type>;

template <class T>
concept monad_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::monad_typeclass<T>)>,
                    std::false_type>;

template <class T>
concept functor_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::functor_typeclass<T>)>,
                    std::false_type>;

template <class T>
concept traversable_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::traversable_typeclass<T>)>,
                    std::false_type>;

// -- Probe callables -------------------------------------------------------
// Defined, not merely declared: deducing a return type instantiates the
// template body, and an undefined callee would fail to link.

constexpr auto to_double(const int &x) -> double { return x * 2.0; }
constexpr auto add(const int &x, const int &y) -> int { return x + y; }

auto opt_of(const int &x) -> std::optional<int> {
    return std::optional<int>{x};
}
auto opt_double_of(const int &x) -> std::optional<double> {
    return std::optional<double>{static_cast<double>(x)};
}
auto sender_of(const int &x) -> bt::sender<int> {
    return bt::sender<int>::ready(x);
}
auto zip_of(const int &x) -> bt::zip_list<int> {
    return bt::zip_list<int>::repeat(x);
}

/** A callable held *inside* a context, to exercise the derived ap basis. */
struct doubler {
    constexpr auto operator()(const int &x) const -> double { return x * 2.0; }
};

// Shorthand for the operands, all as const lvalues.
using opt_int = std::optional<int>;
using opt_double = std::optional<double>;
using vec_int = std::vector<int>;

// =========================================================================
// 1. Applicative over std::optional -- the flagship invoke-native instance.
// =========================================================================

inline constexpr const auto &opt_app = bt::applicative_typeclass<opt_int>;

static_assert(std::is_same_v<decltype(opt_app.pure(std::declval<int>())),
                             std::optional<int>>);
static_assert(std::is_same_v<decltype(opt_app.lift(std::declval<int>())),
                             std::optional<int>>);
static_assert(
    std::is_same_v<decltype(opt_app.invoke(add, std::declval<const opt_int &>(),
                                           std::declval<const opt_int &>())),
                   std::optional<int>>);
static_assert(std::is_same_v<
              decltype(opt_app.map(to_double, std::declval<const opt_int &>())),
              std::optional<double>>);
static_assert(std::is_same_v<
              decltype(opt_app.zip_with(add, std::declval<const opt_int &>(),
                                        std::declval<const opt_int &>())),
              std::optional<int>>);
static_assert(std::is_same_v<decltype(opt_app.discard_first(
                                 std::declval<const opt_int &>(),
                                 std::declval<const opt_double &>())),
                             std::optional<double>>);
static_assert(std::is_same_v<decltype(opt_app.discard_second(
                                 std::declval<const opt_int &>(),
                                 std::declval<const opt_double &>())),
                             std::optional<int>>);

// The derived ap basis: available exactly because optional can hold a
// callable. ap(cf, cx) = invoke(applicative_eval, cf, cx).
static_assert(std::is_same_v<decltype(opt_app.ap(
                                 std::declval<const std::optional<doubler> &>(),
                                 std::declval<const opt_int &>())),
                             std::optional<double>>);

// =========================================================================
// 2. Monad over std::optional -- bind basis, invoke synthesized.
// =========================================================================

inline constexpr const auto &opt_monad = bt::monad_typeclass<opt_int>;

static_assert(
    std::is_same_v<decltype(opt_monad.bind(std::declval<const opt_int &>(),
                                           opt_double_of)),
                   std::optional<double>>);
static_assert(std::is_same_v<
              decltype(opt_monad.invoke(add, std::declval<const opt_int &>(),
                                        std::declval<const opt_int &>())),
              std::optional<int>>);
static_assert(std::is_same_v<decltype(bt::mbind(std::declval<const opt_int &>(),
                                                opt_double_of)),
                             std::optional<double>>);
static_assert(
    std::is_same_v<
        decltype(bt::join(std::declval<const std::optional<opt_int> &>())),
        std::optional<int>>);

// =========================================================================
// 3. Functor over std::optional and std::vector.
// =========================================================================

static_assert(std::is_same_v<decltype(bt::functor_typeclass<opt_int>.fmap(
                                 to_double, std::declval<const opt_int &>())),
                             std::optional<double>>);
static_assert(std::is_same_v<decltype(bt::functor_typeclass<vec_int>.fmap(
                                 to_double, std::declval<const vec_int &>())),
                             std::vector<double>>);
static_assert(std::is_same_v<decltype(bt::functor_typeclass<opt_int>.replace(
                                 std::declval<const opt_int &>(),
                                 std::declval<double>())),
                             std::optional<double>>);

// =========================================================================
// 4. Traversable over std::vector, through each shipped applicative.
//    These are the three front-door results of Paper A's motivating
//    domains, and the shape of every compatibility claim downstream.
// =========================================================================

static_assert(std::is_same_v<
              decltype(bt::traverse(opt_of, std::declval<const vec_int &>())),
              std::optional<std::vector<int>>>);
static_assert(
    std::is_same_v<decltype(bt::traverse(opt_double_of,
                                         std::declval<const vec_int &>())),
                   std::optional<std::vector<double>>>);
static_assert(std::is_same_v<decltype(bt::traverse(
                                 sender_of, std::declval<const vec_int &>())),
                             bt::sender<std::vector<int>>>);
static_assert(std::is_same_v<
              decltype(bt::traverse(zip_of, std::declval<const vec_int &>())),
              bt::zip_list<std::vector<int>>>);

// transpose is traverse(identity): structure<context<T>> ->
// context<structure<T>>
static_assert(std::is_same_v<decltype(bt::transpose(
                                 std::declval<const std::vector<opt_int> &>())),
                             std::optional<std::vector<int>>>);
static_assert(
    std::is_same_v<decltype(bt::transpose(
                       std::declval<const std::vector<bt::sender<int>> &>())),
                   bt::sender<std::vector<int>>>);

// The derived object operations deduce identically to the free functions.
static_assert(
    std::is_same_v<decltype(bt::traversable_typeclass<vec_int>.for_each(
                       std::declval<const vec_int &>(), opt_of)),
                   std::optional<std::vector<int>>>);

// =========================================================================
// 5. Traversable over the test Identity context.
// =========================================================================

using ident_int = bt::test::Identity<int>;

static_assert(
    std::is_same_v<decltype(bt::traversable_typeclass<ident_int>.for_each(
                       std::declval<const ident_int &>(), opt_of)),
                   std::optional<bt::test::Identity<int>>>);

// =========================================================================
// 6. Positional applicatives: std::array and the SoA/AoS tuple transpose.
// =========================================================================

using arr3 = std::array<int, 3>;

static_assert(std::is_same_v<decltype(bt::applicative_typeclass<arr3>.invoke(
                                 add, std::declval<const arr3 &>(),
                                 std::declval<const arr3 &>())),
                             std::array<int, 3>>);
static_assert(
    std::is_same_v<decltype(bt::transpose_tuple(
                       std::declval<const std::tuple<
                           std::array<int, 2>, std::array<double, 2>> &>())),
                   std::array<std::tuple<int, double>, 2>>);

// =========================================================================
// 7. The registration frontier.
//
// At stage baseline-capture all three of these read `!registered`: expected
// appeared nowhere in the library, so no expression over it deduced anything.
// Stage expected-instance
// (docs/transpose-grading-plan.md#expected-instance) flipped the applicative
// and monad deliberately, which is what the pin was here to make visible --
// the goldens failed on exactly these two lines when the instance landed.
// Functor is untouched and stays unregistered: the stage's scope is
// Applicative and Monad, and widening it silently is what the pin prevents.
// =========================================================================

using exp_int = std::expected<int, std::errc>;

static_assert(applicative_registered<exp_int>);
static_assert(monad_registered<exp_int>);
static_assert(!functor_registered<exp_int>);

// The bare value carries no applicative registration either. Under
// docs/decisions.md#empty-grade-spelling bare `T` IS the empty grade, so
// the promotion of bare values at grade zero (stage crtp-absorption) must
// arrive without registering `int` as a context. Pinned so that it cannot.
static_assert(!applicative_registered<int>);
static_assert(!monad_registered<int>);

// The STRUCTURE side is already ready for expected; only the CONTEXT side is
// missing. A vector of expected is a registered traversable today -- it just
// has nothing to transpose into.
static_assert(traversable_registered<std::vector<exp_int>>);
static_assert(traversable_registered<vec_int>);

// The element-extraction trait already handles expected via its nested
// value_type, with no specialization needed. Stage grade-concept depends on
// this holding.
static_assert(std::is_same_v<bt::applicative_value_t<exp_int>, int>);
static_assert(std::is_same_v<bt::applicative_value_t<opt_int>, int>);

// -- Contexts that ARE registered today, pinned as the complement. ---------
static_assert(applicative_registered<opt_int>);
static_assert(applicative_registered<bt::sender<int>>);
static_assert(applicative_registered<bt::zip_list<int>>);
static_assert(applicative_registered<arr3>);
static_assert(applicative_registered<ident_int>);
static_assert(monad_registered<opt_int>);
static_assert(functor_registered<opt_int>);
static_assert(functor_registered<vec_int>);

// =========================================================================
// 8. Stage expected-instance -- the ungraded before-state.
//
// These are the deductions the graded framework must reproduce
// character-for-character. Read them as the statement that
// docs/decisions.md#grading-footprint's no-spontaneous-singletons corollary
// is measurable: every result below is `expected<T, E>` with a bare error
// type, and not one of them is `expected<T, error_set<E>>`. A later stage
// that turns any of these into a singleton error_set has manufactured a
// grade out of an unmixed pipeline, which is the thing the corollary forbids.
// =========================================================================

/** A second, unrelated error type -- the other side of a mixing point. */
enum class other_errc { boom };

auto exp_of(const int &x) -> exp_int { return exp_int{x}; }
auto exp_double_of(const int &x) -> std::expected<double, std::errc> {
    return std::expected<double, std::errc>{static_cast<double>(x)};
}
auto exp_other_of(const int &x) -> std::expected<double, other_errc> {
    return std::expected<double, other_errc>{static_cast<double>(x)};
}

inline constexpr const auto &exp_app = bt::applicative_typeclass<exp_int>;
inline constexpr const auto &exp_monad = bt::monad_typeclass<exp_int>;

static_assert(std::is_same_v<decltype(exp_app.pure(std::declval<int>())),
                             std::expected<int, std::errc>>);
static_assert(
    std::is_same_v<decltype(exp_app.invoke(add, std::declval<const exp_int &>(),
                                           std::declval<const exp_int &>())),
                   std::expected<int, std::errc>>);
static_assert(std::is_same_v<
              decltype(exp_app.map(to_double, std::declval<const exp_int &>())),
              std::expected<double, std::errc>>);
static_assert(std::is_same_v<
              decltype(exp_app.discard_first(std::declval<const exp_int &>(),
                                             std::declval<const exp_int &>())),
              std::expected<int, std::errc>>);
static_assert(
    std::is_same_v<decltype(exp_monad.bind(std::declval<const exp_int &>(),
                                           exp_double_of)),
                   std::expected<double, std::errc>>);
static_assert(std::is_same_v<decltype(bt::mbind(std::declval<const exp_int &>(),
                                                exp_double_of)),
                             std::expected<double, std::errc>>);
static_assert(std::is_same_v<
              decltype(bt::join(
                  std::declval<const std::expected<exp_int, std::errc> &>())),
              std::expected<int, std::errc>>);
static_assert(std::is_same_v<
              decltype(bt::traverse(exp_of, std::declval<const vec_int &>())),
              std::expected<std::vector<int>, std::errc>>);
static_assert(std::is_same_v<decltype(bt::transpose(
                                 std::declval<const std::vector<exp_int> &>())),
                             std::expected<std::vector<int>, std::errc>>);

// -- The mixing frontier, which stays closed until graded-deduction. -------
// One pinned error type per instance object, so an operand carrying a
// different one fails deduction on the Impl basis. The probes are named
// concepts for the usual reason: a bare requires-expression at block scope
// would hard-error rather than yield false.

template <class MAP, class FUNCTION, class... OPERANDS>
concept invocable_through =
    requires(const MAP &map, FUNCTION function, const OPERANDS &...operands) {
        map.invoke(function, operands...);
    };

template <class MAP, class MA, class FUNCTION>
concept bindable_through =
    requires(const MAP &map, const MA &ma, FUNCTION function) {
        map.bind(ma, function);
    };

using exp_app_t = std::remove_cvref_t<decltype(exp_app)>;
using exp_monad_t = std::remove_cvref_t<decltype(exp_monad)>;
using exp_other = std::expected<int, other_errc>;

// Same error type on every operand: fine.
static_assert(invocable_through<exp_app_t, decltype(add), exp_int, exp_int>);
// Mixing two error types: ill-formed, and cleanly so.
static_assert(!invocable_through<exp_app_t, decltype(add), exp_int, exp_other>);
static_assert(!invocable_through<exp_app_t, decltype(add), exp_other, exp_int>);

// Same for bind: the continuation must return the instance's error type.
static_assert(bindable_through<exp_monad_t, exp_int, decltype(exp_double_of)>);
static_assert(!bindable_through<exp_monad_t, exp_int, decltype(exp_other_of)>);

} // namespace

TEST_CASE("baseline-capture: golden deductions hold at this commit") {
    // Every assertion in this translation unit is a static_assert; reaching
    // this line means the ungraded baseline is intact.
    SUCCEED("golden deduction matrix compiled");
}
