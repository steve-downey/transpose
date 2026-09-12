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

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace ex = beman::execution;

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
    auto mapped =
        p2300_applicative.map([](int value) { return value * 2; }, ex::just(21));
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
