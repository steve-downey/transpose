// include/beman/transpose/functor.hpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_FUNCTOR_HPP
#define BEMAN_TRANSPOSE_FUNCTOR_HPP

#include <beman/transpose/detail/typeclass_base.hpp>

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::transpose {

// Functor pattern invariants:
// - Instances are single lookup objects that provide fmap(F, T).
// - replace is a derived object operation implemented from fmap.
// - Dispatch happens through a provided object or functor_typeclass<Concrete>.
// - Keep lookup explicit through typeclass objects, not ADL overloads.

/** CRTP base for Functor instances.
 * `Impl` must provide `fmap(f, container)`; `replace` is derived from it.
 */
template <class Impl>
struct Functor : protected Impl {
    using Impl::fmap;

    /** Replaces every element of `value` with `replacement`, ignoring the
     * original element values.
     */
    template <class T, class U>
    auto replace(this auto &&self, T &&value, U &&replacement) {
        return self.fmap([replacement = std::forward<U>(replacement)](
                             const auto &) { return replacement; },
                         std::forward<T>(value));
    }
};

/** Typeclass lookup variable for Functor; specialize for each container type.
 */
template <class T>
inline constexpr auto functor_typeclass = std::false_type{};

// -- Customization-point objects --
//
// A fourth lookup mode, on top of implicit lookup, explicit object argument,
// and NTTP pinning (examples/lookup_modes_example.cpp): the operation as a
// niebloid that does the lookup itself. `fmap(f, v)` names the operation the
// caller means, rather than the object the operation is reached through.
//
// Two cases motivate them. First, everything-is-local code -- a container on
// the stack that one algorithm maps or traverses -- where naming a lookup
// object costs a line to save nothing. Second, and the reason the pattern is
// not merely cosmetic: a *non-template* caller can not carry the typeclass as
// an NTTP, because there is no template parameter list to hang it on. The CPO
// resolves the instance at the call site instead.
//
// Shape, uniform across every typeclass here: deduce the type that keys the
// lookup from one designated argument, default a trailing `const auto &TC`
// NTTP to the lookup for that type, and call through it. Pinning stays
// available by supplying TC, and the pre-existing modes are untouched -- the
// CPO is an addition, never a replacement.

/** Customization-point object for Functor's `fmap`.
 *
 * The container type keys the lookup and is deduced from the second
 * argument.
 */
struct fmap_fn {
    /** Applies `function` to every element of `value`, preserving shape.
     *
     * @tparam TC  The Functor instance; defaults to the lookup for `T` and
     *             may be pinned explicitly.
     */
    template <class F, class T,
              const auto &TC = functor_typeclass<remove_cvref_t<T>>>
    constexpr auto operator()(F &&function, T &&value) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Functor instance for this type. Specialize "
                      "beman::transpose::functor_typeclass<T> with an object "
                      "providing fmap(f, container).");
        return TC.fmap(std::forward<F>(function), std::forward<T>(value));
    }
};

/** Maps a function over a functor: `fmap(f, container)`. */
inline constexpr fmap_fn fmap{};

/** Instance-pinned form of `fmap`, for reaching an instance other than the
 * registered one.
 *
 * The trailing `TC` on `fmap_fn::operator()` is reachable in principle, but
 * only as `fmap.operator()<F, T, TC>(...)` -- an object has no syntax for
 * explicit template arguments, and every deduced parameter has to be
 * respelled to get past them to the pin. Making the pin a *class* template
 * parameter restores lookup mode 3 at its usual cost:
 *
 *     fmap_with<my_functor>(f, container)
 *
 * The `_with` suffix matches the instance-overriding operations the CRTP
 * bases already carry (`traverse_with`, `invoke_with`, `bind_with`).
 */
template <const auto &TC>
struct fmap_with_fn {
    /** Applies `function` to every element of `value` using the pinned
     * instance. */
    template <class F, class T>
    constexpr auto operator()(F &&function, T &&value) const {
        return TC.fmap(std::forward<F>(function), std::forward<T>(value));
    }
};

/** Maps a function over a functor using a pinned instance. */
template <const auto &TC>
inline constexpr fmap_with_fn<TC> fmap_with{};

/** Customization-point object for Functor's derived `replace`. */
struct replace_fn {
    /** Replaces every element of `value` with `replacement`. */
    template <class T, class U,
              const auto &TC = functor_typeclass<remove_cvref_t<T>>>
    constexpr auto operator()(T &&value, U &&replacement) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Functor instance for this type. Specialize "
                      "beman::transpose::functor_typeclass<T> with an object "
                      "providing fmap(f, container).");
        return TC.replace(std::forward<T>(value), std::forward<U>(replacement));
    }
};

/** Replaces every element of a functor with one value. */
inline constexpr replace_fn replace{};

template <class VALUE_TYPE>
struct OptionalFunctorImpl {
    template <class F>
    auto fmap(this auto &&, F &&function,
              const std::optional<VALUE_TYPE> &value) {
        using Result = std::invoke_result_t<F, const VALUE_TYPE &>;
        if (!value) {
            return std::optional<remove_cvref_t<Result>>{};
        }
        return std::optional<remove_cvref_t<Result>>{
            std::invoke(std::forward<F>(function), *value)};
    }
};

template <class VALUE_TYPE>
struct VectorFunctorImpl {
    template <class F>
    auto fmap(this auto &&, F &&function,
              const std::vector<VALUE_TYPE> &values) {
        using Result = std::invoke_result_t<F, const VALUE_TYPE &>;
        std::vector<remove_cvref_t<Result>> output;
        output.reserve(values.size());

        std::ranges::transform(values, std::back_inserter(output),
                               [&function](const VALUE_TYPE &v) {
                                   return std::invoke(function, v);
                               });

        return output;
    }
};

template <class VALUE_TYPE>
struct OptionalFunctorMap : Functor<OptionalFunctorImpl<VALUE_TYPE>> {
    using OptionalFunctorImpl<VALUE_TYPE>::fmap;
};

template <class VALUE_TYPE>
struct VectorFunctorMap : Functor<VectorFunctorImpl<VALUE_TYPE>> {
    using VectorFunctorImpl<VALUE_TYPE>::fmap;
};

/** Functor instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto functor_typeclass<std::optional<VALUE_TYPE>> =
    OptionalFunctorMap<VALUE_TYPE>{};

/** Functor instance for `std::vector<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto functor_typeclass<std::vector<VALUE_TYPE>> =
    VectorFunctorMap<VALUE_TYPE>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_FUNCTOR_HPP
