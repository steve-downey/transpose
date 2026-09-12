// tests/beman/transpose/transpose.test.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Paper A's three-domain proof: a single front-door `transpose` flips
//   structure<context<T>>  ->  context<structure<T>>
// for three independent contexts (fallible value, deferred result, lanewise),
// all over the same std::vector structure and the same API.

#include <beman/transpose/transpose.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

TEST_CASE(
    "transpose: optional domain (vector<optional<T>> -> optional<vector<T>>)") {
    std::vector<std::optional<int>> all{
        std::optional<int>{1}, std::optional<int>{2}, std::optional<int>{3}};
    REQUIRE(bt::transpose(all) == std::optional<std::vector<int>>{{1, 2, 3}});

    std::vector<std::optional<int>> gapped{
        std::optional<int>{1}, std::optional<int>{}, std::optional<int>{3}};
    REQUIRE(bt::transpose(gapped) == std::optional<std::vector<int>>{});
}

TEST_CASE("transpose: sender domain (vector<sender<T>> -> sender<vector<T>>)") {
    auto runs = std::make_shared<int>(0);
    auto deferred = [runs](int v) {
        return bt::sender<int>{[runs, v]() {
            ++(*runs);
            return v;
        }};
    };

    std::vector<bt::sender<int>> senders{deferred(10), deferred(20),
                                         deferred(30)};

    auto composed = bt::transpose(senders);
    // Deferred: nothing has run until the composed sender is run.
    REQUIRE(*runs == 0);

    REQUIRE(composed.get() == std::vector<int>{10, 20, 30});
    REQUIRE(*runs == 3);
}

TEST_CASE(
    "transpose: zip/SIMD domain (vector<zip_list<T>> -> zip_list<vector<T>>)") {
    // Three structure positions, each a 2-lane computation.
    std::vector<bt::zip_list<int>> lanes{
        bt::zip_list<int>{{1, 2}},
        bt::zip_list<int>{{10, 20}},
        bt::zip_list<int>{{100, 200}},
    };

    auto transposed = bt::transpose(lanes);
    // Result is 2 lanes, each a vector across the three positions.
    REQUIRE_FALSE(transposed.is_repeating());
    REQUIRE(transposed.data.size() == 2);
    REQUIRE(transposed.data[0] == std::vector<int>{1, 10, 100});
    REQUIRE(transposed.data[1] == std::vector<int>{2, 20, 200});
}

// --- The front door is detectable -----------------------------------------
//
// transpose's Constraints clause says it is available where
// traversable_typeclass names a traversable object for the argument's type
// and applicative_typeclass names an applicative object for that structure's
// element type. Until that was an associated constraint the declaration
// said none of it: the return type is deduced, so asking whether transpose
// is available for a type instantiated the body and hard-errored instead of
// answering no. A caller could not write a concept over the front door.
//
// Spelled as a named concept, not a bare requires-expression at block scope:
// the latter hard-errors on an invalid expression rather than yielding false
// ([expr.prim.req]).

namespace {

template <class T>
concept transposable = requires(T &&value) {
    bt::transpose(std::forward<T>(value));
};

struct not_a_structure {};

} // namespace

// Positive controls, one per motivating domain.
static_assert(transposable<std::vector<std::optional<int>>>);
static_assert(transposable<std::vector<std::optional<int>> &>);
static_assert(transposable<std::vector<bt::sender<int>>>);

// A type that names no traversable object at all.
static_assert(!transposable<int>);
static_assert(!transposable<not_a_structure>);

// A structure that IS traversable, but whose elements are not a context --
// transposing it is not a meaningful operation, not a missing one.
static_assert(!transposable<std::vector<int>>);
