// tests/beman/transpose/fold.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/transpose/fold.hpp>
#include <beman/transpose/sequence.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace bt = beman::transpose;

namespace {

template <class M, class F, class T>
concept has_fold_map = requires(M &m, F &f, T &t) { m.fold_map(f, t); };

template <class M, class F, class T, class MONOID>
concept has_fold_map_with =
    requires(M &m, F &f, T &t, MONOID mo) { m.fold_map(f, t, mo); };

template <class M, class T>
concept has_combine_all = requires(M &m, T &t) { m.combine_all(t); };

// An Impl with a native `length` returning a sentinel the fold_map
// derivation could never produce, registered through a Map with no `using`
// for it -- the ergonomics claim, exercised directly: an instance author
// adds a native member and gets it back with no Map edit.
struct MarkerLengthImpl {
    template <class T>
    auto length(this auto &&, const T &) -> std::size_t {
        return 999;
    }
};

struct MarkerLengthMap : bt::Foldable<MarkerLengthImpl> {};

// An Impl with a fold_map over std::vector<V>, and a native to_vector for
// std::vector<int> only -- a marker the fold_map derivation could never
// produce, and unreachable for any other element type. This is the
// per-instantiation property a Map-level `using` could never express. The
// Impl is a template over V so that it can declare element_type, which the
// derived to_vector needs to name the vector it collects into.
template <class V>
struct PerInstantiationToVectorImpl {
    using element_type = V;

    template <class FUNCTION, class MONOID>
    auto fold_map(this auto &&, FUNCTION &&function,
                  const std::vector<V> &values, MONOID monoid) {
        using Result =
            std::remove_cvref_t<std::invoke_result_t<FUNCTION &, const V &>>;
        Result accumulated = monoid.identity();
        for (const auto &value : values) {
            accumulated = monoid.combine(std::move(accumulated),
                                         std::invoke(function, value));
        }
        return accumulated;
    }

    auto to_vector(this auto &&, const std::vector<int> &) -> std::vector<int>
        requires std::same_as<V, int>
    {
        return std::vector<int>{-1};
    }
};

template <class V>
struct PerInstantiationToVectorMap
    : bt::Foldable<PerInstantiationToVectorImpl<V>> {};

// The hazard this step closes: an Impl providing only fold_right +
// element_type, no fold_map at all, registered through a Map with no
// using-declaration whatsoever -- the Map body below is exactly its
// definition and nothing else. Before this step, fold_map's derivation and
// fold_right's derivation were both self-routed, so a Map shaped like this
// sent the two into unbounded mutual template recursion (each round nesting
// another RightFoldProgram) rather than a clean answer. After this step,
// fold_map is available and correct because its derivation addresses Impl
// directly instead of re-entering through self.
struct FoldRightOnlyImpl {
    using element_type = int;

    // A plain const member, not an explicit-object one: MSVC's backend
    // ICEs (C1001, p2) emitting the deducing-this form of exactly this
    // body; see the twin in objects.test.cpp.
    template <class STATE, class FUNCTION>
    auto fold_right(const std::vector<int> &values, STATE initial_state,
                    FUNCTION &&function) const -> STATE {
        STATE state = std::move(initial_state);
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            state = std::invoke(function, *it, std::move(state));
        }
        return state;
    }
};

struct FoldRightOnlyMap : bt::Foldable<FoldRightOnlyImpl> {};

} // namespace

TEST_CASE("fold: length prefers a native Impl::length, no Map using needed") {
    MarkerLengthMap m{};
    REQUIRE(m.length(std::vector<int>{1, 2, 3}) == 999);
}

TEST_CASE("fold: to_vector native preference is per instantiation") {
    // std::vector<int>: the native to_vector fires.
    REQUIRE(PerInstantiationToVectorMap<int>{}.to_vector(
                std::vector<int>{1, 2, 3}) == std::vector<int>{-1});

    // std::vector<std::string>: no native to_vector exists for this element
    // type, so the same member falls back to the fold_map derivation.
    REQUIRE(PerInstantiationToVectorMap<std::string>{}.to_vector(
                std::vector<std::string>{"a", "b"}) ==
            std::vector<std::string>{"a", "b"});
}

TEST_CASE("fold: fold_map derives from a fold_right + element_type Impl, "
          "with no Map using-declaration at all") {
    FoldRightOnlyMap m{};
    std::vector<int> xs{1, 2, 3, 4};

    // fold_map itself, via the fold_right basis -- this is the shape that
    // would have recursed unboundedly before this step.
    REQUIRE(m.fold_map([](int) { return std::size_t{1}; }, xs,
                       bt::sum_monoid<std::size_t>{}) == 4);

    // The rest of the family, all routed through the same fold_map.
    REQUIRE(m.length(xs) == 4);
    REQUIRE(m.to_vector(xs) == xs);
    REQUIRE(m.fold_right(xs, std::string{}, [](int x, std::string acc) {
        return acc + std::to_string(x);
    }) == "4321");
}

TEST_CASE("fold: fold_map does not exist when Impl provides neither basis") {
    struct NoBasisImpl {};
    struct NoBasisMap : bt::Foldable<NoBasisImpl> {};

    static_assert(!has_fold_map<NoBasisMap, decltype([](int x) { return x; }),
                                std::vector<int>>);
    static_assert(
        !has_fold_map_with<NoBasisMap, decltype([](int x) { return x; }),
                           std::vector<int>, bt::sum_monoid<int>>);
}

TEST_CASE("fold: a fold over a type with several monoids names its instance") {
    // docs/decisions.md#monoid-selection: int carries addition, product,
    // max, min, ... so no Monoid<int> is registered, and the two-argument
    // fold_map is not a candidate -- not a hard error from inside the body.
    // The caller passes the instance. std::string has one canonical monoid,
    // so the two-argument form is available and defaults to it.
    using Map = bt::VectorFoldableMap<int>;
    using Identity = decltype([](int x) { return x; });
    static_assert(!has_fold_map<Map, Identity, std::vector<int>>);
    static_assert(has_fold_map_with<Map, Identity, std::vector<int>,
                                    bt::sum_monoid<int>>);
    static_assert(!has_combine_all<Map, std::vector<int>>);
    static_assert(has_combine_all<bt::VectorFoldableMap<std::string>,
                                  std::vector<std::string>>);

    Map m{};
    std::vector<int> xs{3, 1, 4, 1, 5};
    REQUIRE(m.fold_map([](int x) { return x; }, xs, bt::sum_monoid<int>{}) ==
            14);
    REQUIRE(m.fold_map([](int x) { return x; }, xs, bt::max_monoid<int>{}) ==
            5);
    REQUIRE(m.combine_all(xs, bt::product_monoid<int>{}) == 60);
    REQUIRE(m.fold(xs, bt::min_monoid<int>{}) == 1);

    // Instances compose, and the value type stays bare: a pair of ints
    // under sum and max at once, no wrapper on either component.
    REQUIRE(
        (m.fold_map(
             [](int x) { return std::tuple{x, x}; }, xs,
             bt::tuple_monoid<bt::sum_monoid<int>, bt::max_monoid<int>>{}) ==
         std::tuple{14, 5}));

    bt::VectorFoldableMap<std::string> ms{};
    std::vector<std::string> ss{"a", "b", "c"};
    REQUIRE(ms.combine_all(ss) == "abc");
    REQUIRE(ms.fold_map([](const std::string &s) { return s; }, ss) == "abc");
    REQUIRE(ms.combine_all(ss, bt::dual_monoid<bt::Monoid<std::string>>{}) ==
            "cba");
}

TEST_CASE("fold: length and to_vector over Sequence") {
    const auto &f = bt::foldable_typeclass<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{10, 20, 30}};
    REQUIRE(f.length(seq) == 3);
    REQUIRE(f.to_vector(seq) == std::vector<int>{10, 20, 30});
}

TEST_CASE("fold: fold_left and fold_right accumulate") {
    const auto &f = bt::foldable_typeclass<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{1, 2, 3, 4}};
    REQUIRE(f.fold_left(seq, 0, [](int acc, int x) { return acc + x; }) == 10);
    REQUIRE(f.fold_right(seq, std::string{}, [](int x, std::string acc) {
        return acc + std::to_string(x);
    }) == "4321");
}

TEST_CASE("fold: any_of, all_of, find_first") {
    const auto &f = bt::foldable_typeclass<bt::test::Sequence<int>>;
    bt::test::Sequence<int> seq{{2, 4, 6, 7}};
    REQUIRE(f.any_of(seq, [](int x) { return x % 2 == 1; }));
    REQUIRE_FALSE(f.all_of(seq, [](int x) { return x % 2 == 0; }));
    REQUIRE(f.find_first(seq, [](int x) { return x > 5; }) == 6);
}
