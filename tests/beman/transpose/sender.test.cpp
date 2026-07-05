// tests/beman/transpose/sender.test.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/sender.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>

namespace bt = beman::transpose;

TEST_CASE("sender: ready value runs on get") {
    auto s = bt::sender<int>::ready(42);
    REQUIRE(s.get() == 42);
}

TEST_CASE("sender: applicative defers work until get") {
    auto runs = std::make_shared<int>(0);
    const auto &app = bt::applicative_typeclass<bt::sender<int>>;

    auto make = [runs](int v) {
        return bt::sender<int>{[runs, v] {
            ++(*runs);
            return v;
        }};
    };

    auto combined =
        app.invoke([](int a, int b) { return a + b; }, make(3), make(4));
    REQUIRE(*runs == 0);
    REQUIRE(combined.get() == 7);
    REQUIRE(*runs == 2);
}
