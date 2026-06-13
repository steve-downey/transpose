// transpose/transpose.test.cpp                                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <smd/transpose/transpose.hpp>

#include <smd/transpose/transpose.hpp> // test 2nd include OK

#include <catch2/catch_test_macros.hpp>

// 03013d1f-bcc1-4d3e-9701-3ed1a15c6370
TEST_CASE("transpose returns Steve", "transpose") {
    REQUIRE(transpose::transpose() == "Steve");
}
// 03013d1f-bcc1-4d3e-9701-3ed1a15c6370 end
