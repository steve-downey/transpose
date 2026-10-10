// tests/beman/transpose/dual_monoid.test.cpp                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/monoid.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace bt = beman::transpose;

// dual_monoid<M> is an instance over M's own value type: the string stays a
// string, only the combine order flips. Nothing is wrapped.

TEST_CASE("dual_monoid: flips combine argument order") {
    bt::dual_monoid<bt::Monoid<std::string>> d;
    auto combined = d.combine(std::string{"ab"}, std::string{"cd"});
    // Flipped: rhs + lhs == "cd" + "ab".
    REQUIRE(combined == "cdab");
}

TEST_CASE("dual_monoid: identity matches underlying identity") {
    bt::dual_monoid<bt::Monoid<std::string>> d;
    REQUIRE(d.identity().empty());
}

TEST_CASE("dual_monoid: the dual of the dual is the original order") {
    bt::dual_monoid<bt::dual_monoid<bt::Monoid<std::string>>> dd;
    REQUIRE(dd.combine(std::string{"ab"}, std::string{"cd"}) == "abcd");
}
