// tests/beman/transpose/p2300.test.cpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The Applicative surface checked against genuine std::execution senders.
//
// Built only when BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE is ON, because it is
// the one thing in this repository with an external dependency. See
// examples/p2300_adapter.hpp for what this establishes and, just as
// important, what writing it established that it cannot.

#include "p2300_adapter.hpp"

#include <beman/execution/execution.hpp>

#include "test_support.hpp"
#include <beman/transpose/sender.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ex = beman::execution;
namespace bt = beman::transpose;

using beman::transpose::examples::p2300_applicative;

TEST_CASE("p2300: invoke composes real senders into one sender of a tuple") {
    auto composed = p2300_applicative.invoke(
        [](int first, double second, std::string third) {
            return std::tuple{first, second, third};
        },
        ex::just(1), ex::just(2.5), ex::just(std::string{"three"}));

    auto result = ex::sync_wait(std::move(composed));
    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(std::get<0>(values) == 1);
    REQUIRE(std::get<1>(values) == 2.5);
    REQUIRE(std::get<2>(values) == "three");
}

TEST_CASE("p2300: the derived operations work over a context they were not "
          "designed against") {
    // The adapter supplies only pure and invoke. map and zip_with are the
    // library's own derivations on the CRTP base, so this is the base being
    // exercised over a real sender rather than over optional.
    auto mapped = p2300_applicative.map([](int value) { return value * 2; },
                                        ex::just(21));
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(mapped))) == 42);

    auto zipped = p2300_applicative.zip_with(
        [](int left, int right) { return left + right; }, ex::just(3),
        ex::just(4));
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(zipped))) == 7);

    auto lifted = p2300_applicative.lift(11);
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(lifted))) == 11);
}

TEST_CASE("p2300: composition is lazy -- no operand runs until the composed "
          "sender is started") {
    int started = 0;
    auto counting = [&started](int value) {
        ++started;
        return value;
    };

    auto composed = p2300_applicative.invoke(
        [](int left, int right) { return left + right; },
        ex::just(1) | ex::then(counting), ex::just(2) | ex::then(counting));

    REQUIRE(started == 0);
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(composed))) == 3);
    REQUIRE(started == 2);
}

TEST_CASE("p2300: an operand's error completion propagates through the "
          "composition") {
    // The operand is one that CAN complete with a value and fails at run
    // time. just_error(...) cannot be an operand at all: when_all requires
    // each child to have exactly one value completion and just_error has
    // none. That limit is the finding, not a gap in this test.
    auto failing = ex::just(1) | ex::then([](int) -> int {
                       throw std::runtime_error("boom");
                   });

    auto composed = p2300_applicative.invoke(
        [](int left, int right) { return left + right; }, ex::just(1),
        std::move(failing));

    REQUIRE_THROWS_AS(ex::sync_wait(std::move(composed)), std::runtime_error);
}

TEST_CASE("p2300: a move-only value composes") {
    auto composed = p2300_applicative.invoke(
        [](std::unique_ptr<int> held, int addend) { return *held + addend; },
        ex::just(std::make_unique<int>(40)), ex::just(2));

    REQUIRE(std::get<0>(*ex::sync_wait(std::move(composed))) == 42);
}

// =========================================================================
// Stage sender-registration (2026-09-13). The cases above were written when
// this adapter supplied only the pure/invoke basis. Everything below audits
// it against docs/decisions.md#sender-instance-keying and
// docs/decisions.md#sender-value-type-reading, which is what that stage is.
// =========================================================================

namespace {

using beman::transpose::examples::single_value_sender;

using just_int = decltype(ex::just(1));
using then_adapted = decltype(ex::just(1) | ex::then([](int x) { return x; }));

// -- 1. What is and is not an Applicative element. ------------------------
// The concept is `ex::sender` plus exactly one value completion of exactly
// one argument. Each negative below is a sender the adapter must NOT
// register, and each is a case that could plausibly have been an operand.

static_assert(single_value_sender<just_int>);
static_assert(single_value_sender<then_adapted>);
static_assert(single_value_sender<decltype(ex::just(std::string{}))>);
static_assert(
    single_value_sender<decltype(ex::just(std::make_unique<int>(1)))>);

static_assert(!single_value_sender<decltype(ex::just(1, 2))>);
static_assert(!single_value_sender<decltype(ex::just())>);
static_assert(!single_value_sender<decltype(ex::just_error(1))>);
static_assert(!single_value_sender<decltype(ex::just_stopped())>);
static_assert(!single_value_sender<int>);
static_assert(!single_value_sender<std::optional<int>>);

// A RAW when_all IS NOT AN ELEMENT, and that is correct rather than a gap.
// when_all(just(1), just(2)) completes with TWO value arguments, so it is
// not a single-value sender and is not registered. The adapter's own
// invoke composes `when_all(...) | then(f)`, and the `then` is what
// collapses the pack back to one value -- which is why the RESULT of invoke
// is an element even though its middle term is not.
static_assert(
    !single_value_sender<decltype(ex::when_all(ex::just(1), ex::just(2)))>);
static_assert(single_value_sender<
              decltype(ex::when_all(ex::just(1), ex::just(2)) |
                       ex::then([](int a, int b) { return a + b; }))>);

// THE ARITY CHECK IS A CONSTRAINT FAILURE, NOT A DIAGNOSTIC. Every negative
// above evaluates to false rather than hard-erroring, even though naming
// value_types_of_t with a non-variadic Tuple for a two-argument sender is
// ill-formed. Ill-formed inside a requires-expression is a constraint
// failure. This is what lets an unregistered sender reach the framework's
// own "No applicative_typeclass<T>" message instead of a template backtrace
// from inside the constraint.

// -- 2. Registration is by concept, and there is ONE object. --------------

template <class T>
concept applicative_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::applicative_typeclass<T>)>,
                    std::false_type>;

static_assert(applicative_registered<just_int>);
static_assert(applicative_registered<then_adapted>);
static_assert(!applicative_registered<decltype(ex::just(1, 2))>);
static_assert(!applicative_registered<decltype(ex::just_stopped())>);

// One object for ALL sender types, not one per S. Unrelated sender types --
// different value types, different adaptors -- find the SAME object type.
// That is what lets pure(x) return just(x), a different sender type from
// whatever S the object was found under, without the object knowing it.
static_assert(
    std::is_same_v<
        std::remove_const_t<decltype(bt::applicative_typeclass<just_int>)>,
        std::remove_const_t<
            decltype(bt::applicative_typeclass<then_adapted>)>>);
static_assert(
    std::is_same_v<
        std::remove_const_t<decltype(bt::applicative_typeclass<just_int>)>,
        std::remove_const_t<decltype(bt::applicative_typeclass<
                                     decltype(ex::just(std::string{}))>)>>);

// pure DOES return a different type than the S it was found under. Pinned
// because it is the reason the object cannot be templated on S: an object
// keyed per-S would have to promise pure returns S, which is unkeepable.
static_assert(
    !std::is_same_v<decltype(p2300_applicative.pure(1)), then_adapted>);
static_assert(std::is_same_v<decltype(p2300_applicative.pure(1)), just_int>);

// SENTINEL for docs/decisions.md#sender-instance-keying: the demonstration
// sender must not be an ex::sender. If it ever becomes one, this
// concept-keyed registration and the demo's per-type one both match and the
// keying decision requires the ambiguity be resolved by subsumption, never
// by a tie-breaker tag -- so this failing is a design question, not a fix.
static_assert(!ex::sender<bt::sender<int>>);

// -- 3. The element type is read from completion signatures. --------------

static_assert(std::is_same_v<bt::applicative_value_t<just_int>, int>);
static_assert(std::is_same_v<bt::applicative_value_t<then_adapted>, int>);
static_assert(
    std::is_same_v<bt::applicative_value_t<decltype(ex::just(std::string{}))>,
                   std::string>);
static_assert(
    std::is_same_v<
        bt::applicative_value_t<decltype(ex::just(1) | ex::then([](int x) {
                                             return std::to_string(x);
                                         }))>,
        std::string>);

// The reading is DECAYED: a sender completing with a reference reports the
// value type, matching every other carrier in the library.
static_assert(
    std::is_same_v<bt::applicative_value_t<decltype(ex::just(1))>, int>);

// DISJOINTNESS, which the value-reading decision's tripwire guards. No
// sender has a nested value_type, so this adapter's specialization and the
// framework's void_t<typename T::value_type> path never both match. If one
// of these flips, the two partial specializations become ambiguous.
static_assert(!beman::transpose::examples::has_value_type_member<just_int>);
static_assert(!beman::transpose::examples::has_value_type_member<then_adapted>);
static_assert(
    beman::transpose::examples::has_value_type_member<std::optional<int>>);

// -- 4. Deep conformance, structurally. -----------------------------------
// docs/decisions.md#typeclass-conformance-depth: the object must satisfy the
// full Applicative object surface over the carrier, not merely the basis.

static_assert(
    bt::applicative_object<
        std::remove_const_t<decltype(bt::applicative_typeclass<just_int>)>,
        just_int>);
static_assert(
    bt::applicative_object<
        std::remove_const_t<decltype(bt::applicative_typeclass<then_adapted>)>,
        then_adapted>);

/** Observe a sender by running it.
 *
 * The law helpers in test_support.hpp state each equation between two
 * contexts and compare them. Senders have no equality -- two senders that
 * compute the same value are unrelated types -- so the comparison has to
 * happen after running, and sync_wait's optional<tuple<T>> is what compares.
 */
struct observe_by_sync_wait {
    template <class SENDER>
    auto operator()(SENDER &&sender) const {
        return ex::sync_wait(std::forward<SENDER>(sender));
    }
};

} // namespace

TEST_CASE("p2300: the applicative laws hold, observed by running") {
    // The SAME four helpers optional and expected are checked with, with
    // sync_wait substituted for equality. Not a parallel sender-flavoured
    // suite: one statement of each law, so the two cannot drift.
    using bt::test::check_applicative_homomorphism_law;
    using bt::test::check_applicative_identity_law;
    using bt::test::check_applicative_interchange_law;
    using bt::test::check_functor_composition_law;

    const observe_by_sync_wait observe{};

    CHECK(check_applicative_identity_law(ex::just(7), observe));

    CHECK(check_applicative_homomorphism_law<just_int, observe_by_sync_wait>(
        [](int x) { return x * 3; }, 14));
    CHECK(check_applicative_homomorphism_law<just_int, observe_by_sync_wait>(
        [](int left, int right) { return left + right; }, 20, 22));

    CHECK(check_applicative_interchange_law(
        ex::just([](int x) { return x + 1; }), 41, observe));

    CHECK(check_functor_composition_law([](int x) { return x + 1; },
                                        [](int x) { return x * 2; },
                                        ex::just(20), observe));
}

TEST_CASE("p2300: invoke composes at arities 2 and 5") {
    auto two = p2300_applicative.invoke(
        [](int left, int right) { return left * right; }, ex::just(6),
        ex::just(7));
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(two))) == 42);

    auto five = p2300_applicative.invoke(
        [](int a, int b, int c, int d, int e) { return a + b + c + d + e; },
        ex::just(1), ex::just(2), ex::just(3), ex::just(4), ex::just(5));
    REQUIRE(std::get<0>(*ex::sync_wait(std::move(five))) == 15);
}

TEST_CASE("p2300: the result of invoke is a plain sender and keeps composing") {
    // Nothing is wrapped, so the caller keeps composing on the real thing --
    // which is the half of the "no wrapper materialized" claim that is
    // available before all_of exists.
    auto composed = p2300_applicative.invoke(
        [](int left, int right) { return left + right; }, ex::just(1),
        ex::just(2));

    static_assert(ex::sender<decltype(composed)>);
    static_assert(single_value_sender<decltype(composed)>);
    static_assert(
        std::is_same_v<bt::applicative_value_t<decltype(composed)>, int>);

    auto further = std::move(composed) |
                   ex::then([](int x) { return x * 10; }) |
                   ex::then([](int x) { return x + 4; });

    REQUIRE(std::get<0>(*ex::sync_wait(std::move(further))) == 34);
}

TEST_CASE("p2300: a move-only payload survives registration and the basis") {
    // operand-value-category: operands are forwarded, not taken const&. A
    // move-only payload is what tells a forwarding implementation from one
    // that merely looks like it.
    static_assert(!std::is_copy_constructible_v<decltype(ex::just(
                      std::make_unique<int>(1)))>);
    static_assert(
        single_value_sender<decltype(ex::just(std::make_unique<int>(1)))>);
    static_assert(std::is_same_v<bt::applicative_value_t<decltype(ex::just(
                                     std::make_unique<int>(1)))>,
                                 std::unique_ptr<int>>);

    auto composed = p2300_applicative.invoke(
        [](std::unique_ptr<int> held) { return *held + 1; },
        ex::just(std::make_unique<int>(41)));

    REQUIRE(std::get<0>(*ex::sync_wait(std::move(composed))) == 42);
}

// -- 4b. What pure returns, and what the policy concept does about it. ----
//
// applicative_object (above) does not constrain what pure RETURNS. traverse's
// policy concept, applicative_object_for, is where that question is asked.
//
// THE MEASUREMENT THIS SECTION EXISTS FOR IS UNCHANGED. pure(x) is always
// just(x), so "pure returns exactly CONTEXT" is satisfiable only by accident:
// it holds when CONTEXT happens to BE decltype(just(x)) and fails for every
// adapted sender, which is what a caller actually has. That is finding (b) --
// one object for all sender types, pure free to return a type of its own
// choosing -- showing up as a consequence rather than a virtue, and it is
// still true. Recorded at docs/review/execution-sender-registration.md
// section 4.
//
// WHAT CHANGED IS THE CONSEQUENCE, NOT THE FACT. Steve's ruling of 2026-09-19
// (docs/decisions.md#collect-hook part 2) disjoined applicative_object_for on
// `collect`: a policy that composes the whole structure in one operation is
// not bound by what pure returns, because the pairwise composition that
// constraint describes is one it will not perform. So all three shapes now
// satisfy the policy concept -- by the second alternative, not the first.
// Both halves are pinned, so a later reading can tell which one is carrying
// each row.

namespace {
template <class S>
using object_for_ = std::remove_const_t<decltype(bt::applicative_typeclass<S>)>;

using whenall_then = decltype(ex::when_all(ex::just(1), ex::just(2)) |
                              ex::then([](int a, int b) { return a + b; }));

// The refinement's FIRST alternative, spelled here so that the fact and the
// consequence can be asserted apart from each other.
template <class OBJ, class CONTEXT>
concept pure_returns_context = requires(const OBJ &object) {
    {
        object.pure(std::declval<bt::applicative_value_t<CONTEXT>>())
    } -> std::same_as<CONTEXT>;
};

// The deep object concept holds for every registered sender shape.
static_assert(bt::applicative_object<object_for_<just_int>, just_int>);
static_assert(bt::applicative_object<object_for_<then_adapted>, then_adapted>);
static_assert(bt::applicative_object<object_for_<whenall_then>, whenall_then>);

// The first alternative holds for exactly one of them -- the fact, unchanged.
static_assert(pure_returns_context<object_for_<just_int>, just_int>);
static_assert(!pure_returns_context<object_for_<then_adapted>, then_adapted>);
static_assert(!pure_returns_context<object_for_<whenall_then>, whenall_then>);

// The policy concept holds for all three, the other two by way of `collect`.
static_assert(bt::applicative_object_for<object_for_<just_int>, just_int>);
static_assert(
    bt::applicative_object_for<object_for_<then_adapted>, then_adapted>);
static_assert(
    bt::applicative_object_for<object_for_<whenall_then>, whenall_then>);
} // namespace

// -- 5. What this stage deliberately did NOT make work. -------------------
//
// `transpose(std::vector<S>)` over a real sender is stage collect-hook's
// deliverable, and Stage 1's acceptance forbids reaching it early. It does
// not work, which is correct.
//
// THERE IS NO NEGATIVE static_assert FOR IT, and the reason is worth
// knowing: probing it IS the failure. `bt::transpose` has a deduced return
// type, so a requires-expression naming it instantiates the body, and the
// body dies inside sequence.hpp at `accumulated = applicative.invoke(...)`
// -- "no viable overloaded '='" -- which is a hard error, not something a
// concept can evaluate to false. Measured; see the 2026-09-13 entry under
// docs/decisions.md#sender-instance-keying.
//
// That assignment is exactly the invariance the left fold needs and a real
// sender does not have, so the message is honest. It is also worse than
// what an UNREGISTERED sender gets, which is a clean "no matching function
// for call to 'transpose'" at the call site. Registering the applicative
// bought that regression. Stage collect-hook is where it is paid back, and
// "this message becomes clean, or becomes a success" is a reasonable
// acceptance signal for that stage.
