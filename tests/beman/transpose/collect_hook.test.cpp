// tests/beman/transpose/collect_hook.test.cpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The Traversable side of docs/decisions.md#runtime-arity-composition: the
// vector traversal prefers an applicative object's native `collect` and
// derives against `pure`/`invoke` otherwise.
//
// Everything here is generic. No sender, no execution dependency, nothing
// that needs BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE -- which is the point:
// the hook knows nothing about what supplies `collect`, so the property it
// has can be stated without one. The sender instance that motivated it is
// checked in transpose_senders.test.cpp, under the option.

#include <beman/transpose/sequence.hpp>

#include <beman/transpose/apply.hpp>
#include <beman/transpose/transpose.hpp>
#include <beman/transpose/traverse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace bt = beman::transpose;

namespace {

/// Whether an applicative object answers the probe the vector traversal
/// makes. Spelled here rather than reached for in `detail` so that the test
/// states the property independently of the header's spelling of it: if the
/// two ever disagree, this is what says so.
template <class APPLICATIVE, class EFFECT>
concept offers_collect = requires(const APPLICATIVE &applicative) {
    applicative.collect(std::declval<std::vector<EFFECT>>());
};

// -- a context whose object has no `collect`: the fold path ---------------

/// A box whose composition is type-stable, the shape every instance this
/// library registers has. Its object counts compositions so a traversal can
/// be asked which path it took rather than only what it returned.
template <class T>
struct boxed {
    using value_type = T;
    T held;

    friend auto operator==(const boxed &, const boxed &) -> bool = default;
};

struct composition_counts {
    static inline long invokes = 0;
    static inline long collects = 0;

    static auto reset() -> void {
        invokes = 0;
        collects = 0;
    }
};

struct FoldingApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        return boxed<std::remove_cvref_t<VALUE>>{std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class... BOXES>
    auto invoke(this auto &&, FUNCTION &&function, BOXES &&...boxes) {
        ++composition_counts::invokes;
        return boxed<std::remove_cvref_t<std::invoke_result_t<
            FUNCTION &, typename std::remove_cvref_t<BOXES>::value_type...>>>{
            function(std::forward<BOXES>(boxes).held...)};
    }
};

struct FoldingApplicativeMap : bt::Applicative<FoldingApplicativeImpl> {
    using FoldingApplicativeImpl::invoke;
    using FoldingApplicativeImpl::pure;
};

// -- the same context, by an object that does offer `collect` -------------

/// Identical basis, plus the range twin of `invoke`. Nothing else differs,
/// so a traversal that behaves differently under the two objects did so
/// because of `collect` and for no other reason.
struct CollectingApplicativeImpl : FoldingApplicativeImpl {
    template <class T>
    auto collect(this auto &&, std::vector<boxed<T>> boxes) {
        ++composition_counts::collects;
        std::vector<T> values;
        values.reserve(boxes.size());
        for (auto &box : boxes) {
            values.push_back(std::move(box.held));
        }
        return boxed<std::vector<T>>{std::move(values)};
    }
};

struct CollectingApplicativeMap : bt::Applicative<CollectingApplicativeImpl> {
    using CollectingApplicativeImpl::collect;
    using CollectingApplicativeImpl::invoke;
    using CollectingApplicativeImpl::pure;
};

} // namespace

// -- deliverable 3: `collect` is optional in `applicative_object` ---------
//
// docs/decisions.md#collect-hook. Both objects above conform over the same
// context, and they differ only in whether they supply `collect`. An object
// that has it is not thereby a different kind of thing, and an object that
// lacks it -- every instance this library registers -- is not thereby
// deficient.

static_assert(bt::applicative_object<FoldingApplicativeMap, boxed<int>>);
static_assert(bt::applicative_object<CollectingApplicativeMap, boxed<int>>);

static_assert(!offers_collect<FoldingApplicativeMap, boxed<int>>);
static_assert(offers_collect<CollectingApplicativeMap, boxed<int>>);

// The registered instances have no `collect`, which is what says they still
// take the fold -- stated per object so that acquiring one becomes a test
// failure rather than a silent change of path.

static_assert(!offers_collect<
              std::remove_cvref_t<
                  decltype(bt::applicative_typeclass<std::optional<int>>)>,
              std::optional<int>>);

TEST_CASE("collect hook: an object without collect takes the pairwise fold") {
    composition_counts::reset();

    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    auto result = traversable.traverse(
        FoldingApplicativeMap{}, [](int x) { return boxed<int>{x + 1}; },
        std::vector<int>{1, 2, 3, 4});

    REQUIRE(result == boxed<std::vector<int>>{{2, 3, 4, 5}});

    // One composition per element: the fold, unchanged.
    REQUIRE(composition_counts::invokes == 4);
    REQUIRE(composition_counts::collects == 0);
}

TEST_CASE("collect hook: an object with collect is preferred, once") {
    composition_counts::reset();

    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    auto result = traversable.traverse(
        CollectingApplicativeMap{}, [](int x) { return boxed<int>{x + 1}; },
        std::vector<int>{1, 2, 3, 4});

    REQUIRE(result == boxed<std::vector<int>>{{2, 3, 4, 5}});

    // The whole point: one composition for the whole structure, and the
    // pairwise basis not reached at all.
    REQUIRE(composition_counts::collects == 1);
    REQUIRE(composition_counts::invokes == 0);
}

TEST_CASE("collect hook: both traverse overloads prefer collect") {
    // The rvalue overload is a separate definition with its own body, so the
    // preference has to be established twice or it is established once.
    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    const std::vector<int> values{1, 2, 3};

    composition_counts::reset();
    auto from_lvalue = traversable.traverse(
        CollectingApplicativeMap{}, [](int x) { return boxed<int>{x}; },
        values);
    REQUIRE(composition_counts::collects == 1);
    REQUIRE(composition_counts::invokes == 0);
    REQUIRE(from_lvalue == boxed<std::vector<int>>{{1, 2, 3}});

    composition_counts::reset();
    auto from_rvalue = traversable.traverse(
        CollectingApplicativeMap{}, [](int x) { return boxed<int>{x}; },
        std::vector<int>{1, 2, 3});
    REQUIRE(composition_counts::collects == 1);
    REQUIRE(composition_counts::invokes == 0);
    REQUIRE(from_rvalue == boxed<std::vector<int>>{{1, 2, 3}});
}

TEST_CASE("collect hook: the empty structure reaches collect too") {
    // An empty vector is where a fold returns `pure({})` without composing
    // anything, so it is the one size at which taking the wrong path is
    // invisible in the result. Pin it.
    composition_counts::reset();

    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    auto result = traversable.traverse(
        CollectingApplicativeMap{}, [](int x) { return boxed<int>{x}; },
        std::vector<int>{});

    REQUIRE(result.held.empty());
    REQUIRE(composition_counts::collects == 1);
    REQUIRE(composition_counts::invokes == 0);
}

TEST_CASE("collect hook: registered instances are unchanged by it") {
    // The fold path for the shipped instances, through the front door rather
    // than through a hand-passed object. The goldens say the same thing at
    // greater length; this says it where the hook is.
    std::vector<std::optional<int>> values{1, 2, 3};
    auto transposed = bt::transpose(values);
    REQUIRE(transposed == std::optional<std::vector<int>>{{1, 2, 3}});

    std::vector<std::optional<int>> with_gap{1, std::nullopt, 3};
    REQUIRE_FALSE(bt::transpose(with_gap).has_value());
}
