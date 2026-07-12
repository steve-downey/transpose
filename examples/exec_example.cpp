// examples/exec_example.cpp                                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Paper A's deferred domain, for real, on beman.execution (a std::execution /
// P2300 implementation). The same `transpose` front door flips a structure of
// senders into one sender of the structure:
//   vector<sender<T>>  ->  sender<vector<T>>
// No type erasure: a std::vector is homogeneous, so the element senders share
// one concrete type and the result is one concrete sender type. Nothing runs
// until the gathered sender is run.

#include <beman/transpose/exec.hpp>
#include <beman/transpose/transpose.hpp>

#if !defined(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION)
#include <iostream>
int main() {
    std::cout << "beman.execution not available; skipping.\n";
    return 0;
}
#else

#include <beman/execution/execution.hpp>

#include <atomic>
#include <iostream>
#include <vector>

namespace bt = beman::transpose;
namespace ex = beman::execution;

int main() {
    std::atomic<int> ran{0};
    auto make = [&ran](int x) {
        return ex::just(x) | ex::then([&ran](int v) {
                   ran.fetch_add(1);
                   return v * 10;
               });
    };
    using S = decltype(make(0));

    std::vector<S> senders;
    for (int i = 1; i <= 3; ++i) {
        senders.push_back(make(i));
    }

    auto gathered = bt::transpose(senders); // sender<vector<int>>
    std::cout << "before run: " << ran.load() << " children executed\n";

    auto [values] = ex::sync_wait(std::move(gathered)).value(); // now it runs
    std::cout << "after run:  " << ran.load() << " children executed\n";
    std::cout << "result:";
    for (int v : values) {
        std::cout << ' ' << v;
    }
    std::cout << '\n';
    return 0;
}

#endif
