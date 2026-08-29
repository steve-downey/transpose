// tests/beman/transpose/error_set.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/error_set.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace bt = beman::transpose;

// Named types with external linkage and a stable spelling -- the documented
// restrictions of the interim P2830 fallback. NOT in an anonymous namespace,
// deliberately: these probes must satisfy the restriction they are testing.
namespace beman_transpose_error_probes {

struct alpha {
    int value{};
};
struct beta {
    int value{};
};
struct gamma {
    int value{};
};

} // namespace beman_transpose_error_probes

namespace probes = beman_transpose_error_probes;

using probes::alpha;
using probes::beta;
using probes::gamma;

using empty_set = bt::error_set<>;
using set_a = bt::error_set<alpha>;
using set_b = bt::error_set<beta>;
using set_ab = bt::error_set<alpha, beta>;
using set_abc = bt::error_set<alpha, beta, gamma>;

// =========================================================================
// Normalization. The load-bearing property: order of spelling is not part
// of the identity, so grade arithmetic is well-defined and inclusions are
// unique.
// =========================================================================

static_assert(
    std::is_same_v<bt::error_set<alpha, beta>, bt::error_set<beta, alpha>>);
static_assert(std::is_same_v<bt::error_set<alpha, beta, gamma>,
                             bt::error_set<gamma, alpha, beta>>);
static_assert(std::is_same_v<bt::error_set<gamma, beta, alpha>,
                             bt::error_set<alpha, gamma, beta>>);

// Duplicates collapse, in any position and any multiplicity.
static_assert(std::is_same_v<bt::error_set<alpha, alpha>, set_a>);
static_assert(std::is_same_v<bt::error_set<alpha, beta, alpha>, set_ab>);
static_assert(std::is_same_v<bt::error_set<beta, alpha, beta, alpha>, set_ab>);

// Distinct sets stay distinct.
static_assert(!std::is_same_v<set_a, set_b>);
static_assert(!std::is_same_v<set_a, set_ab>);
static_assert(!std::is_same_v<empty_set, set_a>);

// =========================================================================
// Semilattice laws, on hand-picked sets over three generators. Eight
// elements is the free semilattice on three, so every law shape is
// exercised by concrete instances.
// =========================================================================

// Unit: bottom is the identity of join.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, empty_set>, set_a>);
static_assert(std::is_same_v<bt::error_set_join_t<empty_set, set_a>, set_a>);
static_assert(
    std::is_same_v<bt::error_set_join_t<empty_set, empty_set>, empty_set>);
static_assert(std::is_same_v<bt::error_set_bottom, empty_set>);

// Idempotence -- the property that keeps fold grades shape-independent.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_a>, set_a>);
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_ab>, set_ab>);

// Commutativity -- the property that makes grade arithmetic order-free.
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_b>,
                             bt::error_set_join_t<set_b, set_a>>);
static_assert(std::is_same_v<bt::error_set_join_t<set_a, set_b>, set_ab>);

// Associativity.
static_assert(std::is_same_v<
              bt::error_set_join_t<bt::error_set_join_t<set_a, set_b>,
                                   bt::error_set<gamma>>,
              bt::error_set_join_t<
                  set_a, bt::error_set_join_t<set_b, bt::error_set<gamma>>>>);
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, bt::error_set<gamma>>,
                             set_abc>);

// Overlapping joins do not double-count.
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_a>, set_ab>);
static_assert(std::is_same_v<bt::error_set_join_t<set_ab, set_abc>, set_abc>);

// =========================================================================
// Order from join: e ⊆ f  ⟺  e ∪ f = f. Subsumption is not an independent
// relation, it is read off the join -- which is what makes the coercions
// canonical and therefore coherently implicit.
// =========================================================================

template <class LEFT, class RIGHT>
inline constexpr bool order_agrees_with_join =
    bt::error_set_subsumes_v<LEFT, RIGHT> ==
    std::is_same_v<bt::error_set_join_t<LEFT, RIGHT>, RIGHT>;

static_assert(order_agrees_with_join<empty_set, set_a>);
static_assert(order_agrees_with_join<set_a, set_ab>);
static_assert(order_agrees_with_join<set_ab, set_a>);
static_assert(order_agrees_with_join<set_a, set_b>);
static_assert(order_agrees_with_join<set_ab, set_abc>);
static_assert(order_agrees_with_join<set_abc, set_ab>);
static_assert(order_agrees_with_join<set_a, set_a>);

// Spelled out, so a broken join and a broken order cannot agree vacuously.
static_assert(bt::error_set_subsumes_v<set_a, set_ab>);
static_assert(bt::error_set_subsumes_v<empty_set, set_abc>);
static_assert(bt::error_set_subsumes_v<set_a, set_a>);
static_assert(!bt::error_set_subsumes_v<set_ab, set_a>);
static_assert(!bt::error_set_subsumes_v<set_a, set_b>);

// =========================================================================
// Conversions: exactly the ⊆ widenings, and nothing else. A non-subset
// conversion compiling is a stage tripwire, so both directions are pinned.
// =========================================================================

template <class FROM, class TO>
concept converts_to = std::is_convertible_v<FROM, TO>;

// Widening along ⊆ is available.
static_assert(converts_to<set_a, set_ab>);
static_assert(converts_to<set_b, set_ab>);
static_assert(converts_to<set_a, set_abc>);
static_assert(converts_to<set_ab, set_abc>);

// Narrowing is not.
static_assert(!converts_to<set_ab, set_a>);
static_assert(!converts_to<set_abc, set_ab>);

// Incomparable sets do not convert in either direction.
static_assert(!converts_to<set_a, set_b>);
static_assert(!converts_to<set_b, set_a>);

// Injection: an error value becomes a set containing it.
static_assert(converts_to<alpha, set_a>);
static_assert(converts_to<alpha, set_ab>);
static_assert(converts_to<beta, set_ab>);

// A non-member does not inject.
static_assert(!converts_to<gamma, set_ab>);

// The empty set is uninhabited: nothing converts into it, and it cannot be
// default-constructed.
static_assert(!converts_to<alpha, empty_set>);
static_assert(!std::is_default_constructible_v<empty_set>);

// Membership is type-level and answers for non-members too.
static_assert(set_ab::contains<alpha>());
static_assert(set_ab::contains<beta>());
static_assert(!set_ab::contains<gamma>());
static_assert(!empty_set::contains<alpha>());

// =========================================================================
// Runtime surface: injection, membership of the held alternative, and
// visitation. This is the whole data-facing API on purpose.
// =========================================================================

TEST_CASE("error_set: injection and the held alternative") {
    set_ab held{alpha{7}};

    REQUIRE(held.holds<alpha>());
    REQUIRE_FALSE(held.holds<beta>());
}

TEST_CASE("error_set: visitation reaches the held alternative") {
    set_ab from_alpha{alpha{3}};
    set_ab from_beta{beta{4}};

    auto tag = [](const auto &error) {
        using held = std::remove_cvref_t<decltype(error)>;
        if constexpr (std::is_same_v<held, alpha>) {
            return error.value * 10;
        } else {
            return error.value * 100;
        }
    };

    REQUIRE(from_alpha.visit(tag) == 30);
    REQUIRE(from_beta.visit(tag) == 400);
}

TEST_CASE("error_set: widening preserves the held alternative") {
    set_a narrow{alpha{5}};
    set_abc wide{narrow};

    REQUIRE(wide.holds<alpha>());
    REQUIRE_FALSE(wide.holds<beta>());
    REQUIRE(wide.visit([](const auto &error) { return error.value; }) == 5);
}

TEST_CASE("error_set: widening is available through a join") {
    using joined = bt::error_set_join_t<set_a, set_b>;
    static_assert(std::is_same_v<joined, set_ab>);

    joined from_left{alpha{1}};
    joined from_right{beta{2}};

    REQUIRE(from_left.holds<alpha>());
    REQUIRE(from_right.holds<beta>());
}

TEST_CASE("error_set: laws hold in a consteval context") {
    // The static_asserts above already ran at compile time; this case makes
    // the coverage visible to ctest rather than silently passing by being a
    // translation unit that merely compiled.
    SUCCEED("normalization, semilattice laws, and conversion fences compiled");
}
