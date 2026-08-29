// tests/beman/transpose/grade.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/grade.hpp>

#include <beman/transpose/error_set.hpp>
#include <beman/transpose/transpose.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <expected>
#include <optional>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

namespace bt = beman::transpose;

namespace beman_transpose_grade_probes {

struct alpha {
    int value{};
};
struct beta {
    int value{};
};

} // namespace beman_transpose_grade_probes

namespace probes = beman_transpose_grade_probes;

using probes::alpha;
using probes::beta;

using set_a = bt::error_set<alpha>;
using set_ab = bt::error_set<alpha, beta>;

namespace {

// =========================================================================
// 1. unit_grade is a grade in its own right, and the identity of join in
//    every algebra. It is what lets the framework say "raises nothing"
//    without naming an error type.
// =========================================================================

static_assert(bt::grade_semilattice<bt::unit_grade>);
static_assert(bt::grade_semilattice<set_a>);
static_assert(bt::grade_semilattice<set_ab>);

// A type nobody registered is not a grade. The primaries are undefined on
// purpose, so this cannot pass by accident.
static_assert(!bt::grade_semilattice<int>);
static_assert(!bt::grade_semilattice<std::optional<int>>);
static_assert(!bt::grade_semilattice<std::variant<alpha, beta>>);

static_assert(std::is_same_v<bt::grade_join_t<bt::unit_grade, bt::unit_grade>,
                             bt::unit_grade>);
static_assert(
    std::is_same_v<bt::grade_bottom_t<bt::unit_grade>, bt::unit_grade>);

// ∅ is the identity of the shipped model's join, from either side.
static_assert(std::is_same_v<bt::grade_join_t<bt::unit_grade, set_ab>, set_ab>);
static_assert(std::is_same_v<bt::grade_join_t<set_ab, bt::unit_grade>, set_ab>);

// ∅ subsumes into everything; nothing but ∅ subsumes into ∅.
static_assert(bt::grade_subsumes_v<bt::unit_grade, bt::unit_grade>);
static_assert(bt::grade_subsumes_v<bt::unit_grade, set_ab>);
static_assert(!bt::grade_subsumes_v<set_ab, bt::unit_grade>);

// =========================================================================
// 2. The shipped model, reached through grade vocabulary only. These
//    assertions never mention error_set_join_t -- they go through the
//    framework verbs, which is the whole point of the registration.
// =========================================================================

static_assert(
    std::is_same_v<bt::grade_join_t<set_a, bt::error_set<beta>>, set_ab>);
static_assert(std::is_same_v<bt::grade_join_t<set_ab, set_a>, set_ab>);
static_assert(std::is_same_v<bt::grade_bottom_t<set_ab>, bt::error_set<>>);
static_assert(bt::grade_subsumes_v<set_a, set_ab>);
static_assert(!bt::grade_subsumes_v<set_ab, set_a>);

// Order-from-join, through the framework verbs.
template <class LEFT, class RIGHT>
inline constexpr bool order_agrees_with_join =
    bt::grade_subsumes_v<LEFT, RIGHT> ==
    std::is_same_v<bt::grade_join_t<LEFT, RIGHT>, RIGHT>;

static_assert(order_agrees_with_join<set_a, set_ab>);
static_assert(order_agrees_with_join<set_ab, set_a>);
static_assert(order_agrees_with_join<bt::unit_grade, set_ab>);
static_assert(order_agrees_with_join<set_ab, bt::unit_grade>);

// =========================================================================
// 3. STAGE ACCEPTANCE: grade_of on every baseline-capture matrix type
//    yields ∅ except genuine carriers.
//
// This is the tripwire of stage grade-concept
// (docs/transpose-grading-plan.md#grade-concept): any matrix type
// classified as graded that today flows through the identity path means
// the nominal fence of docs/decisions.md#error-set-identity has a hole.
// =========================================================================

template <class T>
inline constexpr bool is_ungraded =
    std::is_same_v<bt::grade_of_t<T>, bt::unit_grade>;

// Bare values and every pre-grading carrier.
static_assert(is_ungraded<int>);
static_assert(is_ungraded<double>);
static_assert(is_ungraded<std::optional<int>>);
static_assert(is_ungraded<std::vector<int>>);
static_assert(is_ungraded<std::array<int, 3>>);
static_assert(is_ungraded<bt::sender<int>>);
static_assert(is_ungraded<bt::zip_list<int>>);
static_assert(is_ungraded<bt::test::Identity<int>>);

// The carrier stage expected-instance introduced. An unmixed expected over a
// bare error type is UNGRADED -- this is the no-spontaneous-singletons
// corollary of docs/decisions.md#grading-footprint, at the trait level.
static_assert(is_ungraded<std::expected<int, std::errc>>);
static_assert(is_ungraded<std::expected<int, alpha>>);

// The datum-vs-grade fence. A variant error alternative is a value-level sum
// and stays ungraded; only the nominal error_set is a grade carrier. If this
// ever flips, structural detection has started conscripting the unwilling.
static_assert(is_ungraded<std::expected<int, std::variant<alpha, beta>>>);

// Genuine carriers, and only these.
static_assert(!is_ungraded<std::expected<int, set_ab>>);
static_assert(
    std::is_same_v<bt::grade_of_t<std::expected<int, set_ab>>, set_ab>);
static_assert(std::is_same_v<bt::grade_of_t<std::expected<int, set_a>>, set_a>);

// graded_context is the constraint the framework uses to keep the two paths
// mutually exclusive rather than merely ranked.
static_assert(bt::graded_context<std::expected<int, set_ab>>);
static_assert(!bt::graded_context<std::expected<int, std::errc>>);
static_assert(!bt::graded_context<std::optional<int>>);
static_assert(!bt::graded_context<int>);

// =========================================================================
// 4. rebind_grade, including the empty-grade-spelling sentinel.
// =========================================================================

// Re-indexing anything at ∅ leaves it alone -- the framework default.
static_assert(std::is_same_v<bt::rebind_grade_t<int, bt::unit_grade>, int>);
static_assert(
    std::is_same_v<bt::rebind_grade_t<std::optional<int>, bt::unit_grade>,
                   std::optional<int>>);

// Promotion: a bare value re-indexed at a non-empty set acquires a carrier.
static_assert(std::is_same_v<bt::rebind_grade_t<int, set_ab>,
                             std::expected<int, set_ab>>);

// Re-indexing a carrier replaces its grade rather than nesting.
static_assert(
    std::is_same_v<bt::rebind_grade_t<std::expected<int, set_a>, set_ab>,
                   std::expected<int, set_ab>>);
static_assert(
    std::is_same_v<bt::rebind_grade_t<std::expected<int, std::errc>, set_ab>,
                   std::expected<int, set_ab>>);

// SENTINEL (docs/decisions.md#empty-grade-spelling): re-indexing at the empty
// set yields the BARE value. No framework path may materialize
// expected<T, error_set<>>; the uniform form stays an explicit spelling only.
static_assert(std::is_same_v<bt::rebind_grade_t<int, bt::error_set<>>, int>);
static_assert(
    std::is_same_v<
        bt::rebind_grade_t<std::expected<int, set_ab>, bt::error_set<>>, int>);
static_assert(!std::is_same_v<bt::rebind_grade_t<int, bt::error_set<>>,
                              std::expected<int, bt::error_set<>>>);

// Round-trip: grade_of after rebind_grade returns what was asked for.
static_assert(
    std::is_same_v<bt::grade_of_t<bt::rebind_grade_t<int, set_ab>>, set_ab>);
static_assert(
    std::is_same_v<bt::grade_of_t<bt::rebind_grade_t<int, bt::unit_grade>>,
                   bt::unit_grade>);

} // namespace

TEST_CASE("grade: the framework layer names no error vocabulary") {
    // Compile-time only. Reaching here means the ∅ default holds for every
    // pre-grading carrier and the nominal fence admits only error_set.
    SUCCEED("grade traits and algebra verbs compiled");
}
