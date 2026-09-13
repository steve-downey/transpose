// tests/beman/transpose/sequence.test.cpp                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/sequence.hpp>

#include <beman/transpose/expected.hpp>
#include <beman/transpose/transpose.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <expected>
#include <optional>
#include <system_error>
#include <vector>

namespace bt = beman::transpose;

using beman::transpose::test::counted;

namespace {

// Traverses a vector of `size` engaged optionals and reports how many element
// copies the traversal itself performed. Building the input is excluded: the
// counter is reset after the vector exists and before transpose runs.
auto element_copies_transposing(std::size_t size) -> long {
    std::vector<std::optional<counted>> values;
    values.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        values.emplace_back(counted{static_cast<int>(index)});
    }

    counted::reset();
    auto transposed = bt::transpose(values);
    const auto copies = counted::copies;

    REQUIRE(transposed.has_value());
    REQUIRE(transposed->size() == size);
    REQUIRE(transposed->front() == counted{0});
    REQUIRE(transposed->back() == counted{static_cast<int>(size) - 1});

    return copies;
}

} // namespace

TEST_CASE("sequence: vector foldable length and to_vector") {
    const auto &f = bt::foldable_typeclass<std::vector<int>>;
    std::vector<int> xs{4, 5, 6};
    REQUIRE(f.length(xs) == 3);
    REQUIRE(f.to_vector(xs) == std::vector<int>{4, 5, 6});
    REQUIRE(f.fold_left(xs, 0, [](int a, int x) { return a + x; }) == 15);
}

TEST_CASE("sequence: vector foldable length takes the native path, no Map "
          "edit needed") {
    // VectorFoldableImpl grows a native length in this step; VectorFoldableMap
    // is untouched (no `using length;` was added). length must still answer
    // correctly for empty, one-element, and many-element vectors -- it is
    // now taking the native size()-based path instead of the fold_map-derived
    // one, and the answer must not move.
    const auto &f = bt::foldable_typeclass<std::vector<int>>;
    REQUIRE(f.length(std::vector<int>{}) == 0);
    REQUIRE(f.length(std::vector<int>{42}) == 1);
    REQUIRE(f.length(std::vector<int>{1, 2, 3, 4, 5}) == 5);
}

TEST_CASE("sequence: vector traversable primitive sequences effects") {
    const auto &t = bt::traversable_typeclass<std::vector<int>>;
    const auto &app = bt::applicative_typeclass<std::optional<int>>;
    auto result = t.traverse(
        app, [](int x) { return std::optional<int>{x + 1}; },
        std::vector<int>{1, 2, 3});
    REQUIRE(result == std::optional<std::vector<int>>{{2, 3, 4}});
}

TEST_CASE("sequence: successful traversal is linear in element copies") {
    // The wording promises transpose is linear in the number of elements.
    // Reading that from a count needs two sizes: doubling the input doubles
    // the copies of a linear traversal and roughly quadruples those of one
    // that rebuilds its accumulated prefix at every step. Both traversals
    // return the same vector, so only the count can tell them apart.
    const auto small = element_copies_transposing(100);
    const auto large = element_copies_transposing(200);

    INFO("copies for 100 elements: " << small);
    INFO("copies for 200 elements: " << large);

    REQUIRE(small <= 4 * 100);
    REQUIRE(large < 3 * small);
}

namespace {

// The same measurement over the other registered standard context. The
// accumulation shape is the applicative object's, not the vector's, so each
// registered object needs its own reading.
auto element_copies_transposing_expected(std::size_t size) -> long {
    std::vector<std::expected<counted, std::errc>> values;
    values.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        values.emplace_back(counted{static_cast<int>(index)});
    }

    counted::reset();
    auto transposed = bt::transpose(values);
    const auto copies = counted::copies;

    REQUIRE(transposed.has_value());
    REQUIRE(transposed->size() == size);

    return copies;
}

} // namespace

TEST_CASE("sequence: traversal into expected is linear in element copies") {
    const auto small = element_copies_transposing_expected(100);
    const auto large = element_copies_transposing_expected(200);

    INFO("copies for 100 elements: " << small);
    INFO("copies for 200 elements: " << large);

    REQUIRE(small <= 4 * 100);
    REQUIRE(large < 3 * small);
}
