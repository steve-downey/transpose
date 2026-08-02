// examples/operation_objects_example.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// A fourth lookup mode: the typeclass operation as an object, in namespace
// beman::transpose::typeclass, that does its own lookup.
//
// examples/lookup_modes_example.cpp shows the three modes the typeclass-object
// pattern has always had -- implicit lookup through the variable template, an
// explicit instance passed as an argument, and NTTP pinning. All three name
// the instance. This file shows the mode that does not: `typeclass::fmap(f,
// tree)` names the operation and lets the call site deduce the rest.
//
// Two situations motivate it, and both appear below:
//
//   1. Everything-local code. A container on the stack, one function that
//      folds and traverses it. Naming a lookup object per typeclass costs a
//      line each and buys nothing, because nothing else in the function
//      refers to the instance.
//
//   2. A non-template caller. A plain function over concrete types has no
//      template parameter list, so there is nowhere to hang a `const auto &`
//      NTTP. Before `typeclass`, such a function either opened with a block of
//      lookup-object declarations or became a template for no reason of its
//      own.
//
// What the object form costs is mode 3: an operation object takes no explicit
// template arguments, and `typeclass::fmap<..., my_instance>(f, t)` does not
// parse. Pinning falls back to mode 2, which was always shorter anyway -- see
// the last section.

#include "binary_tree.hpp"

#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>

#include <iostream>
#include <optional>
#include <vector>

namespace bt = beman::transpose;
namespace tc = beman::transpose::typeclass;
using example::BinaryTree;

namespace {

// -- Case 1: everything is local ------------------------------------------
//
// The same computation twice, over a tree living on the stack. Neither
// version is wrong; the second one is what the operations look like when the
// instances are not otherwise interesting.

// 7f7a0dee-cb47-47e1-a113-b1fab3604e90
/** Summarize a tree through the lookup objects (modes 1 and 2). */
template <class T>
auto summarize_through_lookups(const BinaryTree<T> &tree) -> std::size_t {
    const auto &foldable = bt::foldable_typeclass<BinaryTree<T>>;
    const auto &traversable = bt::traversable_typeclass<BinaryTree<T>>;

    auto checked = traversable.for_each(tree, [](const T &x) {
        return x >= 0 ? std::optional<T>{x} : std::optional<T>{};
    });
    if (!checked) {
        return 0;
    }
    return foldable.length(tree);
}

/** The same summary through the operation objects. */
template <class T>
auto summarize_through_objects(const BinaryTree<T> &tree) -> std::size_t {
    auto checked = tc::traverse(
        [](const T &x) {
            return x >= 0 ? std::optional<T>{x} : std::optional<T>{};
        },
        tree);
    if (!checked) {
        return 0;
    }
    return tc::length(tree);
}
// 7f7a0dee-cb47-47e1-a113-b1fab3604e90 end

// -- Case 2: the caller is not a template ---------------------------------

// 31b7b877-08dd-472b-9d08-5978a2502251
/** Total a concrete vector of readings, rejecting any out-of-range value.
 *
 * This function is not a template and has no reason to become one. There is
 * no template parameter list to carry `const auto &TC`, so mode 3 is simply
 * unavailable here -- and the instances are fixed at authoring time anyway,
 * because the argument type is. The operation objects resolve them at the
 * call site.
 */
auto total_valid_readings(const std::vector<int> &readings) -> int {
    if (tc::empty(readings)) {
        return 0;
    }
    if (!tc::all_of(readings, [](int x) { return x >= 0 && x < 1000; })) {
        return -1;
    }
    return tc::fold_left(readings, 0, [](int acc, int x) { return acc + x; });
}
// 31b7b877-08dd-472b-9d08-5978a2502251 end

// -- Pinning, after the object form takes mode 3 away ----------------------

// 98f2de5d-d3c1-41ab-b36e-62e2f88f1e82
/** An alternate Functor instance for std::vector, never registered.
 *
 * Reaching it needs mode 2 -- the instance object called directly -- which is
 * the answer to losing mode 3, and is shorter than any spelling that could
 * have been routed through `typeclass`.
 */
template <class T>
struct ReverseFunctorImpl {
    template <class F>
    auto fmap(this auto &&, F &&function, const std::vector<T> &values) {
        using Result = bt::remove_cvref_t<std::invoke_result_t<F, const T &>>;
        std::vector<Result> output;
        output.reserve(values.size());
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            output.push_back(std::invoke(function, *it));
        }
        return output;
    }
};

template <class T>
struct ReverseFunctorMap : bt::Functor<ReverseFunctorImpl<T>> {
    using ReverseFunctorImpl<T>::fmap;
};

inline constexpr ReverseFunctorMap<int> reverse_functor{};
// 98f2de5d-d3c1-41ab-b36e-62e2f88f1e82 end

} // namespace

int main() {
    // tree: node(2) with left = leaf(3), right = leaf(5); three nodes.
    auto tree = BinaryTree<int>::node(2, BinaryTree<int>::leaf(3),
                                      BinaryTree<int>::leaf(5));
    auto with_negative = BinaryTree<int>::node(-1, BinaryTree<int>::leaf(2),
                                               BinaryTree<int>::leaf(3));

    std::cout << "Case 1 -- everything local:\n";
    std::cout << "  through lookups: " << summarize_through_lookups(tree)
              << '\n';
    std::cout << "  through objects: " << summarize_through_objects(tree)
              << '\n';
    std::cout << "  rejected tree:   "
              << summarize_through_objects(with_negative) << '\n';

    std::cout << "Case 2 -- non-template caller:\n";
    std::cout << "  valid:     " << total_valid_readings({3, 1, 4, 1, 5})
              << '\n';
    std::cout << "  empty:     " << total_valid_readings({}) << '\n';
    std::cout << "  rejected:  " << total_valid_readings({3, -1, 4}) << '\n';

    // 65770d79-e4c5-40f2-add9-85817bbd15c2
    // Pinning. typeclass::fmap takes the registered vector Functor; the
    // instance object reaches one that was never registered at all.
    const std::vector<int> values{1, 2, 3};
    auto label = [](int x) { return x * 10; };

    auto registered = tc::fmap(label, values);
    auto pinned = reverse_functor.fmap(label, values);
    // 65770d79-e4c5-40f2-add9-85817bbd15c2 end

    std::cout << "Pinning:\n";
    std::cout << "  registered instance:";
    for (int x : registered) {
        std::cout << ' ' << x;
    }
    std::cout << "\n  pinned instance:    ";
    for (int x : pinned) {
        std::cout << ' ' << x;
    }
    std::cout << '\n';

    return 0;
}
