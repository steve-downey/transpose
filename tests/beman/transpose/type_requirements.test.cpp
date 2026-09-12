// tests/beman/transpose/type_requirements.test.cpp                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The type requirements the operations actually impose, pinned against the
// ones they document.
//
// The rest of the suite exercises the operations with scalars, and a scalar
// satisfies every requirement anyone could accidentally reach for: it is
// default-constructible, assignable, copyable, and invocable-with in any
// value category. That makes a passing scalar suite silent about the
// difference between a requirement the wording states and one an
// implementation acquired by accident -- which is precisely the class of
// defect the P3200 reference-implementation review found. The fixtures here
// satisfy exactly what is documented and nothing beyond it, so an
// undocumented requirement is a compile error rather than a footnote.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/array.hpp>
#include <beman/transpose/simd_lanes.hpp>
#include <beman/transpose/transpose.hpp>
#include <beman/transpose/traverse.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

using beman::transpose::test::copyable_only;
using beman::transpose::test::lvalue_only_callable;
using beman::transpose::test::rvalue_only_callable;

namespace {

template <class CONTEXT>
using object_for =
    bt::remove_cvref_t<decltype(bt::applicative_typeclass<CONTEXT>)>;

template <class STRUCTURE>
using traversable_for =
    bt::remove_cvref_t<decltype(bt::traversable_typeclass<STRUCTURE>)>;

// A named concept, not a bare requires-expression at block scope: the latter
// hard-errors on an invalid expression instead of yielding false
// ([expr.prim.req]). See the note at the top of test_support.hpp.
template <class F, class T>
concept traversable_with = requires(F &&function, T &&value) {
    bt::traverse(std::forward<F>(function), std::forward<T>(value));
};

} // namespace

// --- Concept probes must not require a default-constructible element -------
//
// The concepts probe operations templated over an arbitrary callable with a
// representative witness. Probing a derived operation with a deduced return
// type instantiates enough of that operation's body to instantiate the
// witness call, so a witness that returns `RESULT{}` puts a
// default-construction requirement into every such concept -- one no
// operation's specification states.

static_assert(bt::applicative_object<object_for<std::optional<copyable_only>>,
                                     std::optional<copyable_only>>);

static_assert(
    bt::applicative_object_for<object_for<std::optional<copyable_only>>,
                               std::optional<copyable_only>>);

static_assert(
    bt::traversable_object<traversable_for<std::vector<copyable_only>>,
                           std::vector<copyable_only>>);

static_assert(bt::applicative_object<object_for<std::array<copyable_only, 3>>,
                                     std::array<copyable_only, 3>>);

// --- Array and lane results are constructed, not assigned into -------------

TEST_CASE("type requirements: array pure broadcasts without assigning") {
    const auto &app = bt::applicative_typeclass<std::array<copyable_only, 3>>;

    auto broadcast = app.pure(copyable_only{7});

    REQUIRE(broadcast.size() == 3);
    REQUIRE(broadcast[0] == copyable_only{7});
    REQUIRE(broadcast[1] == copyable_only{7});
    REQUIRE(broadcast[2] == copyable_only{7});
}

TEST_CASE("type requirements: array invoke builds lanes without assigning") {
    const auto &app = bt::applicative_typeclass<std::array<copyable_only, 3>>;

    std::array<copyable_only, 3> left{copyable_only{1}, copyable_only{2},
                                      copyable_only{3}};
    std::array<copyable_only, 3> right{copyable_only{10}, copyable_only{20},
                                       copyable_only{30}};

    auto combined = app.invoke(
        [](const copyable_only &a, const copyable_only &b) {
            return copyable_only{a.value + b.value};
        },
        left, right);

    REQUIRE(combined[0] == copyable_only{11});
    REQUIRE(combined[1] == copyable_only{22});
    REQUIRE(combined[2] == copyable_only{33});
}

TEST_CASE("type requirements: simd_lanes builds lanes without assigning") {
    const auto &app =
        bt::applicative_typeclass<bt::simd_lanes<copyable_only, 3>>;

    auto repeated = bt::simd_lanes<copyable_only, 3>::repeat(copyable_only{5});
    REQUIRE(repeated.data[0] == copyable_only{5});
    REQUIRE(repeated.data[2] == copyable_only{5});

    auto doubled = app.invoke(
        [](const copyable_only &a) { return copyable_only{a.value * 2}; },
        repeated);

    REQUIRE(doubled.data[0] == copyable_only{10});
    REQUIRE(doubled.data[2] == copyable_only{10});
}

// --- Callables are detected in the category they are invoked in ------------
//
// A traversal invokes a named `function` variable once per element, always as
// an lvalue. Type computation that spells `invoke_result_t<F, ...>` for a
// deduced `F&&` tests the wrong category for a caller who passes a temporary:
// it rejects an `&`-qualified callable that would have worked, and accepts an
// `&&`-only callable that then fails inside the loop.

static_assert(traversable_with<lvalue_only_callable, std::vector<int>>,
              "an &-qualified callable is invocable in the category a "
              "traversal actually uses");

static_assert(traversable_with<lvalue_only_callable &, std::vector<int>>);

static_assert(!traversable_with<rvalue_only_callable, std::vector<int>>,
              "an &&-only callable is never invocable by a traversal, and "
              "must fail to match rather than fail mid-instantiation");

TEST_CASE("type requirements: traverse accepts an lvalue-only callable") {
    std::vector<int> values{1, 2, 3};

    auto from_temporary = bt::traverse(lvalue_only_callable{10}, values);
    REQUIRE(from_temporary.has_value());
    REQUIRE(*from_temporary == std::vector<int>{11, 12, 13});

    lvalue_only_callable named{100};
    auto from_lvalue = bt::traverse(named, values);
    REQUIRE(from_lvalue.has_value());
    REQUIRE(*from_lvalue == std::vector<int>{101, 102, 103});
}

// --- The front door over a non-default-constructible element ---------------

TEST_CASE("type requirements: transpose over a copy-only element type") {
    std::vector<std::optional<copyable_only>> values{
        std::optional<copyable_only>{copyable_only{1}},
        std::optional<copyable_only>{copyable_only{2}},
        std::optional<copyable_only>{copyable_only{3}}};

    auto transposed = bt::transpose(values);

    REQUIRE(transposed.has_value());
    REQUIRE(transposed->size() == 3);
    REQUIRE((*transposed)[0] == copyable_only{1});
    REQUIRE((*transposed)[2] == copyable_only{3});

    // Built rather than assigned into: optional's assignment operator is
    // deleted for an element type that is not itself assignable, which is
    // exactly the point of this fixture.
    std::vector<std::optional<copyable_only>> with_a_gap{
        std::optional<copyable_only>{copyable_only{1}},
        std::optional<copyable_only>{},
        std::optional<copyable_only>{copyable_only{3}}};

    REQUIRE_FALSE(bt::transpose(with_a_gap).has_value());
}

// --- A consuming traversal over a move-only element type -------------------
//
// transpose(std::move(v)) looks like it consumes v, and until the traversal
// grew a consuming overload it did not: the vector Traversable primitive
// accepted only `const vector&`, so the front door's forwarding reference
// forwarded an rvalue into a parameter that could only copy from it. A
// move-only element type makes the difference impossible to overlook -- it
// is the case that does not merely cost more, it does not compile at all.

TEST_CASE("type requirements: transpose consumes a move-only structure") {
    std::vector<std::optional<std::unique_ptr<int>>> values;
    values.push_back(std::make_unique<int>(1));
    values.push_back(std::make_unique<int>(2));
    values.push_back(std::make_unique<int>(3));

    auto transposed = bt::transpose(std::move(values));

    REQUIRE(transposed.has_value());
    REQUIRE(transposed->size() == 3);
    REQUIRE(*(*transposed)[0] == 1);
    REQUIRE(*(*transposed)[2] == 3);
}

TEST_CASE("type requirements: a disengaged move-only element propagates") {
    std::vector<std::optional<std::unique_ptr<int>>> values;
    values.push_back(std::make_unique<int>(1));
    values.emplace_back();
    values.push_back(std::make_unique<int>(3));

    REQUIRE_FALSE(bt::transpose(std::move(values)).has_value());
}

// --- A const-only applicative object still composes ------------------------
//
// Handing the accumulated result to each composition as an rvalue is what
// makes the traversal linear, but it must not become a requirement: an
// applicative object that takes its operands by `const&` is still a
// perfectly good one, and the test suite's Identity instance is deliberately
// left that way so that this stays covered. Such an object copies the prefix
// and so is not linear -- it is still correct, which is what this pins.

TEST_CASE("type requirements: a const-only applicative still traverses") {
    auto result = bt::traverse(
        [](int element) {
            return beman::transpose::test::Identity<int>{element + 1};
        },
        std::vector<int>{1, 2, 3});

    REQUIRE(result ==
            beman::transpose::test::Identity<std::vector<int>>{{2, 3, 4}});
}
