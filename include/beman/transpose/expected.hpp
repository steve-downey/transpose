// include/beman/transpose/expected.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXPECTED_HPP
#define BEMAN_TRANSPOSE_EXPECTED_HPP

// Applicative and Monad instances for std::expected<T, E>.
//
// THREE CORES, MUTUALLY EXCLUSIVE BY CONSTRAINT.
//
// The same-error core handles operands that all declare this instance's error
// type, and deduces exactly what it always did: expected<T,E>, with no
// error_set in the carrier spelling. The same-error-plus-bare core keeps that
// spelling when ungraded operands participate. The graded core handles real
// mixing -- operands declaring different model grades -- and deduces their
// join. The constraints are complements, so the choice is never a matter of
// overload ranking (docs/decisions.md#grading-footprint).
//
// Every combination the graded core accepts was ILL-FORMED before grading:
// there was no instance that would take operands with differing error types.
// So grading claims only previously-ill-formed territory, and nothing that
// deduced a type before deduces a different one now. The golden tests in
// tests/beman/transpose/baseline_deduction.test.cpp are where that claim is
// mechanized rather than argued.
//
// The CARRIER SPELLING is lazy. `grade_of<std::expected<T,E>>` is the
// singleton grade `error_set<E>`, but operands that agree on an alternative
// carry that alternative verbatim, so unmixed pipelines never acquire a
// singleton error_set in their deduced type. A bare operand is lifted to the
// model bottom at a mixing point and leaves no trace in the carrier spelling.
// An error_set only appears where genuinely different alternatives met.
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

/** True when every expected operand declares exactly ERROR_TYPE and every
 * other operand is bare. This is not a mixing point: lazy join says bare
 * values contribute the model bottom and leave no trace.
 */
template <class ERROR_TYPE, class CARRIER>
inline constexpr bool declares_or_bare_v =
    !is_expected_v<remove_cvref_t<CARRIER>> ||
    is_expected_with_error_v<remove_cvref_t<CARRIER>, ERROR_TYPE>;

template <class ERROR_TYPE, class... CARRIERS>
inline constexpr bool all_declare_or_bare_v =
    (declares_or_bare_v<ERROR_TYPE, CARRIERS> && ...);

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

    /** N-ary core: expected operands share ERROR_TYPE, with any number of
     * bare operands. This is not a mixing point: the bare operands lift to
     * the model bottom and leave no trace in the deduced type.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (detail::is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
                detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> std::expected<
            remove_cvref_t<std::invoke_result_t<
                FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>,
            ERROR_TYPE> {
        using Result = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>;
        using Returned = std::expected<Result, ERROR_TYPE>;

        std::optional<ERROR_TYPE> failure;
        auto record_first_failure = [&failure](const auto &operand) {
            if constexpr (detail::is_expected_v<
                              remove_cvref_t<decltype(operand)>>) {
                if (!failure.has_value() && !operand.has_value()) {
                    failure = operand.error();
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

    /** Graded n-ary core: the mixing point.
     *
     * Reached exactly when the operands do NOT all declare this instance's
     * error type -- the ungraded core above handles that case, and the two
     * constraints are complementary, so the choice is never a matter of
     * overload ranking (docs/decisions.md#grading-footprint).
     *
     * The result carries the join of the operands' semantic grades. Carrier
     * spelling remains lazy: identical alternatives are carried verbatim,
     * only genuinely different ones become an error_set, and bare operands
     * lift to the model bottom without leaving a trace in the deduced type.
     *
     * This is the ONLY behavioural change grading makes to existing code, and
     * it makes previously-ill-formed combinations well-formed. Nothing that
     * deduced a type before deduces a different one now.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (detail::is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
                (detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     CARRIERS> &&
                 ...)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<
                remove_cvref_t<std::invoke_result_t<
                    FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>,
                ERROR_TYPE>,
            CARRIERS...> {
        using Result = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>;
        using Returned = detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<Result, ERROR_TYPE>, CARRIERS...>;
        using Joined = typename Returned::error_type;

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

// -- The accumulating applicative instance ---------------------------------
//
// The second NTTP-pinned object over the SAME carrier and grade algebra as
// ExpectedApplicativeMap above, per docs/decisions.md#applicative-objects.
// Stage accumulating-object of docs/transpose-grading-plan.md.
//
// Where the short-circuit object above keeps only the LEFTMOST failing
// operand's witness, this object COMBINES every failing operand's witness
// through the grade model's evidence-combine hook. Same-type collisions keep
// the leftmost witness; model evidence with multiple slots can preserve
// failures of genuinely different types. That is the whole difference:
// examining every operand instead of stopping at the first.
//
// Structurally this mirrors ExpectedApplicativeImpl's ungraded/graded split
// exactly -- same two constraints, same complementary coverage, same
// bare-operand promotion. Only the failure-collecting step differs.

namespace detail {

/** Folds model evidence over every failing operand's error, left to right.
 * `EXTRACT` maps an operand to `std::optional<ERROR>` -- present exactly
 * when that operand failed -- so this one loop serves both the ungraded core
 * and the graded core.
 */
template <class ERROR, class EXTRACT, class... OPERANDS>
constexpr auto accumulate_failures(EXTRACT &&extract,
                                   const OPERANDS &...operands)
    -> std::optional<ERROR> {
    std::optional<ERROR> accumulated;
    auto fold_in = [&accumulated, &extract](const auto &operand) {
        auto this_one = extract(operand);
        if (!this_one.has_value()) {
            return;
        }
        if (accumulated.has_value()) {
            accumulated = combine_grade_evidence(*accumulated, *this_one);
        } else {
            accumulated = std::move(this_one);
        }
    };
    (fold_in(operands), ...);
    return accumulated;
}

} // namespace detail

/** Accumulating applicative instance for std::expected<VALUE_TYPE,
 * ERROR_TYPE>. Same carrier as ExpectedApplicativeImpl; see the section
 * comment above for how the two differ.
 */
template <class VALUE_TYPE, class ERROR_TYPE>
struct AccumulatingExpectedApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::expected<remove_cvref_t<VALUE>, ERROR_TYPE> {
        return std::expected<remove_cvref_t<VALUE>, ERROR_TYPE>{
            std::forward<VALUE>(value)};
    }

    /** N-ary core: all operands share ERROR_TYPE. When ERROR_TYPE is itself
     * a witnessed subset (the traverse case: one function, one declared
     * error, possibly several members), failures of distinct members all
     * survive; otherwise there is one slot and this degrades to
     * leftmost-wins, same as the short-circuit object.
     */
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

        auto extract_failure =
            [](const auto &operand) -> std::optional<ERROR_TYPE> {
            if (operand.has_value()) {
                return std::nullopt;
            }
            return operand.error();
        };
        auto accumulated = detail::accumulate_failures<ERROR_TYPE>(
            extract_failure, first, rest...);

        if (accumulated.has_value()) {
            return Returned{std::unexpect, std::move(*accumulated)};
        }
        return Returned{std::invoke(function, *first, *rest...)};
    }

    /** N-ary core: expected operands share ERROR_TYPE, with any number of
     * bare operands. This is not a mixing point, so the result carries the
     * declared error alternative verbatim.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (detail::is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_v<ERROR_TYPE, CARRIERS...>) &&
                detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> std::expected<
            remove_cvref_t<std::invoke_result_t<
                FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>,
            ERROR_TYPE> {
        using Result = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>;
        using Returned = std::expected<Result, ERROR_TYPE>;

        auto extract_failure =
            [](const auto &operand) -> std::optional<ERROR_TYPE> {
            if constexpr (detail::is_expected_v<
                              remove_cvref_t<decltype(operand)>>) {
                if (!operand.has_value()) {
                    return operand.error();
                }
            }
            return std::nullopt;
        };
        auto accumulated = detail::accumulate_failures<ERROR_TYPE>(
            extract_failure, operands...);

        if (accumulated.has_value()) {
            return Returned{std::unexpect, std::move(*accumulated)};
        }
        return Returned{
            std::invoke(function, detail::operand_value(operands)...)};
    }

    /** Graded n-ary core: the mixing point, mirroring
     * ExpectedApplicativeImpl's, but accumulating every failing operand's
     * witness into the joined grade instead of keeping only the first.
     */
    template <class FUNCTION, class... CARRIERS>
        requires(sizeof...(CARRIERS) > 0) &&
                (detail::is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
                (!detail::all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
                (detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     CARRIERS> &&
                 ...)
    auto invoke(this auto &&, FUNCTION &&function, const CARRIERS &...operands)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<
                remove_cvref_t<std::invoke_result_t<
                    FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>,
                ERROR_TYPE>,
            CARRIERS...> {
        using Result = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, const detail::carrier_value_t<CARRIERS> &...>>;
        using Returned = detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<Result, ERROR_TYPE>, CARRIERS...>;
        using Joined = typename Returned::error_type;

        auto extract_failure =
            [](const auto &operand) -> std::optional<Joined> {
            if constexpr (detail::is_expected_v<
                              remove_cvref_t<decltype(operand)>>) {
                if (detail::operand_failed(operand)) {
                    return Joined(operand.error());
                }
            }
            return std::nullopt;
        };
        auto accumulated =
            detail::accumulate_failures<Joined>(extract_failure, operands...);

        if (accumulated.has_value()) {
            return Returned{std::unexpect, std::move(*accumulated)};
        }
        return Returned{
            std::invoke(function, detail::operand_value(operands)...)};
    }
};

template <class VALUE_TYPE, class ERROR_TYPE>
struct AccumulatingExpectedApplicativeMap
    : Applicative<AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>> {
    using AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::invoke;
    using AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::pure;

    /** Bind is refused, not merely absent, per
     * docs/decisions.md#applicative-objects: sequencing needs a value from
     * the first computation to feed the continuation, and when that
     * computation failed there is none -- a value-flow obstruction, not
     * something grade bookkeeping can fix. The framework never derives bind
     * for this object. `always_false_v` defers the static_assert to the
     * point of an actual call, so merely naming this type (as
     * `accumulating_applicative_typeclass<T>` does) stays well-formed; only
     * calling `.bind(...)` on it fails, with this message.
     */
    template <class... ARGS>
    constexpr auto bind(ARGS &&...) const {
        static_assert(
            always_false_v<ARGS...>,
            "The accumulating applicative object has no bind: sequencing "
            "needs a value from the first computation to feed the "
            "continuation, and there is none when that computation failed "
            "-- a value-flow obstruction, not something grade bookkeeping "
            "can fix. Use the short-circuit (monad-derived) applicative "
            "object (applicative_typeclass) for sequential composition. "
            "See docs/decisions.md#applicative-objects.");
    }
};

/** Accumulating applicative instance for `std::expected<VALUE_TYPE,
 * ERROR_TYPE>`. */
template <class VALUE_TYPE, class ERROR_TYPE>
inline constexpr auto
    accumulating_applicative_typeclass<std::expected<VALUE_TYPE, ERROR_TYPE>> =
        AccumulatingExpectedApplicativeMap<VALUE_TYPE, ERROR_TYPE>{};

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
                                                    ERROR_TYPE>) &&
                 detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     std::expected<A, ERROR_TYPE>> &&
                 detail::mixes_with_model<
                     grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
                     detail::bind_result_t<F, A>>
    auto bind(this auto &&, const std::expected<A, ERROR_TYPE> &ma, F &&f)
        -> detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<typename detail::bind_result_t<F, A>::value_type,
                          ERROR_TYPE>,
            std::expected<A, ERROR_TYPE>, detail::bind_result_t<F, A>> {
        using Continuation = detail::bind_result_t<F, A>;
        using Returned = detail::mixed_result_t<
            grade_of_t<std::expected<VALUE_TYPE, ERROR_TYPE>>,
            std::expected<typename Continuation::value_type, ERROR_TYPE>,
            std::expected<A, ERROR_TYPE>, Continuation>;
        using Joined = typename Returned::error_type;

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
