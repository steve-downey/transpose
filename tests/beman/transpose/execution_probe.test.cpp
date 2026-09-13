// tests/beman/transpose/execution_probe.test.cpp                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// DEPENDENCY COMPILE PROBE -- stage execution-baseline of the execution plan
// (docs/transpose-execution-plan.md#execution-baseline, deliverable 3).
//
// WHY THIS EXISTS, AND WHY IT IS NOT p2300.test.cpp. This file includes
// beman.execution and NOTHING from this library. It is the canary for the
// dependency itself: if the pinned beman.execution stops compiling, stops
// exporting the namespace this repository expects, or stops supplying one of
// the four names every later stage of the execution plan is built on, this
// probe fails on its own, before any transpose-side diagnostic can confuse
// the question with an adapter defect.
//
// p2300.test.cpp is the opposite test: it exercises the library's Applicative
// surface OVER the dependency, so a failure there is normally ours. Keeping
// the two apart is what makes "is it them or is it us" answerable by reading
// which executable went red.
//
// The four names probed here -- just, when_all, then, sync_wait -- are not an
// arbitrary sample. They are exactly the basis
// docs/decisions.md#sender-instance-keying registers an Applicative object
// on: pure is just, invoke is when_all + then, and sync_wait is the
// observation every law check in the plan uses. A fifth name, an operation
// state owning n children, is what stage all-of-algorithm must write because
// the dependency does NOT ship it; see
// docs/review/prior-art-when-all-range.md.
//
// Built only when the execution dependency is enabled. The option's spelling
// is the repository's existing one, not the plan's proposed
// BEMAN_TRANSPOSE_WITH_EXECUTION; see the divergence logged at
// docs/decisions.md#execution-dependency-shape.

#include <beman/execution/execution.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ex = beman::execution;

namespace {

// The namespace this repository expects is the one the dependency exports.
// beman.execution also still ships a legacy beman::execution26 tree; naming
// the modern spelling here is what makes a silent move back to the old one a
// build failure rather than a discovery three stages later.
constexpr auto probe_sender = [] { return ex::just(1); };

static_assert(ex::sender<decltype(probe_sender())>);

} // namespace

TEST_CASE("execution probe: the pinned dependency composes and runs") {
    auto composed =
        ex::when_all(ex::just(1), ex::just(2)) |
        ex::then([](int first, int second) { return first + second; });

    // Laziness: the whole point of the dependency. Nothing above ran.
    auto result = ex::sync_wait(std::move(composed));

    REQUIRE(result.has_value());

    const auto &[value] = *result;
    CHECK(value == 3);
}

TEST_CASE("execution probe: a value completion carries through then") {
    auto result =
        ex::sync_wait(ex::just(20) | ex::then([](int x) { return x * 2; }));

    REQUIRE(result.has_value());

    const auto &[value] = *result;
    CHECK(value == 40);
}

// The single-value-completion shape the plan's registration constraint reads
// from (docs/decisions.md#sender-value-type-reading). Pinned here as a
// property of the DEPENDENCY, so that stage sender-registration can tell a
// changed completion-signature spelling from its own constraint being wrong.
//
// THE EMPTY ENVIRONMENT IS SPELLED env<>, NOT empty_env. The decision text
// says empty_env, which is what P2300 called it before LWG replaced the
// dedicated type with the zero-argument case of the env<...> template; the
// pinned beman.execution exports only the newer spelling. Logged as a What
// divergence at docs/decisions.md#sender-value-type-reading. It is also the
// DEFAULT for this alias's second parameter, so the shortest correct
// spelling omits it entirely -- but naming it is what makes this assertion a
// sensor for the dependency changing it back.
namespace {

using probe_value_types =
    ex::value_types_of_t<decltype(ex::just(1)), ex::env<>, std::type_identity_t,
                         std::type_identity_t>;

static_assert(std::is_same_v<probe_value_types, int>);

// A two-argument value completion collapses under the same reading, which is
// precisely why sender-value-type-reading must constrain on arity rather
// than trust the alias: type_identity_t is not variadic, so this would be
// ill-formed rather than merely wrong. Pinned as a NEGATIVE: the arity check
// stage sender-registration writes has to do real work.
template <class S>
concept reads_as_single_value = requires {
    typename ex::value_types_of_t<S, ex::env<>, std::type_identity_t,
                                  std::type_identity_t>;
};

static_assert(reads_as_single_value<decltype(ex::just(1))>);
static_assert(!reads_as_single_value<decltype(ex::just(1, 2))>);

} // namespace
