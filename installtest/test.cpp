// testinstall/test.cpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <smd/transpose/transpose.hpp>

int main() {
    std::cout << "transpose: |" << transpose::transpose() << '|' << '\n';
    return 0;
}
