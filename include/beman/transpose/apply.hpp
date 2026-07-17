// include/beman/transpose/apply.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_APPLY_HPP
#define BEMAN_TRANSPOSE_APPLY_HPP

#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

namespace detail {

// Currying support for the invoke-from-ap derivation: wraps a plain n-ary
// function so it can be fed through one-step contextual application,
// consuming one argument at a time and invoking when saturated. This cost
// exists only in the derivation direction -- C++ is not natively curried --
// and never at invoke-native call sites.
template <class FUNCTION, class... BOUND_ARGS>
struct terminating_partial {
    FUNCTION function;
    std::tuple<BOUND_ARGS...> bound_args;

    template <class NEXT_ARG>
    auto operator()(NEXT_ARG &&next_arg) {
        return invoke_or_extend(std::forward<NEXT_ARG>(next_arg),
                                std::index_sequence_for<BOUND_ARGS...>{});
    }

    template <class NEXT_ARG>
    auto operator()(NEXT_ARG &&next_arg) const {
        return invoke_or_extend_const(std::forward<NEXT_ARG>(next_arg),
                                      std::index_sequence_for<BOUND_ARGS...>{});
    }

  private:
    template <class NEXT_ARG, std::size_t... IDX>
    auto invoke_or_extend(NEXT_ARG &&next_arg, std::index_sequence<IDX...>) {
        if constexpr (std::invocable<FUNCTION &, BOUND_ARGS &..., NEXT_ARG>) {
            return std::invoke(function, std::get<IDX>(bound_args)...,
                               std::forward<NEXT_ARG>(next_arg));
        } else {
            using NEXT_PARTIAL = terminating_partial<FUNCTION, BOUND_ARGS...,
                                                     remove_cvref_t<NEXT_ARG>>;
            return NEXT_PARTIAL{
                function,
                std::tuple_cat(std::move(bound_args),
                               std::tuple<remove_cvref_t<NEXT_ARG>>{
                                   std::forward<NEXT_ARG>(next_arg)})};
        }
    }

    template <class NEXT_ARG, std::size_t... IDX>
    auto invoke_or_extend_const(NEXT_ARG &&next_arg,
                                std::index_sequence<IDX...>) const {
        if constexpr (std::invocable<const FUNCTION &, const BOUND_ARGS &...,
                                     NEXT_ARG>) {
            return std::invoke(function, std::get<IDX>(bound_args)...,
                               std::forward<NEXT_ARG>(next_arg));
        } else {
            using NEXT_PARTIAL = terminating_partial<FUNCTION, BOUND_ARGS...,
                                                     remove_cvref_t<NEXT_ARG>>;
            return NEXT_PARTIAL{
                function,
                std::tuple_cat(bound_args,
                               std::tuple<remove_cvref_t<NEXT_ARG>>{
                                   std::forward<NEXT_ARG>(next_arg)})};
        }
    }
};

template <class FUNCTION>
auto make_terminating_partial(FUNCTION &&function) {
    using STORED_FUNCTION = remove_cvref_t<FUNCTION>;
    return terminating_partial<STORED_FUNCTION>{
        std::forward<FUNCTION>(function), std::tuple<>{}};
}

// The evaluator that recovers one-step application (ap) from the n-ary
// core: ap(cf, cx) = invoke(applicative_eval, cf, cx) -- GHC's
// (<*>) = liftA2 id. The trailing return type keeps it SFINAE-visible so
// the derived ap's constraint fails cleanly -- never hard-errors -- for
// contexts that cannot hold callables.
struct applicative_eval_t {
    template <class FUNCTION, class ARGUMENT>
    constexpr auto operator()(FUNCTION &&function, ARGUMENT &&argument) const
        -> std::invoke_result_t<FUNCTION, ARGUMENT> {
        return std::invoke(std::forward<FUNCTION>(function),
                           std::forward<ARGUMENT>(argument));
    }
};
inline constexpr applicative_eval_t applicative_eval{};

// Free-function form for generic internal use.
template <class APPLICATIVE_MAP, class FUNCTIONS_IN_CONTEXT,
          class ARGUMENTS_IN_CONTEXT>
constexpr auto ap(const APPLICATIVE_MAP &map, FUNCTIONS_IN_CONTEXT &&functions,
                  ARGUMENTS_IN_CONTEXT &&arguments) {
    return map.invoke(applicative_eval,
                      std::forward<FUNCTIONS_IN_CONTEXT>(functions),
                      std::forward<ARGUMENTS_IN_CONTEXT>(arguments));
}

} // namespace detail

// Applicative pattern invariants:
// - Dual BASIS, single INTERFACE. An instance opts in with pure + invoke
//   or pure + ap -- both are perfectly cromulent bases, and the base class
//   derives whichever one the instance does not supply. The user-facing
//   application verb is invoke: McBride & Paterson's canonical form
//     pure f <*> u1 <*> ... <*> un   ==   invoke(f, u1, ..., un),
//   and the GHC analogue is {-# MINIMAL pure, ((<*>) | liftA2) #-}.
// - ap (one-step application of a callable-in-context) is the classic
//   basis and remains available as a secondary operation, but it is never
//   the lead verb: papers, examples, and teaching lead with invoke. The
//   interface choice is forced by std::simd::vec, which cannot form a
//   vec<callable> -- ap is unspellable exactly where this proposal's
//   motivation lives, while n-ary invoke works everywhere.
// - ap-from-invoke is invoke(applicative_eval, cf, cx); it exists only
//   when the context can hold a callable. invoke-from-ap is the currying
//   derivation (detail::terminating_partial); its cost lives entirely in
//   the derivation direction.
// - Impls keep their basis SFINAE-friendly: a trailing return type via
//   std::invoke_result_t (or an explicit requires clause), never a bare
//   auto return with the result type computed only in the body, so that
//   availability probes fail cleanly instead of hard-erroring.
// - Derived operations (map/lift/zip_with/discard_*) live on that object.
// - Dispatch happens through a provided object or
//   applicative_typeclass<Concrete>.
// - Do not introduce hidden alternate semantics without a distinct map/type.

/** CRTP base for Applicative instances.
 * `Impl` must provide `pure(value)` and either the n-ary
 * `invoke(f, args_in_context...)` or the one-step
 * `ap(f_in_context, arg_in_context)`; the base derives the missing one and
 * every other operation. invoke is the user-facing interface; ap is the
 * classic basis.
 */
template <class Impl>
struct Applicative : protected Impl {
    static_assert(
        !std::is_same_v<Impl, std::false_type>,
        "No applicative_typeclass<T> specialization found. "
        "Specialize beman::transpose::applicative_typeclass<T> for "
        "your type T and provide pure(...) plus invoke(...) or ap(...).");
    using Impl::pure;

    /** Lifts `function` into the applicative and applies it to one or more
     * effectful arguments left-to-right, producing a single effectful result.
     * Dispatches to a native `Impl::invoke` when one exists; otherwise
     * derived from the ap basis by currying `function` through one-step
     * applications.
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

        if constexpr (requires(IMPL_BASE &impl) {
                          impl.invoke(
                              std::forward<FUNCTION>(function),
                              std::forward<FIRST_ARGUMENT>(first_argument),
                              std::forward<REST_ARGUMENTS>(rest_arguments)...);
                      }) {
            return static_cast<IMPL_BASE &>(self).invoke(
                std::forward<FUNCTION>(function),
                std::forward<FIRST_ARGUMENT>(first_argument),
                std::forward<REST_ARGUMENTS>(rest_arguments)...);
        } else {
            // Derivation from the ap basis:
            //   invoke(f, x1, ..., xn) = pure(curried f) `ap` x1 `ap` ... xn
            auto lifted_function = self.pure(detail::make_terminating_partial(
                std::forward<FUNCTION>(function)));
            static_assert(
                requires(IMPL_BASE &impl) {
                    impl.ap(std::move(lifted_function),
                            std::forward<FIRST_ARGUMENT>(first_argument));
                }, "Applicative Impl must provide pure and at least one basis: "
                   "invoke(f, args_in_context...) or "
                   "ap(f_in_context, arg_in_context).");
            return self.ap_chain(
                self.ap(std::move(lifted_function),
                        std::forward<FIRST_ARGUMENT>(first_argument)),
                std::forward<REST_ARGUMENTS>(rest_arguments)...);
        }
    }

    /** One-step contextual application -- the classic basis, kept as a
     * secondary operation. Dispatches to a native `Impl::ap` when one
     * exists; otherwise derived as invoke(applicative_eval, cf, cx), which
     * is available exactly when the context can hold a callable (the
     * constraint SFINAEs away otherwise, e.g. for std::simd::vec). Both
     * branches call the Impl directly, so no derivation cycle with
     * `invoke` is possible.
     */
    template <class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
    auto ap(this auto &&self, FUNCTION_IN_CONTEXT &&function,
            ARGUMENT_IN_CONTEXT &&argument)
        requires requires(const Impl &impl) {
            impl.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                    std::forward<ARGUMENT_IN_CONTEXT>(argument));
        } || requires(const Impl &impl) {
            impl.invoke(detail::applicative_eval,
                        std::forward<FUNCTION_IN_CONTEXT>(function),
                        std::forward<ARGUMENT_IN_CONTEXT>(argument));
        }
    {
        using SELF = std::remove_reference_t<decltype(self)>;
        using IMPL_BASE =
            std::conditional_t<std::is_const_v<SELF>, const Impl, Impl>;
        if constexpr (requires(IMPL_BASE &impl) {
                          impl.ap(std::forward<FUNCTION_IN_CONTEXT>(function),
                                  std::forward<ARGUMENT_IN_CONTEXT>(argument));
                      }) {
            return static_cast<IMPL_BASE &>(self).ap(
                std::forward<FUNCTION_IN_CONTEXT>(function),
                std::forward<ARGUMENT_IN_CONTEXT>(argument));
        } else {
            return static_cast<IMPL_BASE &>(self).invoke(
                detail::applicative_eval,
                std::forward<FUNCTION_IN_CONTEXT>(function),
                std::forward<ARGUMENT_IN_CONTEXT>(argument));
        }
    }

  private:
    template <class ACCUMULATED>
    auto ap_chain(this auto &&, ACCUMULATED &&accumulated) {
        return std::forward<ACCUMULATED>(accumulated);
    }

    template <class ACCUMULATED, class NEXT_ARGUMENT, class... REST_ARGUMENTS>
    auto ap_chain(this auto &&self, ACCUMULATED &&accumulated,
                  NEXT_ARGUMENT &&next_argument,
                  REST_ARGUMENTS &&...rest_arguments) {
        auto next = self.ap(std::forward<ACCUMULATED>(accumulated),
                            std::forward<NEXT_ARGUMENT>(next_argument));
        if constexpr (sizeof...(REST_ARGUMENTS) == 0) {
            return next;
        } else {
            return self.ap_chain(std::move(next), std::forward<REST_ARGUMENTS>(
                                                      rest_arguments)...);
        }
    }

  public:
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
