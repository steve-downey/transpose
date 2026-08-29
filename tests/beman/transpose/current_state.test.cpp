// tests/beman/transpose/current_state.test.cpp                       -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// CURRENT-STATE ASSERTIONS. These are NOT goldens.
//
// A golden is defined by not changing: `baseline_deduction.test.cpp` records
// deductions expected to hold unchanged from stage baseline-capture through
// stage paper-revision, and rule 4 of the divergence protocol
// (docs/transpose-grading-plan.md#divergence-protocol) says never to update
// one to make a stage pass.
//
// Everything in THIS file is the opposite kind of assertion. It records what
// the library does today, at a point the plan intends to move, and names the
// stage that will move it. Editing this file is ordinary work; editing a
// golden is a stop-and-ask. Keeping the two kinds in separate translation
// units is what keeps that distinction legible -- a golden with a scheduled
// flip would teach the reader that a red build is routine, and rule 4 depends
// on that not being true.
//
// Each block below states its expiry. When the named stage lands, the block
// goes green in its new form or goes away; it does not get argued with.
//
// See docs/decisions.md#golden-vs-scheduled-assertions.

#include <beman/transpose/transpose.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <system_error>
#include <type_traits>

namespace bt = beman::transpose;

namespace {

// =========================================================================
// THE MIXING FRONTIER.  Expiry: stage graded-deduction.
//
// Today each expected instance object pins one error type, so combining
// expected<T,E1> with expected<T,E2> fails deduction. That is exactly the
// "previously-ill-formed territory" of
// docs/decisions.md#grading-footprint, and stage graded-deduction
// (docs/transpose-grading-plan.md#graded-deduction) exists to open it: after
// that stage the mixed cases are well-formed and deduce
// expected<T, error_set<E1,E2>>.
//
// These negatives are DUE to flip, by a named stage, for a recorded reason.
// They are here to prove the territory is still shut in the meantime, so that
// opening it is a deliberate act rather than a drift nobody noticed.
//
// WHEN graded-deduction LANDS: replace each negative with the positive
// deduction it becomes, verify it, and move the result into
// baseline_deduction.test.cpp, where it becomes a golden for the rest of the
// plan. Do not leave a flipped assertion here.
// =========================================================================

/** A second, unrelated error type -- the other side of a mixing point. */
enum class other_errc { boom };

using exp_int = std::expected<int, std::errc>;
using exp_other = std::expected<int, other_errc>;

constexpr auto add(const int &x, const int &y) -> int { return x + y; }

auto exp_double_of(const int &x) -> std::expected<double, std::errc> {
    return std::expected<double, std::errc>{static_cast<double>(x)};
}
auto exp_other_of(const int &x) -> std::expected<double, other_errc> {
    return std::expected<double, other_errc>{static_cast<double>(x)};
}

inline constexpr const auto &exp_app = bt::applicative_typeclass<exp_int>;
inline constexpr const auto &exp_monad = bt::monad_typeclass<exp_int>;

using exp_app_t = std::remove_cvref_t<decltype(exp_app)>;
using exp_monad_t = std::remove_cvref_t<decltype(exp_monad)>;

// The probes are named concepts for the usual reason: a bare
// requires-expression at block scope would hard-error rather than yield false
// ([expr.prim.req]).

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

// CONTROLS, and these do NOT expire. Without them the negatives could pass by
// being vacuously false, and a probe that never matches anything proves
// nothing. Unmixed operands stay invocable through every stage, so these two
// hold before and after graded-deduction alike.
static_assert(invocable_through<exp_app_t, decltype(add), exp_int, exp_int>);
static_assert(bindable_through<exp_monad_t, exp_int, decltype(exp_double_of)>);

// SCHEDULED. Mixing two error types is ill-formed today, and cleanly so: the
// deduction fails on the Impl basis rather than tripping a static_assert
// inside the CRTP derivation. graded-deduction reverses all three.
static_assert(!invocable_through<exp_app_t, decltype(add), exp_int, exp_other>);
static_assert(!invocable_through<exp_app_t, decltype(add), exp_other, exp_int>);
static_assert(!bindable_through<exp_monad_t, exp_int, decltype(exp_other_of)>);

} // namespace

TEST_CASE("current-state: the mixing frontier is still shut") {
    // Compile-time only. This case exists so the coverage is visible to
    // ctest, and so the expiry is discoverable by running the suite rather
    // than only by reading the file.
    SUCCEED("mixed error types do not combine yet; graded-deduction opens them");
}
