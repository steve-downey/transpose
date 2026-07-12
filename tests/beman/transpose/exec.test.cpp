// tests/beman/transpose/exec.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/exec.hpp>
#include <beman/transpose/transpose.hpp>

#include <catch2/catch_test_macros.hpp>

#if !defined(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION)

TEST_CASE("exec: beman.execution unavailable in this build") {
    SUCCEED("beman.execution not available; real-async test skipped");
}

#else

#include <beman/execution/execution.hpp>

#include <atomic>
#include <stdexcept>
#include <vector>

namespace bt = beman::transpose;
namespace ex = beman::execution;

namespace {
auto times_ten(int x) {
    return ex::just(x) | ex::then([](int v) { return v * 10; });
}
} // namespace

TEST_CASE("exec: transpose gathers vector<sender> into sender<vector>, in order") {
    using S = decltype(times_ten(0));
    std::vector<S> senders;
    for (int i = 1; i <= 3; ++i) {
        senders.push_back(times_ten(i));
    }

    auto gathered = bt::transpose(senders);
    auto [values] = ex::sync_wait(std::move(gathered)).value();

    REQUIRE(values == std::vector<int>{10, 20, 30});
}

TEST_CASE("exec: nothing runs until the gathered sender is run") {
    std::atomic<int> ran{0};
    auto make = [&ran](int x) {
        return ex::just(x) | ex::then([&ran](int v) {
                   ran.fetch_add(1);
                   return v;
               });
    };
    using S = decltype(make(0));
    std::vector<S> senders;
    for (int i = 0; i < 4; ++i) {
        senders.push_back(make(i));
    }

    auto gathered = bt::transpose(senders);
    REQUIRE(ran.load() == 0); // deferred

    auto result = ex::sync_wait(std::move(gathered));
    REQUIRE(result.has_value());
    REQUIRE(ran.load() == 4); // every child ran exactly once
}

TEST_CASE("exec: an error in any child propagates to the gathered sender") {
    auto boom = [](int x) {
        return ex::just(x) | ex::then([](int) -> int {
                   throw std::runtime_error("boom");
               });
    };
    using S = decltype(boom(0));
    std::vector<S> senders;
    senders.push_back(boom(1));
    senders.push_back(boom(2));

    auto gathered = bt::transpose(senders);
    REQUIRE_THROWS_AS(ex::sync_wait(std::move(gathered)), std::runtime_error);
}

#endif
