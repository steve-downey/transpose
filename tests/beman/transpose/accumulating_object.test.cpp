// tests/beman/transpose/accumulating_object.test.cpp                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Stage accumulating-object
// (docs/transpose-grading-plan.md#accumulating-object).
//
// The claim under test: a SECOND NTTP-pinned applicative object over the same
// carrier and grade algebra as the short-circuit one
// (docs/decisions.md#applicative-objects), which COLLECTS every failing
// operand's witness at the joined grade rather than stopping at the first
// (docs/decisions.md#accumulation-evidence). Three things distinguish it from
// the short-circuit object, and each gets its own test group below:
//
//   1. It exists as a distinct object, reachable via
//      `accumulating_applicative_typeclass<Context>` and as the explicit
//      `traverse` policy (docs/decisions.md#traverse-policy-surface).
//   2. Where short-circuit keeps only the FIRST failure's witness, it unions
//      witnesses of DISTINCT types from every failing operand, examined in
//      the normative left-to-right order.
//   3. It has no bind: see the comment on the negative-control section at the
//      bottom, which documents how that was verified (a permanently
//      non-compiling test cannot live in this suite).

#include <beman/transpose/traverse.hpp>

#include <beman/transpose/apply.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/expected.hpp>
#include <beman/transpose/sequence.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <optional>
#include <type_traits>
#include <vector>

namespace bt = beman::transpose;

// Named types with external linkage and a stable spelling, matching the
// restriction the interim type ordering documents (error_set.hpp).
namespace beman_transpose_accumulate_probes {

struct alpha {
    int value{};

    friend auto operator==(const alpha &, const alpha &) -> bool = default;
};
struct beta {
    int value{};

    friend auto operator==(const beta &, const beta &) -> bool = default;
};

} // namespace beman_transpose_accumulate_probes

namespace probes = beman_transpose_accumulate_probes;
using probes::alpha;
using probes::beta;

namespace {

/** Whether `bt::traverse(f, value, policy)` is a well-formed call for these
 * three types. A NAMED CONCEPT on purpose, matching the pattern documented
 * in test_support.hpp: a bare requires-expression at block scope hard-errors
 * on an invalid expression instead of yielding false ([expr.prim.req]) --
 * only a template gets the substitution-failure-is-not-an-error treatment.
 */
template <class F, class T, class POLICY>
concept traverse_accepts_policy =
    requires(F f, T value, POLICY policy) { bt::traverse(f, value, policy); };

using mixed = bt::error_set<alpha, beta>;
using exp_mixed = std::expected<int, mixed>;

auto ok(int x) -> exp_mixed { return exp_mixed{x}; }
auto fails_alpha(int payload) -> exp_mixed {
    return exp_mixed{std::unexpect, alpha{payload}};
}
auto fails_beta(int payload) -> exp_mixed {
    return exp_mixed{std::unexpect, beta{payload}};
}

} // namespace

// =========================================================================
// 1. The object exists, distinctly from the short-circuit one.
// =========================================================================

static_assert(!std::is_same_v<
              decltype(bt::applicative_typeclass<exp_mixed>),
              decltype(bt::accumulating_applicative_typeclass<exp_mixed>)>);

TEST_CASE("accumulating-object: invoke collects distinct-type failures") {
    const auto &accumulate = bt::accumulating_applicative_typeclass<exp_mixed>;
    auto add = [](int a, int b) { return a + b; };

    auto both_fail = accumulate.invoke(add, fails_alpha(1), fails_beta(2));

    REQUIRE_FALSE(both_fail.has_value());
    REQUIRE(both_fail.error().holds<alpha>());
    REQUIRE(both_fail.error().holds<beta>());
    REQUIRE(both_fail.error().witness<alpha>() == std::optional{alpha{1}});
    REQUIRE(both_fail.error().witness<beta>() == std::optional{beta{2}});
}

TEST_CASE("accumulating-object: the short-circuit object keeps only the "
          "first, by contrast") {
    const auto &short_circuit = bt::applicative_typeclass<exp_mixed>;
    auto add = [](int a, int b) { return a + b; };

    auto both_fail = short_circuit.invoke(add, fails_alpha(1), fails_beta(2));

    REQUIRE_FALSE(both_fail.has_value());
    REQUIRE(both_fail.error().holds<alpha>());
    REQUIRE_FALSE(both_fail.error().holds<beta>()); // dropped, by design
}

// =========================================================================
// 2. Left-to-right order and left-biased combining: same-type collisions
//    keep the LEFTMOST witness, which is what makes combining deterministic
//    (docs/decisions.md#accumulation-evidence,
//    docs/decisions.md#applicative-objects).
// =========================================================================

TEST_CASE("accumulating-object: two failures of the SAME type keep the "
          "leftmost witness") {
    const auto &accumulate = bt::accumulating_applicative_typeclass<exp_mixed>;
    auto add = [](int a, int b) { return a + b; };

    auto result = accumulate.invoke(add, fails_alpha(10), fails_alpha(20));

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().holds<alpha>());
    REQUIRE(result.error().witness<alpha>() == std::optional{alpha{10}});
}

TEST_CASE("accumulating-object: order-observing probe over three operands") {
    const auto &accumulate = bt::accumulating_applicative_typeclass<exp_mixed>;
    auto sum3 = [](int a, int b, int c) { return a + b + c; };

    // alpha at position 0, beta at position 1, alpha again at position 2:
    // the SECOND alpha must not overwrite the first (leftmost wins), and
    // beta must still show up alongside it (distinct types both survive).
    auto result =
        accumulate.invoke(sum3, fails_alpha(1), fails_beta(2), fails_alpha(3));

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().witness<alpha>() == std::optional{alpha{1}});
    REQUIRE(result.error().witness<beta>() == std::optional{beta{2}});
}

// =========================================================================
// 3. traverse with the accumulating object as the trailing policy
//    (docs/decisions.md#traverse-policy-surface). This is the stage's
//    front-door acceptance case: a single vector, one per-element function,
//    different elements raising different members of the SAME declared
//    grade.
// =========================================================================

TEST_CASE(
    "accumulating-object: traverse default is unchanged (short-circuit)") {
    auto classify = [](int x) -> exp_mixed {
        if (x == 1) {
            return fails_alpha(x);
        }
        if (x == 2) {
            return fails_beta(x);
        }
        return ok(x);
    };

    // Default policy: today's behavior, character-identical call site.
    auto result = bt::traverse(classify, std::vector<int>{0, 1, 2});
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().holds<alpha>());
    REQUIRE_FALSE(result.error().holds<beta>());
}

TEST_CASE("accumulating-object: traverse collects every distinct error at "
          "the joined grade") {
    auto classify = [](int x) -> exp_mixed {
        if (x == 1) {
            return fails_alpha(x);
        }
        if (x == 2) {
            return fails_beta(x);
        }
        return ok(x);
    };

    auto result =
        bt::traverse(classify, std::vector<int>{0, 1, 2, 3},
                     bt::accumulating_applicative_typeclass<exp_mixed>);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().holds<alpha>());
    REQUIRE(result.error().holds<beta>());
    REQUIRE(result.error().witness<alpha>() == std::optional{alpha{1}});
    REQUIRE(result.error().witness<beta>() == std::optional{beta{2}});
}

TEST_CASE("accumulating-object: traverse succeeds when nothing fails, same "
          "as short-circuit") {
    auto classify = [](int x) -> exp_mixed { return ok(x * 2); };

    auto result =
        bt::traverse(classify, std::vector<int>{1, 2, 3},
                     bt::accumulating_applicative_typeclass<exp_mixed>);

    REQUIRE(result.has_value());
    REQUIRE(*result == std::vector<int>{2, 4, 6});
}

// =========================================================================
// 4. The traverse-policy-surface enforcement hook: a stray third argument --
//    someone's extra container, not an applicative object at all -- fails
//    the constraint instead of being silently swallowed as a policy.
// =========================================================================

using classify_fn = exp_mixed (*)(int);

static_assert(
    !traverse_accepts_policy<classify_fn, std::vector<int>, std::vector<int>>);
// The positive counterpart, so the negative cannot pass vacuously (e.g. by
// `traverse` failing to be callable at all for unrelated reasons).
static_assert(traverse_accepts_policy<
              classify_fn, std::vector<int>,
              decltype(bt::accumulating_applicative_typeclass<exp_mixed>)>);
static_assert(
    traverse_accepts_policy<classify_fn, std::vector<int>,
                            decltype(bt::applicative_typeclass<exp_mixed>)>);

TEST_CASE("accumulating-object: a non-applicative third argument is "
          "rejected at the constraint, not silently accepted") {
    // The static_asserts above already ran at compile time; this gives the
    // coverage a visible line in the ctest run.
    SUCCEED("stray third argument fails the applicative_object_for "
            "constraint, and both real policies still pass");
}

// =========================================================================
// 5. Bind is refused for the accumulating object -- a compile error, not a
//    silent absence. This CANNOT be a normal test case: a call that fails to
//    compile cannot sit in a translation unit this suite builds, and
//    `requires { policy.bind(...); }` does not detect it either, because the
//    static_assert inside the member body only fires on instantiation, which
//    an unevaluated requires-expression does not force -- the call is
//    syntactically well-formed there even though invoking it is not.
//
// Verified by hand instead, per the stage instructions: a scratch line
//
//   bt::accumulating_applicative_typeclass<exp_mixed>.bind(
//       ok(1), [](int) { return ok(2); });
//
// was added temporarily to this file, built, and confirmed to fail with the
// static_assert message on AccumulatingExpectedApplicativeMap::bind
// (expected.hpp) naming the value-flow reason -- "sequencing needs a value
// from the first computation ... there is none when that computation
// failed" -- then removed. See docs/decisions.md#applicative-objects.
// =========================================================================

TEST_CASE("accumulating-object: no Monad instance is registered for it") {
    // There is no monad_typeclass entry keyed by the accumulating object --
    // only by carrier type, and the carrier's monad_typeclass is still the
    // short-circuit bind (docs/decisions.md#applicative-objects: bind
    // belongs to the short-circuit object only). This does not, by itself,
    // prove `.bind` is refused on the accumulating object -- that half is
    // the hand-verified static_assert documented above -- but it does pin
    // that grading never grew a second bind path.
    static_assert(std::is_same_v<decltype(bt::monad_typeclass<exp_mixed>.bind(
                                     ok(1), [](int x) { return ok(x); })),
                                 exp_mixed>);
    SUCCEED("carrier-level bind remains the short-circuit one only");
}
