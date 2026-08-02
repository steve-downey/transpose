// tests/beman/transpose/ops.test.cpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The operation objects in beman::transpose::ops: one per typeclass
// operation, each doing the lookup its callers used to do by hand.
//
// Two things are under test. First, that every operation object agrees
// exactly with the lookup object it dispatches through -- it must be a
// spelling, never a second semantics. Second, that the properties the object
// form is adopted for actually hold: an alternate instance is still
// reachable, and a non-template caller can use the operations at all.

#include <beman/transpose/functor.hpp>
#include <beman/transpose/functor.hpp> // re-inclusion / idempotency check

#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/transpose.hpp>
#include <beman/transpose/traverse.hpp>

#include "test_support.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace bt = beman::transpose;

namespace {

// An alternate Functor instance for std::vector, never registered in
// functor_typeclass: the only way to reach it is through mode 2. It is
// wrong on purpose (it drops all but the first element) so that a test
// claiming to have reached it can not pass by accident if the registered
// instance quietly runs instead.
template <class VALUE_TYPE>
struct HeadOnlyFunctorImpl {
    template <class F>
    auto fmap(this auto &&, F &&function,
              const std::vector<VALUE_TYPE> &values) {
        using Result =
            bt::remove_cvref_t<std::invoke_result_t<F, const VALUE_TYPE &>>;
        std::vector<Result> output;
        if (!values.empty()) {
            output.push_back(std::invoke(function, values.front()));
        }
        return output;
    }
};

template <class VALUE_TYPE>
struct HeadOnlyFunctorMap : bt::Functor<HeadOnlyFunctorImpl<VALUE_TYPE>> {
    using HeadOnlyFunctorImpl<VALUE_TYPE>::fmap;
};

inline constexpr HeadOnlyFunctorMap<int> head_only_functor{};

// A type with no instance of anything, for the negative-diagnostic checks.
struct Unregistered {};

// The motivating non-template caller: a plain function over concrete types.
// It has no template parameter list, so there is nowhere to hang a
// `const auto &TC` NTTP -- the options before `ops` were to name a lookup
// object per operation in the body, or to make the function a template it has
// no other reason to be.
auto shipping_weight(const std::vector<int> &parcels) -> int {
    if (bt::ops::empty(parcels)) {
        return 0;
    }
    return bt::ops::fold_left(parcels, 0,
                              [](int acc, int x) { return acc + x; });
}

} // namespace

TEST_CASE("ops: header is idempotent") {
    // Bootstrap: passes if the file compiles and links.
    REQUIRE(true);
}

// -- The operations are objects, not function templates --

TEST_CASE("ops: every operation is an object") {
    // Objects, so ADL never finds them and nothing can overload them. This
    // is the property that makes a lookup-based verb predictable, and it is
    // the behavioural difference from the free function templates `traverse`
    // and `transpose` used to be.
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::fmap)>);
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::invoke)>);
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::traverse)>);
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::transpose)>);
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::fold_map)>);
    STATIC_REQUIRE(std::is_class_v<decltype(bt::ops::bind)>);
    STATIC_REQUIRE(std::is_empty_v<decltype(bt::ops::fmap)>);
}

// -- Functor --

TEST_CASE("ops: fmap agrees with the lookup object") {
    const std::vector<int> values{1, 2, 3};
    auto double_it = [](int x) { return x * 2; };

    const auto &functor = bt::functor_typeclass<std::vector<int>>;
    REQUIRE(bt::ops::fmap(double_it, values) ==
            functor.fmap(double_it, values));
    REQUIRE(bt::ops::fmap(double_it, values) == std::vector<int>{2, 4, 6});
}

TEST_CASE("ops: fmap deduces the instance per argument type") {
    // Same call spelling, two different instances, chosen by the argument.
    REQUIRE(bt::ops::fmap([](int x) { return x + 1; },
                          std::optional<int>{41}) == std::optional<int>{42});
    REQUIRE(bt::ops::fmap([](int x) { return x + 1; },
                          std::vector<int>{1, 2}) == std::vector<int>{2, 3});
}

TEST_CASE("ops: replace reaches the derived operation") {
    const std::vector<int> values{1, 2, 3};
    REQUIRE(bt::ops::replace(values, 7) == std::vector<int>{7, 7, 7});
    REQUIRE(bt::ops::replace(std::optional<int>{1}, 7) ==
            std::optional<int>{7});
    REQUIRE(bt::ops::replace(std::optional<int>{}, 7) == std::optional<int>{});
}

// -- Applicative --

TEST_CASE("ops: invoke lifts a plain function over contexts") {
    auto add = [](int a, int b) { return a + b; };
    REQUIRE(bt::ops::invoke(add, std::optional<int>{2},
                            std::optional<int>{3}) == std::optional<int>{5});
    REQUIRE(bt::ops::invoke(add, std::optional<int>{}, std::optional<int>{3}) ==
            std::optional<int>{});
}

TEST_CASE("ops: ap keys the lookup on the argument, not the callable") {
    // The callable-in-context is optional<lambda>; the instance that matters
    // is the one for the argument's context.
    auto increment = [](int x) { return x + 1; };
    std::optional<decltype(increment)> lifted{increment};
    REQUIRE(bt::ops::ap(lifted, std::optional<int>{41}) ==
            std::optional<int>{42});
    REQUIRE(bt::ops::ap(lifted, std::optional<int>{}) == std::optional<int>{});
}

TEST_CASE("ops: pure names its context because nothing deduces it") {
    // The one operation in the family that the deducing pattern cannot
    // serve: pure builds a context rather than consuming one, so the
    // context lives in the return type and must be spelled.
    REQUIRE(bt::ops::pure<std::optional<int>>(42) == std::optional<int>{42});

    const auto &applicative = bt::applicative_typeclass<std::optional<int>>;
    REQUIRE(bt::ops::pure<std::optional<int>>(42) == applicative.pure(42));
}

TEST_CASE("ops: derived applicative operations dispatch") {
    auto multiply = [](int a, int b) { return a * b; };
    REQUIRE(bt::ops::zip_with(multiply, std::optional<int>{6},
                              std::optional<int>{7}) == std::optional<int>{42});
    REQUIRE(
        bt::ops::discard_first(std::optional<int>{1}, std::optional<int>{2}) ==
        std::optional<int>{2});
    REQUIRE(
        bt::ops::discard_second(std::optional<int>{1}, std::optional<int>{2}) ==
        std::optional<int>{1});
    REQUIRE(
        bt::ops::discard_first(std::optional<int>{}, std::optional<int>{2}) ==
        std::optional<int>{});
}

// -- Traversable --

TEST_CASE("ops: traverse preserves shape and sequences effects") {
    auto positive = [](int x) {
        return x > 0 ? std::optional<int>{x * 2} : std::optional<int>{};
    };
    REQUIRE(bt::ops::traverse(positive, std::vector<int>{1, 2, 3}) ==
            std::optional<std::vector<int>>{{2, 4, 6}});
    REQUIRE(bt::ops::traverse(positive, std::vector<int>{1, -1, 3}) ==
            std::optional<std::vector<int>>{});
}

TEST_CASE("ops: traverse agrees with the lookup object it replaced") {
    auto wrap = [](int x) { return std::optional<int>{x}; };
    const std::vector<int> values{5, 4, 3};

    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    REQUIRE(bt::ops::traverse(wrap, values) ==
            traversable.for_each(values, wrap));
}

TEST_CASE("ops: transpose flips structure and context") {
    const std::vector<std::optional<int>> all{{1}, {2}, {3}};
    const std::vector<std::optional<int>> gapped{{1}, {}, {3}};
    REQUIRE(bt::ops::transpose(all) ==
            std::optional<std::vector<int>>{{1, 2, 3}});
    REQUIRE(bt::ops::transpose(gapped) == std::optional<std::vector<int>>{});
}

// -- Foldable --

TEST_CASE("ops: fold_map is the primitive and agrees with the object") {
    const std::vector<int> values{1, 2, 3, 4};
    auto identity = [](int x) { return x; };

    const auto &foldable = bt::foldable_typeclass<std::vector<int>>;
    REQUIRE(bt::ops::fold_map(identity, values) ==
            foldable.fold_map(identity, values));
    REQUIRE(bt::ops::fold_map(identity, values) == 10);
}

TEST_CASE("ops: derived Foldable operations dispatch") {
    const std::vector<int> values{2, 4, 6, 7};
    REQUIRE(bt::ops::length(values) == 4);
    REQUIRE(bt::ops::to_vector(values) == values);
    REQUIRE(bt::ops::combine_all(values) == 19);
    REQUIRE(bt::ops::any_of(values, [](int x) { return x % 2 == 1; }));
    REQUIRE_FALSE(bt::ops::all_of(values, [](int x) { return x % 2 == 0; }));
    REQUIRE(bt::ops::find_first(values, [](int x) { return x > 5; }) == 6);
    REQUIRE_FALSE(bt::ops::empty(values));
    REQUIRE(bt::ops::empty(std::vector<int>{}));
}

TEST_CASE("ops: fold_left and fold_right keep their argument order") {
    // The operation objects mirror the member signatures exactly, including
    // container-first order that differs from fold_map's function-first one.
    const std::vector<int> values{1, 2, 3, 4};
    REQUIRE(bt::ops::fold_left(values, 0,
                               [](int acc, int x) { return acc + x; }) == 10);
    REQUIRE(
        bt::ops::fold_right(values, std::string{}, [](int x, std::string acc) {
            return acc + std::to_string(x);
        }) == "4321");
}

TEST_CASE("ops: Foldable operations work over a non-vector instance") {
    // test::Sequence has a Foldable instance and nothing else; the objects are
    // not quietly assuming std::vector anywhere.
    const bt::test::Sequence<int> seq{{10, 20, 30}};
    REQUIRE(bt::ops::length(seq) == 3);
    REQUIRE(bt::ops::to_vector(seq) == std::vector<int>{10, 20, 30});
}

// -- Monad (exposition only; see monad.hpp) --

TEST_CASE("ops: bind gets the operation's real name") {
    auto half_if_even = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    REQUIRE(bt::ops::bind(std::optional<int>{8}, half_if_even) ==
            std::optional<int>{4});
    REQUIRE(bt::ops::bind(std::optional<int>{7}, half_if_even) ==
            std::optional<int>{});
    REQUIRE(bt::ops::bind(std::optional<int>{}, half_if_even) ==
            std::optional<int>{});

    // The old mbind spelling still routes to the same place.
    REQUIRE(bt::ops::mbind(std::optional<int>{8}, half_if_even) ==
            bt::ops::bind(std::optional<int>{8}, half_if_even));
}

TEST_CASE("ops: join and kleisli") {
    const std::optional<std::optional<int>> nested{std::optional<int>{5}};
    REQUIRE(bt::ops::join(nested) == std::optional<int>{5});

    auto halve = [](int x) {
        return x % 2 == 0 ? std::optional<int>{x / 2} : std::optional<int>{};
    };
    auto positive = [](int x) {
        return x > 0 ? std::optional<int>{x} : std::optional<int>{};
    };
    // Like pure, kleisli composes two functions and has no in-context
    // argument to deduce from, so the context is named.
    auto composed = bt::ops::kleisli<std::optional<int>>(halve, positive);
    REQUIRE(composed(8) == std::optional<int>{4});
    REQUIRE(composed(7) == std::optional<int>{});
    REQUIRE(composed(-4) == std::optional<int>{});
}

// -- Instance pinning: what the object form costs, and what covers it --

TEST_CASE("ops: an operation object takes no explicit template arguments") {
    // `bt::ops::fmap` is an object, so `bt::ops::fmap<...>(f, v)` does not
    // parse at all -- the template parameters belong to operator(), and the
    // only syntax that reaches them is the .operator() spelling below.
    // Lookup mode 3, NTTP pinning, is what the object form gives up.
    const std::vector<int> values{1, 2, 3};
    auto double_it = [](int x) { return x * 2; };

    // Default lookup: the registered vector Functor, all three elements.
    REQUIRE(bt::ops::fmap(double_it, values) == std::vector<int>{2, 4, 6});

    // The trailing NTTP is reachable, but only like this, and only by
    // respelling both deduced parameters to get past them. Kept as a test so
    // that the claim stays true, not as an API anyone should use.
    REQUIRE(
        bt::ops::fmap.operator()<decltype(double_it) &,
                                 const std::vector<int> &, head_only_functor>(
            double_it, values) == std::vector<int>{2});
}

TEST_CASE("ops: mode 2 is the pinning story, and it is shorter") {
    // The instance object called directly. No second name is needed for
    // pinning, which is why there are no _with operation objects.
    const std::vector<int> values{1, 2, 3};
    auto double_it = [](int x) { return x * 2; };

    REQUIRE(head_only_functor.fmap(double_it, values) == std::vector<int>{2});

    // And it genuinely reaches an unregistered instance: the default lookup
    // for this same type gives a different, correct answer.
    REQUIRE(bt::ops::fmap(double_it, values) != std::vector<int>{2});
}

TEST_CASE("ops: mode 2 reaches every typeclass, derived operations included") {
    const std::vector<int> values{1, 2, 3, 4};
    const auto &foldable = bt::foldable_typeclass<std::vector<int>>;
    const auto &traversable = bt::traversable_typeclass<std::vector<int>>;
    const auto &applicative = bt::applicative_typeclass<std::optional<int>>;
    const auto &monad = bt::monad_typeclass<std::optional<int>>;

    REQUIRE(foldable.fold_map([](int x) { return x; }, values) == 10);
    REQUIRE(foldable.length(values) == 4); // derived, still reachable
    REQUIRE(traversable.for_each(values, [](int x) {
        return std::optional<int>{x};
    }) == std::optional<std::vector<int>>{{1, 2, 3, 4}});
    REQUIRE(applicative.invoke([](int a, int b) { return a + b; },
                               std::optional<int>{2},
                               std::optional<int>{3}) == std::optional<int>{5});
    REQUIRE(monad.bind(std::optional<int>{8}, [](int x) {
        return std::optional<int>{x / 2};
    }) == std::optional<int>{4});
}

TEST_CASE(
    "ops: the non-deducing operations pin by ordinary template argument") {
    // pure and kleisli are variable templates of operation-object types, so
    // their instance is already a template parameter. The two operations that
    // cannot deduce their context are the two that never lost mode 3.
    using pinned =
        bt::ops::pure_fn<std::optional<int>,
                         bt::applicative_typeclass<std::optional<int>>>;
    REQUIRE(pinned{}(42) == std::optional<int>{42});
}

// -- The motivating cases --

TEST_CASE("ops: a non-template function can use the operations") {
    // shipping_weight is not a template. Before `ops` it would have had
    // to open with two `const auto &` lookup-object declarations, or become
    // a template purely to carry the instances.
    REQUIRE(shipping_weight({1, 2, 3, 4}) == 10);
    REQUIRE(shipping_weight({}) == 0);
}

TEST_CASE("ops: everything-local code reads as the operations, not lookups") {
    // The whole point, in one function body: a container on the stack, three
    // typeclasses, no lookup objects named.
    const std::vector<int> readings{3, 1, 4, 1, 5};

    const auto doubled = bt::ops::fmap([](int x) { return x * 2; }, readings);
    const auto total = bt::ops::fold_map([](int x) { return x; }, doubled);
    const auto checked = bt::ops::traverse(
        [](int x) {
            return x < 100 ? std::optional<int>{x} : std::optional<int>{};
        },
        doubled);

    REQUIRE(total == 28);
    REQUIRE(checked == std::optional<std::vector<int>>{{6, 2, 8, 2, 10}});
}

// -- Negative space --

TEST_CASE("ops: a missing instance is detectable before the call") {
    // has_instance_v is what the objects static_assert on; an unregistered
    // is diagnosable rather than reaching the operation as std::false_type.
    STATIC_REQUIRE_FALSE(
        bt::has_instance_v<decltype(bt::functor_typeclass<Unregistered>)>);
    STATIC_REQUIRE(
        bt::has_instance_v<decltype(bt::functor_typeclass<std::vector<int>>)>);
    STATIC_REQUIRE_FALSE(
        bt::has_instance_v<decltype(bt::foldable_typeclass<Unregistered>)>);
    STATIC_REQUIRE_FALSE(
        bt::has_instance_v<decltype(bt::monad_typeclass<Unregistered>)>);
}
