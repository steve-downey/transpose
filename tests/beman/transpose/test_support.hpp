// tests/beman/transpose/test_support.hpp                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TEST_TEST_SUPPORT_HPP
#define BEMAN_TRANSPOSE_TEST_TEST_SUPPORT_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/traverse.hpp>

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::transpose::test {

// Law helpers are spelled through the n-ary invoke core, the only
// application form in this library (the classic ap/apply spellings were
// removed 2026-07-14; see apply.hpp's pattern invariants). Availability
// probes are NAMED CONCEPTS on purpose: a bare requires-expression at block
// scope hard-errors on an invalid expression instead of yielding false
// ([expr.prim.req]).

/** True when the Map exposes one-step contextual application (`ap`, the
 * classic basis kept as a secondary operation) for these operands. It is
 * available exactly when the context can hold a callable; the probe pins
 * its absence where it cannot (std::simd::vec). */
template <class MAP, class FUNCTIONS_IN_CONTEXT, class ARGUMENTS_IN_CONTEXT>
concept has_apply_form =
    requires(const MAP &map, const FUNCTIONS_IN_CONTEXT &cf,
             const ARGUMENTS_IN_CONTEXT &cx) { map.apply(cf, cx); } ||
    requires(const MAP &map, const FUNCTIONS_IN_CONTEXT &cf,
             const ARGUMENTS_IN_CONTEXT &cx) { map.ap(cf, cx); };

/** How a law helper turns a context into something comparable.
 *
 * Every law below states an equation between two contexts, and until
 * 2026-09-13 every one of them tested it with `==` on the contexts
 * themselves. That works for `optional`, `expected`, `zip_list` and the
 * demonstration sender, and it cannot work for a P2300 sender: two senders
 * that compute the same value are unrelated types with no equality, and the
 * only way to ask what a sender computes is to run it.
 *
 * So the OBSERVATION is a parameter and the LAWS are not. A context that
 * compares gets `observe_directly` and behaves exactly as before; a sender
 * gets an observer that runs it. What must not happen -- and what having one
 * parameter here prevents -- is a second copy of the four equations written
 * in sender vocabulary, which would be a suite that can drift from the one
 * `optional` is checked against. See
 * docs/transpose-execution-plan.md#sender-registration deliverable 3.
 *
 * An observer is called with an rvalue, so it may consume what it is given;
 * the helpers copy where they must to supply one.
 */
struct observe_directly {
    template <class CONTEXT>
    constexpr auto operator()(CONTEXT &&context) const -> CONTEXT && {
        return std::forward<CONTEXT>(context);
    }
};

/** Verify the Applicative identity law, invoke form: `map(id, v) == v`.
 * Holds for every context, including those that cannot hold callables. */
template <class CONTEXT, class OBSERVE = observe_directly>
auto check_applicative_identity_law(const CONTEXT &value,
                                    const OBSERVE &observe = {}) -> bool {
    const auto &applicative =
        applicative_typeclass<std::remove_cvref_t<CONTEXT>>;
    auto result = applicative.map([](const auto &x) { return x; }, value);
    return observe(std::move(result)) == observe(auto(value));
}

/** Verify the Applicative homomorphism law, generalized to the n-ary core:
 * `invoke(f, pure(x1), ..., pure(xn)) == pure(f(x1, ..., xn))`. */
template <class CONTEXT, class OBSERVE = observe_directly, class FUNCTION,
          class... VALUES>
auto check_applicative_homomorphism_law(const FUNCTION &function,
                                        const VALUES &...values) -> bool {
    const auto &applicative =
        applicative_typeclass<std::remove_cvref_t<CONTEXT>>;
    auto left = applicative.invoke(function, applicative.pure(values)...);
    auto right = applicative.pure(std::invoke(function, values...));
    return OBSERVE{}(std::move(left)) == OBSERVE{}(std::move(right));
}

/** Verify the Applicative interchange law, invoke form:
 * `invoke(eval, u, pure(y)) == map(f -> f(y), u)` -- floating a pure
 * operand out of an application. Instantiable only for contexts whose
 * *elements* are callables (a plain vector-of-lambdas is fine); no apply
 * verb is involved. */
template <class FUNCTIONS_IN_CONTEXT, class VALUE,
          class OBSERVE = observe_directly>
auto check_applicative_interchange_law(const FUNCTIONS_IN_CONTEXT &functions,
                                       const VALUE &value,
                                       const OBSERVE &observe = {}) -> bool {
    const auto &applicative =
        applicative_typeclass<std::remove_cvref_t<FUNCTIONS_IN_CONTEXT>>;
    auto call = [](const auto &f, const auto &x) { return std::invoke(f, x); };
    auto left = applicative.invoke(call, functions, applicative.pure(value));
    auto right = applicative.map(
        [&value](const auto &f) { return std::invoke(f, value); }, functions);
    return observe(std::move(left)) == observe(std::move(right));
}

/** Verify the functor composition law: `map(f . g, v) == map(f, map(g, v))`
 * -- the composition analog expressible for every applicative context. */
template <class CONTEXT, class F, class G, class OBSERVE = observe_directly>
auto check_functor_composition_law(const F &outer, const G &inner,
                                   const CONTEXT &value,
                                   const OBSERVE &observe = {}) -> bool {
    const auto &applicative =
        applicative_typeclass<std::remove_cvref_t<CONTEXT>>;
    auto left = applicative.map(
        [&](const auto &x) {
            return std::invoke(outer, std::invoke(inner, x));
        },
        value);
    auto right = applicative.map(outer, applicative.map(inner, value));
    return observe(std::move(left)) == observe(std::move(right));
}

/** Copy-constructible, and deliberately nothing more.
 *
 * The operations that build results -- `pure`, `invoke`, the array and lane
 * constructions -- document only that each result element is constructible
 * from its own result. A scalar fixture satisfies far more than that, so a
 * suite built from scalars cannot tell a documented requirement from one an
 * implementation reached for by accident. This type has no default
 * constructor and no assignment operator, so any operation that needs either
 * fails to compile against it, and the requirement cannot creep back in
 * unnoticed.
 */
struct copyable_only {
    int value;

    explicit constexpr copyable_only(int initial) : value(initial) {}
    constexpr copyable_only(const copyable_only &) = default;
    constexpr copyable_only(copyable_only &&) = default;
    auto operator=(const copyable_only &) -> copyable_only & = delete;
    auto operator=(copyable_only &&) -> copyable_only & = delete;
    ~copyable_only() = default;

    friend constexpr auto operator==(const copyable_only &left,
                                     const copyable_only &right) -> bool {
        return left.value == right.value;
    }
};

static_assert(std::is_copy_constructible_v<copyable_only>);
static_assert(!std::is_default_constructible_v<copyable_only>);
static_assert(!std::is_copy_assignable_v<copyable_only>);
static_assert(!std::is_move_assignable_v<copyable_only>);

/** Counts its own copies and moves, so a test can tell linear construction
 * from quadratic.
 *
 * Comparing values proves a traversal computes the right answer and says
 * nothing about what it cost to get there. A traversal that rebuilds its
 * whole accumulated prefix at every step returns exactly the same vector as
 * one that appends to it. The only way to pin the difference is to count,
 * and the only way to read a count is against a second, larger input: an
 * absolute bound alone cannot distinguish a linear implementation with a
 * large constant from a quadratic one.
 */
struct counted {
    static inline long copies = 0;
    static inline long moves = 0;

    int value;

    explicit counted(int initial) : value(initial) {}
    counted(const counted &other) : value(other.value) { ++copies; }
    counted(counted &&other) noexcept : value(other.value) { ++moves; }

    auto operator=(const counted &other) -> counted & {
        value = other.value;
        ++copies;
        return *this;
    }
    auto operator=(counted &&other) noexcept -> counted & {
        value = other.value;
        ++moves;
        return *this;
    }
    ~counted() = default;

    static void reset() {
        copies = 0;
        moves = 0;
    }

    friend auto operator==(const counted &, const counted &) -> bool = default;
};

/** Invocable only through an lvalue.
 *
 * A traversal invokes a named `function` variable once per element, and a
 * named variable is an lvalue whether the caller passed a temporary or not.
 * This callable is therefore usable everywhere the library invokes one --
 * but a type computation that spells `invoke_result_t<F, ...>` for a deduced
 * `F&&` tests rvalue invocation instead, and rejects it.
 */
struct lvalue_only_callable {
    int bias;

    auto operator()(int element) & -> std::optional<int> {
        return std::optional<int>{element + bias};
    }
};

/** Invocable only through an rvalue: the mirror-image witness.
 *
 * Nothing in the library ever invokes a callable in this category, so this
 * one must be rejected. Detecting with `invoke_result_t<F, ...>` accepts it
 * for a caller who passes a temporary, and then fails inside the loop --
 * a diagnostic from the middle of an instantiation rather than a constraint
 * that simply does not match.
 */
struct rvalue_only_callable {
    int bias;

    auto operator()(int element) && -> std::optional<int> {
        return std::optional<int>{element + bias};
    }
};

/** Invocable only with an rvalue argument.
 *
 * The mirror image of `rvalue_only_callable`, which is about the category
 * of the callable; this one is about the category of the element. A
 * traversable object passes elements on in the category it received the
 * structure in, so this callable is usable for a traversal of an rvalue
 * structure and for no other. An object that accepts an rvalue structure
 * and then hands its elements on as `const` lvalues does not match it,
 * which is what makes the obligation checkable rather than promised.
 */
struct rvalue_argument_only_callable {
    int bias;

    auto operator()(int &&element) const -> std::optional<int> {
        return std::optional<int>{element + bias};
    }
};

/** Minimal single-element applicative context used in law tests. */
template <class VALUE_TYPE>
struct Identity {
    using value_type = VALUE_TYPE;

    VALUE_TYPE value;

    friend auto operator==(const Identity &, const Identity &)
        -> bool = default;
};

/** Ordered multi-element foldable context backed by `std::vector`. */
template <class VALUE_TYPE>
struct Sequence {
    using value_type = VALUE_TYPE;

    std::vector<VALUE_TYPE> values;

    friend auto operator==(const Sequence &, const Sequence &)
        -> bool = default;
};

} // namespace beman::transpose::test

namespace beman::transpose {

/** Applicative implementation for Identity<V>: pure wraps, invoke unwraps
 * every operand and calls the plain function once. */
template <class VALUE_TYPE>
struct TestIdentityApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        return test::Identity<std::remove_cvref_t<VALUE>>{
            std::forward<VALUE>(value)};
    }

    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function,
                const test::Identity<FIRST> &first,
                const test::Identity<REST> &...rest)
        -> test::Identity<std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        return test::Identity<Result>{
            std::invoke(function, first.value, rest.value...)};
    }
};

template <class VALUE_TYPE>
struct TestIdentityApplicativeMap
    : Applicative<TestIdentityApplicativeImpl<VALUE_TYPE>> {
    using TestIdentityApplicativeImpl<VALUE_TYPE>::invoke;
    using TestIdentityApplicativeImpl<VALUE_TYPE>::pure;
};

template <class VALUE_TYPE>
inline constexpr auto applicative_typeclass<test::Identity<VALUE_TYPE>> =
    TestIdentityApplicativeMap<VALUE_TYPE>{};

/** Foldable implementation for Sequence<V>: fold_map walks values
 * left-to-right. */
template <class VALUE_TYPE>
struct TestSequenceFoldableImpl {
    template <class FUNCTION>
    auto fold_map(this auto &&, FUNCTION &&function,
                  const test::Sequence<VALUE_TYPE> &sequence) {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION, const VALUE_TYPE &>>;
        return std::ranges::fold_left(
            sequence.values, monoid_identity<Result>(),
            [&](Result acc, const VALUE_TYPE &value) {
                return monoid_combine(std::move(acc),
                                      std::invoke(function, value));
            });
    }
};

template <class VALUE_TYPE>
struct TestSequenceFoldableMap
    : Foldable<TestSequenceFoldableImpl<VALUE_TYPE>> {
    using TestSequenceFoldableImpl<VALUE_TYPE>::fold_map;
};

template <class VALUE_TYPE>
inline constexpr auto foldable_typeclass<test::Sequence<VALUE_TYPE>> =
    TestSequenceFoldableMap<VALUE_TYPE>{};

/** Traversable implementation for Identity<V>.
 *
 * Two overloads, because a traversable object passes elements on in the
 * category it received the structure in. Identity owns its value outright,
 * so the consuming overload really does hand it over. The constraints are
 * what let an availability probe answer rather than diagnose: the return
 * types are deduced, so a callable an overload cannot invoke would
 * otherwise reach its body.
 */
template <class VALUE_TYPE>
struct TestIdentityTraversableImpl {
    using element_type = VALUE_TYPE;

    // Identity owns its single value, so the consuming overload hands it
    // over rather than copying it into a temporary first. It is the second
    // consuming instance in the suite, so the declaration is exercised on
    // something other than vector.
    static constexpr bool consumes_rvalue_structure = true;

    template <class APPLICATIVE, class FUNCTION>
        requires std::invocable<FUNCTION &, const VALUE_TYPE &>
    auto traverse(this auto &&, const APPLICATIVE &applicative,
                  FUNCTION &&function,
                  const test::Identity<VALUE_TYPE> &identity) {
        return applicative.invoke(
            [](auto &&value) {
                using U = std::remove_cvref_t<decltype(value)>;
                return test::Identity<U>{std::forward<decltype(value)>(value)};
            },
            std::invoke(std::forward<FUNCTION>(function), identity.value));
    }

    template <class APPLICATIVE, class FUNCTION>
        requires std::invocable<FUNCTION &, VALUE_TYPE &&>
    auto traverse(this auto &&, const APPLICATIVE &applicative,
                  FUNCTION &&function, test::Identity<VALUE_TYPE> &&identity) {
        return applicative.invoke(
            [](auto &&value) {
                using U = std::remove_cvref_t<decltype(value)>;
                return test::Identity<U>{std::forward<decltype(value)>(value)};
            },
            std::invoke(std::forward<FUNCTION>(function),
                        std::move(identity.value)));
    }
};

template <class VALUE_TYPE>
struct TestIdentityTraversableMap
    : Traversable<TestIdentityTraversableImpl<VALUE_TYPE>> {
    using TestIdentityTraversableImpl<VALUE_TYPE>::traverse;
};

template <class VALUE_TYPE>
inline constexpr auto traversable_typeclass<test::Identity<VALUE_TYPE>> =
    TestIdentityTraversableMap<VALUE_TYPE>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_TEST_TEST_SUPPORT_HPP
