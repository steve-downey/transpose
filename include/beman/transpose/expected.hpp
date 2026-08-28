// include/beman/transpose/expected.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXPECTED_HPP
#define BEMAN_TRANSPOSE_EXPECTED_HPP

// Applicative and Monad instances for std::expected<T, E>.
//
// UNGRADED BY CONSTRUCTION. Each instance object pins exactly one error type:
// invoke accepts operands that all carry that same E, and bind accepts a
// continuation that returns that same E. Combining expected<T,E1> with
// expected<T,E2> is ill-formed here, and stays ill-formed until stage
// graded-deduction opens that territory deliberately by joining the two error
// types into a grade. This is the before-state the additive-compatibility
// claim of docs/decisions.md#grading-footprint is measured against: unmixed
// pipelines deduce expected<T,E> today, and must still deduce exactly
// expected<T,E> -- never expected<T, error_set<E>> -- after grading lands.
// See docs/decisions.md#expected-instance-introduction.
//
// Short-circuit semantics, first error wins, operands examined left to right.
// That order is observable and therefore part of the contract, per the
// Traversable rules in docs/CODING_RULES.md and the normative left-to-right
// order of docs/decisions.md#applicative-objects.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
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
