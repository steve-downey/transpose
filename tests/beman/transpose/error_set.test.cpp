// tests/beman/transpose/error_set.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/error_set.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <type_traits>

namespace bt = beman::transpose;

// Named types with external linkage and a stable spelling -- the documented
// restrictions of the interim P2830 fallback. NOT in an anonymous namespace,
// deliberately: these probes must satisfy the restriction they are testing.
namespace beman_transpose_error_probes {

struct alpha {
    int value{};

    friend auto operator==(const alpha &, const alpha &) -> bool = default;
};
struct beta {
    int value{};

    friend auto operator==(const beta &, const beta &) -> bool = default;
};
struct gamma {
    int value{};

    friend auto operator==(const gamma &, const gamma &) -> bool = default;
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

// =========================================================================
// Value-level amendment (docs/decisions.md#accumulation-evidence): a value
// is a non-empty WITNESSED SUBSET, not a one-of union. The tests above
// already cover the singleton case -- every one of them holds exactly one
// witness, which is the short-circuit special case of the general
// invariant. These tests exercise the general case: more than one witness
// present at once, and the left-biased combine that produces it.
// =========================================================================

TEST_CASE("error_set: combining distinct types unions their witnesses") {
    set_ab from_alpha{alpha{1}};
    set_ab from_beta{beta{2}};

    auto combined = bt::error_set_combine(from_alpha, from_beta);

    REQUIRE(combined.holds<alpha>());
    REQUIRE(combined.holds<beta>());
    REQUIRE(combined.witness<alpha>() == std::optional{alpha{1}});
    REQUIRE(combined.witness<beta>() == std::optional{beta{2}});
}

TEST_CASE("error_set: combining the same type keeps the LEFT witness") {
    set_a left{alpha{10}};
    set_a right{alpha{20}};

    auto combined = bt::error_set_combine(left, right);

    REQUIRE(combined.holds<alpha>());
    REQUIRE(combined.witness<alpha>() == std::optional{alpha{10}});

    // Combine is not commutative on the WITNESS (only the type-level join
    // is): swapping the operands changes which witness survives.
    auto swapped = bt::error_set_combine(right, left);
    REQUIRE(swapped.witness<alpha>() == std::optional{alpha{20}});
}

TEST_CASE("error_set: combining is left-biased per type, not wholesale") {
    // Left holds alpha only, right holds both alpha and beta. The combined
    // result keeps left's alpha (left-biased) but still picks up right's
    // beta, since left never witnessed beta at all.
    set_ab left{alpha{100}};
    set_ab right{beta{200}};
    // Give `right` a beta witness and, via a second combine, an alpha
    // witness too, to show a per-type -- not per-value -- decision.
    auto right_both = bt::error_set_combine(right, set_ab{alpha{999}});
    REQUIRE(right_both.holds<alpha>());
    REQUIRE(right_both.holds<beta>());

    auto combined = bt::error_set_combine(left, right_both);
    REQUIRE(combined.witness<alpha>() == std::optional{alpha{100}}); // left's
    REQUIRE(combined.witness<beta>() == std::optional{beta{200}});   // right's
}

TEST_CASE("error_set: equality is same present set, equal witnesses") {
    set_ab both_a_b{alpha{1}};
    auto with_beta_too = bt::error_set_combine(both_a_b, set_ab{beta{2}});

    // Different present set: unequal even though alpha's witness agrees.
    REQUIRE_FALSE(both_a_b == with_beta_too);

    // Same present set, same witness values: equal.
    REQUIRE(with_beta_too ==
            bt::error_set_combine(set_ab{alpha{1}}, set_ab{beta{2}}));

    // Same present set, DIFFERENT witness value: unequal. This is the
    // "equal witnesses" half of the amendment -- presence alone is not
    // enough.
    auto different_witness =
        bt::error_set_combine(set_ab{alpha{1}}, set_ab{beta{99}});
    REQUIRE_FALSE(with_beta_too == different_witness);
}

TEST_CASE("error_set: combine is associative, allocation-free evidence") {
    // Associativity is what makes combine well-defined as a fold over any
    // number of failures regardless of grouping, exactly like the type-level
    // join it sits beside.
    set_abc a{alpha{1}};
    set_abc b{beta{2}};
    set_abc c{gamma{3}};

    auto left_first = bt::error_set_combine(bt::error_set_combine(a, b), c);
    auto right_first = bt::error_set_combine(a, bt::error_set_combine(b, c));

    REQUIRE(left_first == right_first);
    REQUIRE(left_first.holds<alpha>());
    REQUIRE(left_first.holds<beta>());
    REQUIRE(left_first.holds<gamma>());
}

// =========================================================================
// visit's precondition is checked, not merely documented.
//
// Multi-witness values and visitation shipped one stage apart
// (accumulating-object and error-set-type respectively), and the seam
// between them is where a silent wrong answer would have lived: visit
// picking the leftmost witness and dropping the rest would surface as a
// lost error somewhere else entirely, much later.
// =========================================================================

/** visit still works in a CONSTANT EXPRESSION when the precondition holds.
 * This is the assertion that matters after the hardening: the check added to
 * to_variant() calls a non-constexpr function, and if that call were reachable
 * on the valid path it would have quietly cost every constexpr use of visit. */
consteval auto visit_a_singleton_at_compile_time() -> int {
    const set_ab held{alpha{7}};
    return held.visit([](const auto &error) { return error.value; });
}

static_assert(visit_a_singleton_at_compile_time() == 7);

// witness_count lets a caller check the precondition instead of tripping it.
consteval auto count_after_combining() -> std::size_t {
    const set_ab left{alpha{1}};
    const set_ab right{beta{2}};
    return left.combined_with(right).witness_count();
}

static_assert(count_after_combining() == 2);

consteval auto count_of_a_singleton() -> std::size_t {
    return set_ab{alpha{1}}.witness_count();
}

static_assert(count_of_a_singleton() == 1);

// Left-bias keeps the count at one when both sides witness the SAME type.
consteval auto count_after_combining_same_type() -> std::size_t {
    const set_ab left{alpha{1}};
    const set_ab right{alpha{2}};
    return left.combined_with(right).witness_count();
}

static_assert(count_after_combining_same_type() == 1);

TEST_CASE("error_set: witness_count reports how many alternatives did raise") {
    set_ab single{alpha{1}};
    REQUIRE(single.witness_count() == 1);

    auto combined = single.combined_with(set_ab{beta{2}});
    REQUIRE(combined.witness_count() == 2);
    REQUIRE(combined.holds<alpha>());
    REQUIRE(combined.holds<beta>());

    // A caller checks rather than trips: this is the guarded idiom the
    // precondition expects of anyone holding a possibly-accumulated value.
    if (combined.witness_count() == 1) {
        FAIL("combined value should witness two alternatives");
    }
    REQUIRE(combined.witness<alpha>().has_value());
    REQUIRE(combined.witness<beta>().has_value());
}
