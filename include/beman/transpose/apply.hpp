// include/beman/transpose/apply.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_APPLY_HPP
#define BEMAN_TRANSPOSE_APPLY_HPP

#include <beman/transpose/detail/typeclass_base.hpp>

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

// Applicative pattern invariants:
// - Single core: the minimal required operations are pure + invoke.
//   invoke -- a plain function applied to N arguments each in the context --
//   is McBride & Paterson's canonical form
//     pure f <*> u1 <*> ... <*> un   ==   invoke(f, u1, ..., un).
// - There is no `apply` (a callable held *inside* the context, applied to
//   one argument-in-context at a time) anywhere in this library. That
//   formulation is an artifact of Haskell's currying, not a primitive:
//   std::simd::vec cannot even form a vec<callable>, so apply is
//   unspellable exactly where this proposal's motivation lives, while
//   n-ary invoke works everywhere. Removed 2026-07-14; do not reintroduce
//   apply-shaped API in new instances.
// - Instances implement invoke directly n-ary; no currying helpers exist.
// - Impls keep invoke SFINAE-friendly: a trailing return type via
//   std::invoke_result_t (or an explicit requires clause), never a bare
//   auto return with the result type computed only in the body, so that
//   availability probes fail cleanly instead of hard-erroring.
// - Derived operations (map/lift/zip_with/discard_*) live on that object.
// - Dispatch happens through a provided object or
//   applicative_typeclass<Concrete>.
// - Do not introduce hidden alternate semantics without a distinct map/type.

/** CRTP base for Applicative instances.
 * `Impl` must provide `pure(value)` and the n-ary
 * `invoke(f, args_in_context...)`; every other operation is derived.
 */
template <class Impl>
struct Applicative : protected Impl {
    static_assert(
        !std::is_same_v<Impl, std::false_type>,
        "No applicative_typeclass<T> specialization found. "
        "Specialize beman::transpose::applicative_typeclass<T> for "
        "your type T and provide pure(...) and invoke(...).");
    using Impl::pure;

    /** Lifts `function` into the applicative and applies it to one or more
     * effectful arguments left-to-right, producing a single effectful result.
     * @param function  A plain callable applied to the unwrapped operands.
     * @param first_argument  First effectful argument (e.g., optional or
     * vector).
     * @param rest_arguments  Additional effectful arguments, if any.
     */
    template <class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS>
    auto invoke(this auto &&self, FUNCTION &&function,
                FIRST_ARGUMENT &&first_argument,
                REST_ARGUMENTS &&...rest_arguments) {
        using SELF = std::remove_reference_t<decltype(self)>;
        using IMPL_BASE =
            std::conditional_t<std::is_const_v<SELF>, const Impl, Impl>;
        static_assert(
            requires(IMPL_BASE &impl) {
                impl.invoke(std::forward<FUNCTION>(function),
                            std::forward<FIRST_ARGUMENT>(first_argument),
                            std::forward<REST_ARGUMENTS>(rest_arguments)...);
            },
            "Applicative Impl must provide pure(...) and the n-ary "
            "invoke(f, args_in_context...).");
        return static_cast<IMPL_BASE &>(self).invoke(
            std::forward<FUNCTION>(function),
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }

    /** Single-argument fmap via invoke; applies `function` to one effectful
     * arg. */
    template <class FUNCTION, class ARGUMENT>
    auto map(this auto &&self, FUNCTION &&function, ARGUMENT &&argument) {
        return self.invoke(std::forward<FUNCTION>(function),
                           std::forward<ARGUMENT>(argument));
    }

    /** Alias for `pure`; embeds a plain value into the applicative context. */
    template <class VALUE>
    auto lift(this auto &&self, VALUE &&value) {
        return self.pure(std::forward<VALUE>(value));
    }

    /** Lifts a binary function and applies it to two effectful arguments. */
    template <class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto zip_with(this auto &&self, FUNCTION &&function,
                  FIRST_ARGUMENT &&first_argument,
                  SECOND_ARGUMENT &&second_argument) {
        return self.invoke(std::forward<FUNCTION>(function),
                           std::forward<FIRST_ARGUMENT>(first_argument),
                           std::forward<SECOND_ARGUMENT>(second_argument));
    }

    /** Sequences two effectful values; returns the second, ignoring the first.
     */
    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto discard_first(this auto &&self, FIRST_ARGUMENT &&first_argument,
                       SECOND_ARGUMENT &&second_argument) {
        return self.invoke(
            [](const auto &, auto &&rhs) {
                return std::forward<decltype(rhs)>(rhs);
            },
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    }

    /** Sequences two effectful values; returns the first, ignoring the second.
     */
    template <class FIRST_ARGUMENT, class SECOND_ARGUMENT>
    auto discard_second(this auto &&self, FIRST_ARGUMENT &&first_argument,
                        SECOND_ARGUMENT &&second_argument) {
        return self.invoke(
            [](auto &&lhs, const auto &) {
                return std::forward<decltype(lhs)>(lhs);
            },
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<SECOND_ARGUMENT>(second_argument));
    }

    /** Delegates invoke to a different applicative instance at runtime. */
    template <class APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
              class... REST_ARGUMENTS>
    auto invoke_with(this auto &&, const APPLICATIVE_MAP &applicative_map,
                     FUNCTION &&function, FIRST_ARGUMENT &&first_argument,
                     REST_ARGUMENTS &&...rest_arguments) {
        return applicative_map.invoke(
            std::forward<FUNCTION>(function),
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }

    /** Delegates invoke to a compile-time constant applicative instance. */
    template <const auto &APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
              class... REST_ARGUMENTS>
    auto invoke_with(this auto &&, FUNCTION &&function,
                     FIRST_ARGUMENT &&first_argument,
                     REST_ARGUMENTS &&...rest_arguments) {
        return APPLICATIVE_MAP.invoke(
            std::forward<FUNCTION>(function),
            std::forward<FIRST_ARGUMENT>(first_argument),
            std::forward<REST_ARGUMENTS>(rest_arguments)...);
    }
};

/** Typeclass lookup variable for Applicative; specialize for each type. */
template <class T>
inline constexpr auto applicative_typeclass = std::false_type{};

/** Applicative instance for std::optional: the flagship of the invoke core.
 * The trailing return type keeps invoke SFINAE-friendly so availability
 * probes fail cleanly.
 */
template <class VALUE_TYPE>
struct OptionalApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::optional<remove_cvref_t<VALUE>> {
        return std::optional<remove_cvref_t<VALUE>>{std::forward<VALUE>(value)};
    }

    /** N-ary core: if every operand is engaged, one call; otherwise empty. */
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function,
                const std::optional<FIRST> &first,
                const std::optional<REST> &...rest)
        -> std::optional<remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>> {
        using Result = remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const FIRST &, const REST &...>>;
        if (first.has_value() && (... && rest.has_value())) {
            return std::optional<Result>{
                std::invoke(function, *first, *rest...)};
        }
        return std::optional<Result>{};
    }
};

template <class VALUE_TYPE>
struct OptionalApplicativeMap
    : Applicative<OptionalApplicativeImpl<VALUE_TYPE>> {
    using OptionalApplicativeImpl<VALUE_TYPE>::invoke;
    using OptionalApplicativeImpl<VALUE_TYPE>::pure;
};

/** Applicative instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto applicative_typeclass<std::optional<VALUE_TYPE>> =
    OptionalApplicativeMap<VALUE_TYPE>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_APPLY_HPP
