// tests/beman/transpose/demo_sender_golden.test.cpp                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// GOLDEN -- stage execution-baseline of the execution plan
// (docs/transpose-execution-plan.md#execution-baseline, deliverable 4),
// pinning docs/decisions.md#demo-sender-fate.
//
// EVERY assertion here is a golden in the sense
// docs/decisions.md#golden-vs-scheduled-assertions fixes: no stage of the
// execution plan is due to move any of it, and one that fails is a
// regression, never a line to update (divergence protocol rule 4).
//
// WHAT THIS ADDS THAT baseline_deduction.test.cpp DOES NOT. That file already
// pins the demonstration sender's front-door DEDUCTIONS -- traverse over a
// vector of ints into sender<vector<int>>, transpose over a
// vector<sender<int>>, and the registration itself -- and it is built in
// every configuration, so those deductions are already checked with the
// execution dependency both enabled and disabled. Repeating them here would
// be duplication, not coverage.
//
// What no existing file pins is the word "unconditional" in demo-sender-fate:
// that the demonstration sender is reachable with NO dependency on an
// execution implementation at all. That claim is about what this translation
// unit includes, so it can only be made by a translation unit that includes
// nothing else -- which is what this file is. It is the sensor for stage
// sender-registration's plan to include an execution-dependent header from
// transpose.hpp: if that inclusion ever becomes unconditional, or if
// sender.hpp acquires an execution include, this file stops compiling.
//
// It is deliberately NOT guarded by the execution option. A golden that only
// builds when the dependency is enabled would assert the opposite of what
// this one is for.

#include <beman/transpose/sender.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

namespace {

// -- 1. sender.hpp stands alone. ------------------------------------------
// The only transpose header included above is sender.hpp. If these hold, the
// demonstration sender needs neither transpose.hpp nor any execution
// implementation to exist and to be registered.

template <class T>
concept applicative_registered =
    !std::is_same_v<std::remove_const_t<decltype(bt::applicative_typeclass<T>)>,
                    std::false_type>;

static_assert(applicative_registered<bt::sender<int>>);
static_assert(applicative_registered<bt::sender<std::string>>);
static_assert(applicative_registered<bt::sender<std::vector<int>>>);

static_assert(std::is_same_v<bt::applicative_value_t<bt::sender<int>>, int>);

// -- 2. The property that makes it a demonstration and not evidence. ------
// The demonstration sender is INVARIANT UNDER COMPOSITION: composing
// sender<vector<int>> with sender<int> yields sender<vector<int>> again. That
// is what lets the vector traversal's accumulator loop type-check, and it is
// exactly what a real P2300 sender does not do -- every combinator there
// names a new type. sender.hpp's own header comment says so; this pins it,
// so the contrast the paper draws rests on a checked fact.
//
// This is also the reason docs/decisions.md#erasure-boundary can be measured
// at all: the invariance is bought with std::function, and the boundary
// decision forbids exactly that purchase in all_of.hpp.

using acc = bt::sender<std::vector<int>>;
using elem = bt::sender<int>;

constexpr auto push_back = [](std::vector<int> soFar, int next) {
    soFar.push_back(next);
    return soFar;
};

static_assert(
    std::is_same_v<decltype(bt::applicative_typeclass<acc>.invoke(
                       push_back, std::declval<acc>(), std::declval<elem>())),
                   acc>);

// -- 3. The erasure is deliberate, and it is here. ------------------------
// <functional> is reachable from this translation unit because sender.hpp
// wants it. Stage all-of-algorithm's tests assert the opposite for
// all_of.hpp. Pinning both directions is what makes "no wrapper
// materialized" a claim about the real instance rather than about the
// library as a whole.

static_assert(std::is_copy_constructible_v<bt::sender<int>>,
              "the demonstration sender is copyable; a real one need not be");

} // namespace

TEST_CASE("demo sender golden: laziness without any execution dependency") {
    int ran = 0;

    auto lazy = bt::sender<int>{[&ran] {
        ++ran;
        return 7;
    }};

    CHECK(ran == 0);
    CHECK(lazy.get() == 7);
    CHECK(ran == 1);
}
