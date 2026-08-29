// include/beman/transpose/expected.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXPECTED_HPP
#define BEMAN_TRANSPOSE_EXPECTED_HPP

// Applicative and Monad instances for std::expected<T, E>.
//
// TWO CORES, MUTUALLY EXCLUSIVE BY CONSTRAINT.
//
// The ungraded core handles operands that all declare this instance's error
// type, and deduces exactly what it always did: expected<T,E>, with no
// error_set anywhere. The graded core handles everything else -- operands
// declaring different error alternatives -- and deduces their join. The two
// constraints are complements, so the choice is never a matter of overload
// ranking (docs/decisions.md#grading-footprint).
//
// Every combination the graded core accepts was ILL-FORMED before grading:
// there was no instance that would take operands with differing error types.
// So grading claims only previously-ill-formed territory, and nothing that
// deduced a type before deduces a different one now. The golden tests in
// tests/beman/transpose/baseline_deduction.test.cpp are where that claim is
// mechanized rather than argued.
//
// The join is LAZY. Operands that agree on an alternative carry it verbatim,
// so unmixed pipelines never acquire a singleton error_set, and a bare
// operand is ∅-graded, promoted inside the framework, and leaves no trace in
// the deduced type. An error_set only ever appears where two genuinely
// different alternatives met.
//
// Short-circuit semantics, first error wins, operands examined left to right.
// That order is observable and therefore part of the contract, per the
// Traversable rules in docs/CODING_RULES.md and the normative left-to-right
// order of docs/decisions.md#applicative-objects. Note that the grade is
// order-blind while the observed error is not: ∪ commutes, so operand order
// cannot change the deduced type, only which error you get back.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/error_set.hpp>
#include <beman/transpose/grade.hpp>
#include <beman/transpose/monad.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

namespace detail {

/** True when EXPECTED is exactly std::expected<T, ERROR_TYPE> for some T.
 * Used to keep the bind basis SFINAE-friendly: a continuation returning a
 * different error type must fail the constraint cleanly rather than
 * hard-error inside the body when the error is rewrapped.
 */
template <class EXPECTED, class ERROR_TYPE>
inline constexpr bool is_expected_with_error_v = false;

template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr bool is_expected_with_error_v<
    std::expected<VALUE_TYPE, ERROR_TYPE>, ERROR_TYPE> = true;

// -- Joining error alternatives at a mixing point -------------------------
//
// LAZY JOIN, per docs/decisions.md#grading-footprint: an error_set appears
// only at a genuine mixing point. If every operand that carries an error
// alternative declares the SAME one, the result carries that alternative
// verbatim -- bare error type or error_set alike, unchanged. Only when two
// operands declare DIFFERENT alternatives are those alternatives flattened
// and unioned into an error_set.
//
// Operands with no error alternative -- bare values -- are ∅-graded and
// contribute nothing to the join. That is what keeps
// `invoke(f, expected<T,E>, 5)` deducing `expected<R,E>` rather than
// `expected<R, error_set<E>>`: the bare side is promoted inside the
// framework and leaves no trace in the deduced type.

/** The elements of an error alternative. A bare error type is the singleton
 * containing it; an error_set is its own elements. */
template <class ERROR>
struct error_elements {
    using type = type_list<ERROR>;
};

template <class... ERRORS>
struct error_elements<error_set_of<ERRORS...>> {
    using type = type_list<ERRORS...>;
};

template <class... LISTS>
struct list_concat;

template <>
struct list_concat<> {
    using type = type_list<>;
};

template <class... ERRORS>
struct list_concat<type_list<ERRORS...>> {
    using type = type_list<ERRORS...>;
};

template <class... LEFT, class... RIGHT, class... REST>
struct list_concat<type_list<LEFT...>, type_list<RIGHT...>, REST...>
    : list_concat<type_list<LEFT..., RIGHT...>, REST...> {};

/** Build a canonical error_set from a list of elements. */
template <class LIST>
struct error_set_of_elements;

template <class... ERRORS>
struct error_set_of_elements<type_list<ERRORS...>> {
    using type = error_set<ERRORS...>;
};

/** The error alternative an operand declares, if it declares one. */
template <class CARRIER>
struct declared_alternative {
    using type = type_list<>;
};

template <class VALUE, class ERROR>
struct declared_alternative<std::expected<VALUE, ERROR>> {
    using type = type_list<ERROR>;
};

template <class LIST>
struct combine_declared;

template <class FIRST, class... REST>
struct combine_declared<type_list<FIRST, REST...>> {
    using type = std::conditional_t<
        (std::is_same_v<FIRST, REST> && ...), FIRST,
        typename error_set_of_elements<typename list_concat<
            typename error_elements<FIRST>::type,
            typename error_elements<REST>::type...>::type>::type>;
};

template <class... CARRIERS>
using joined_error_t = typename combine_declared<typename list_concat<
    typename declared_alternative<remove_cvref_t<CARRIERS>>::type...>::type>::
    type;

/** The value type an operand contributes; a bare operand contributes itself.
 */
template <class CARRIER>
struct carrier_value {
    using type = CARRIER;
};

template <class VALUE, class ERROR>
struct carrier_value<std::expected<VALUE, ERROR>> {
    using type = VALUE;
};

template <class CARRIER>
using carrier_value_t = typename carrier_value<remove_cvref_t<CARRIER>>::type;

template <class F, class A>
using bind_result_t = remove_cvref_t<std::invoke_result_t<F, const A &>>;

/** True when every operand is an expected declaring exactly ERROR_TYPE, which
 * is precisely the case the ungraded core already handles. The graded core
 * negates this, so the two are mutually exclusive by constraint rather than
 * by ranking. */
template <class ERROR_TYPE, class... CARRIERS>
inline constexpr bool all_declare_v =
    (is_expected_with_error_v<remove_cvref_t<CARRIERS>, ERROR_TYPE> && ...);

/** The engaged value of an operand; a bare operand is already the value. */
template <class CARRIER>
constexpr auto operand_value(const CARRIER &operand) -> decltype(auto) {
    if constexpr (is_expected_v<remove_cvref_t<CARRIER>>) {
        return *operand;
    } else {
        return (operand);
    }
}

/** Whether an operand currently carries an error. A bare value never does:
 * ∅ has no failure alternative to be in. */
template <class CARRIER>
constexpr auto operand_failed(const CARRIER &operand) -> bool {
    if constexpr (is_expected_v<remove_cvref_t<CARRIER>>) {
        return !operand.has_value();
    } else {
        return false;
    }
}

} // namespace detail

/** Applicative instance for std::expected<VALUE_TYPE, ERROR_TYPE>.
 * The n-ary invoke core: if every operand holds a value, one call; otherwise
 * the leftmost error propagates. The trailing return type keeps invoke
 * SFINAE-friendly, so an operand carrying a different error type fails
 * deduction cleanly instead of hard-erroring.
 */
template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::expected<remove_cvref_t<VALUE>, ERROR_TYPE> {
        return std::expected<remove_cvref_t<VALUE>, ERROR_TYPE>{
            std::forward<VALUE>(value)};
    }

    /** N-ary core: all operands share ERROR_TYPE; leftmost error wins. */
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function,
                const std::expected<FIRST, ERROR_TYPE> &first,
                const std::expected<REST, ERROR_TYPE> &...rest)
        -> std::expected<remove_cvref_t<std::invoke_result_t<
                             FUNCTION &, const FIRST &, const REST &...>>,
                         ERROR_TYPE> {
        using Result = remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        using Returned = std::expected<Result, ERROR_TYPE>;

        std::optional<ERROR_TYPE> failure;
        auto record_first_failure = [&failure](const auto &operand) {
            if (!failure.has_value() && !operand.has_value()) {
                failure = operand.error();
            }
        };
        record_first_failure(first);
        (record_first_failure(rest), ...);

        if (failure.has_value()) {
            return Returned{std::unexpect, std::move(*failure)};
        }
        return Returned{std::invoke(function, *first, *rest...)};
    }

    /** Graded n-ary core: the mixing point.
     *
     * Reached exactly when the operands do NOT all declare this instance's
     * error type -- the ungraded core above handles that case, and the two
     * constraints are complementary, so the choice is never a matter of
     * overload ranking (docs/decisions.md#grading-footprint).
     *
     * The result carries the join of the declared alternatives. Joining is
     * lazy: identical alternatives are carried verbatim, and only genuinely
     * different ones become an error_set. Bare operands are ∅-graded, are
     * promoted here inside the framework, and leave no trace in the deduced
     * type.
     *
     * This is the ONLY behavioural change grading makes to existing code, and
     * it makes previously-ill-formed combinations well-formed. Nothing that
     * deduced a type before deduces a different one now.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (detail::is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> std::expected<
            remove_cvref_t<std::invoke_result_t<
                FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>,
            detail::joined_error_t<CARRIERS...>> {
        using Result = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>;
        using Joined = detail::joined_error_t<CARRIERS...>;
        using Returned = std::expected<Result, Joined>;

        std::optional<Joined> failure;
        auto record_first_failure = [&failure](const auto &operand) {
            if (!failure.has_value() && detail::operand_failed(operand)) {
                if constexpr (detail::is_expected_v<
                                  remove_cvref_t<decltype(operand)>>) {
                    failure.emplace(Joined(operand.error()));
                }
            }
        };
        (record_first_failure(operands), ...);

        if (failure.has_value()) {
            return Returned{std::unexpect, std::move(*failure)};
        }
        return Returned{
            std::invoke(function, detail::operand_value(operands)...)};
    }
};

template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedApplicativeMap
    : Applicative<ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>> {
    using ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::invoke;
    using ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};

/** Applicative instance for `std::expected<VALUE_TYPE, ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto
    applicative_typeclass<std::expected<VALUE_TYPE, ERROR_TYPE>> =
        ExpectedApplicativeMap<VALUE_TYPE, ERROR_TYPE>{};

/** Monad instance for std::expected<VALUE_TYPE, ERROR_TYPE>.
 * bind short-circuits on the error alternative. The continuation must return
 * an expected carrying the same ERROR_TYPE; the constraint says so directly
 * rather than letting the mismatch surface as a rewrapping failure in the
 * body.
 */
template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadImpl {
    using element_type = VALUE_TYPE;

    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::expected<remove_cvref_t<VALUE>, ERROR_TYPE> {
        return applicative_typeclass<std::expected<VALUE_TYPE, ERROR_TYPE>>.pure(
            std::forward<VALUE>(value));
    }

    template <class A, class F>
    auto bind(this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
        -> remove_cvref_t<std::invoke_result_t<F, const A &>>
        requires detail::is_expected_with_error_v<
            remove_cvref_t<std::invoke_result_t<F, const A &>>, ERROR_TYPE>
    {
        using Result = remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma.has_value()) {
            return Result{std::unexpect, ma.error()};
        }
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }

    /** Graded bind: sequencing joins grades.
     *
     * Reached when the continuation returns an expected declaring a DIFFERENT
     * error alternative; the same-alternative case is the ungraded bind
     * above, and the constraints are complementary. The result carries the
     * join, and each side's error is widened into it by the alternative's own
     * conversion.
     */
    template <class A, class F>
        requires detail::is_expected_v<detail::bind_result_t<F, A>> &&
                 (!detail::is_expected_with_error_v<detail::bind_result_t<F, A>,
                                                    ERROR_TYPE>)
    auto bind(this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
        -> std::expected<typename detail::bind_result_t<F, A>::value_type,
                         detail::joined_error_t<std::expected<A, ERROR_TYPE>,
                                                detail::bind_result_t<F, A>>> {
        using Continuation = detail::bind_result_t<F, A>;
        using Joined =
            detail::joined_error_t<std::expected<A, ERROR_TYPE>, Continuation>;
        using Returned =
            std::expected<typename Continuation::value_type, Joined>;

        if (!ma.has_value()) {
            return Returned{std::unexpect, Joined(ma.error())};
        }
        auto produced = std::invoke(std::forward<F>(f), *ma);
        if (!produced.has_value()) {
            return Returned{std::unexpect, Joined(produced.error())};
        }
        return Returned{std::move(*produced)};
    }
};

template <class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadMap : Monad<ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>> {
    using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::bind;
    using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};

/** Monad instance for `std::expected<VALUE_TYPE, ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto monad_typeclass<std::expected<VALUE_TYPE, ERROR_TYPE>> =
    ExpectedMonadMap<VALUE_TYPE, ERROR_TYPE>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_EXPECTED_HPP
