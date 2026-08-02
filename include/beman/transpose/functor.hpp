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
// - Dispatch happens through a provided object or functor<Concrete>.
// - Keep lookup explicit through typeclass objects, not ADL overloads.

/** CRTP base for Functor instances.
 * `Impl` must provide `fmap(f, container)`; `replace` is derived from it.
 */
template <class Impl>
struct derive_functor : protected Impl {
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
inline constexpr auto functor = std::false_type{};

// -- Operation objects --
//
// This comment is the reference for the operation objects; the other
// typeclass headers point back to it.
//
// A fourth lookup mode, on top of implicit lookup, explicit object argument,
// and NTTP pinning (examples/lookup_modes_example.cpp): the operation as an
// object that does the lookup itself. `fmap(f, v)` names the operation the
// caller means, rather than the object the operation is reached through.
//
// Two cases motivate them. First, everything-is-local code -- a container on
// the stack that one algorithm maps or traverses -- where naming a lookup
// object costs a line to save nothing. Second, and the reason the pattern is
// not merely cosmetic: a *non-template* caller can not carry the typeclass as
// an NTTP, because there is no template parameter list to hang it on. The
// operation object resolves the instance at the call site instead.
//
// They are NOT customization points, and D3200R0's case for a bundled
// mechanism depends on the difference. Customization happens once, at
// functor<T> and its siblings. There is no per-operation hook, no
// ADL path in, and nothing specializable that would make `fmap` mean
// something other than what the instance's own fmap means. The familiar name
// for this shape is "customization point object"; it is the wrong name here,
// and these are operation objects over a customizable bundle.
//
// Shape, uniform across every typeclass: deduce the type that keys the lookup
// from one designated argument, default a trailing `const auto &TC` NTTP to
// the lookup for that type, and call through it.
//
// ONE NAMESPACE FOR THE WHOLE FAMILY. Every typeclass operation lives in one
// namespace, not one namespace per typeclass. Here that namespace is
// beman::transpose, which is already nested and already scoped to exactly
// this facility; the standard spelling this maps to is `std::typeclass`.
//
// The reason is coherence, and it is part of the design rather than a
// consequence of it. A name in here means one thing across the entire family:
// `fmap` is Functor's map whichever instance supplies it, and Applicative is
// free to provide the Functor names because they can not mean anything else.
// That commitment is already load-bearing elsewhere -- it is what lets an
// algorithm inherit from two typeclass instances at once and call both
// unqualified (see examples/algorithm_object_example.cpp). Collecting the
// operations in one namespace makes the commitment checkable in one place. A
// namespace per typeclass would let `fmap` mean two things and hide it.
//
// It also keeps the operations away from std, where `empty`, `length`,
// `invoke`, `to_vector`, `any_of` and `all_of` all exist with related
// meanings. `beman::transpose::empty` is not something a caller drags into
// scope by accident, and `std::typeclass::empty` would not be either.
//
// NAMING. The lookup variables get the plain names -- `functor<T>`,
// `applicative<T>`, `foldable<T>`, `traversable<T>`, `monad<T>` -- because
// they are the user-facing surface. What that collides with gets the longer,
// more descriptive name: the CRTP bases became `derive_functor<Impl>` and
// friends, which says what they do (derive the rest of a typeclass from an
// instance's primitives) instead of relying on a capital letter to tell
// `Functor` from `functor`.
//
// One consequence worth knowing: a local named `applicative` now shadows
// `applicative<T>`. Locals holding a looked-up instance are spelled
// `applicative_map` here, matching the existing `traversable_map` and
// `monad_map` parameter names.
//
// The one name where coherence is under real strain is `empty`, which means
// Foldable's "holds nothing" here and Alternative's "identity of alt" in the
// sibling compile-time-scheme tree. Adding Alternative would claim it twice,
// in the one namespace whose job is to make that impossible. The three
// languages with both typeclasses all give `empty` to Alternative and name
// the Foldable predicate something else -- `null` in Haskell (Data.Foldable)
// and PureScript (which re-exports Control.Plus's `empty` from the same
// module, so the split is load-bearing there), `isEmpty` in Cats, whose
// Alternative gets `empty` from MonoidK.
//
// C++ points the other way: std::empty, std::ranges::empty and every
// container's .empty() are the *predicate*, and the language has no
// nullary-constructor-named-empty convention to protect. So the plan here is
// the opposite of the FP convention and consistent with the host language:
// Foldable keeps `empty`, and Alternative's identity takes another name when
// it arrives. `zero<C>()` sits naturally beside `pure<C>(v)` -- both build a
// context and both must name it, for the same reason.
//
// PINNING is mode 2, unchanged: `tc.fmap(f, v)` on an instance object, which
// is shorter than any spelling routed through these objects. An operation
// object takes no explicit template arguments, so mode 3 is not available on
// it -- the trailing TC above is reachable only as
// `fmap.operator()<F, T, TC>(...)`, which is a curiosity, not an
// API. The earlier lookup modes are all untouched; `typeclass` is an addition.

/** Operation object for Functor's `fmap`.
 *
 * The container type keys the lookup and is deduced from the second
 * argument.
 */
struct fmap_fn {
    /** Applies `function` to every element of `value`, preserving shape.
     *
     * @tparam TC  The Functor instance, from the lookup for the keying
     * argument. Not for callers to supply; pin with mode 2.
     */
    template <class F, class T, const auto &TC = functor<remove_cvref_t<T>>>
    constexpr auto operator()(F &&function, T &&value) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Functor instance for this type. Specialize "
                      "beman::transpose::functor<T> with an object "
                      "providing fmap(f, container).");
        return TC.fmap(std::forward<F>(function), std::forward<T>(value));
    }
};

/** Maps a function over a functor: `fmap(f, container)`. */
inline constexpr fmap_fn fmap{};

/** Operation object for Functor's derived `replace`. */
struct replace_fn {
    /** Replaces every element of `value` with `replacement`. */
    template <class T, class U, const auto &TC = functor<remove_cvref_t<T>>>
    constexpr auto operator()(T &&value, U &&replacement) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Functor instance for this type. Specialize "
                      "beman::transpose::functor<T> with an object "
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
struct OptionalFunctorMap : derive_functor<OptionalFunctorImpl<VALUE_TYPE>> {
    using OptionalFunctorImpl<VALUE_TYPE>::fmap;
};

template <class VALUE_TYPE>
struct VectorFunctorMap : derive_functor<VectorFunctorImpl<VALUE_TYPE>> {
    using VectorFunctorImpl<VALUE_TYPE>::fmap;
};

/** Functor instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto functor<std::optional<VALUE_TYPE>> =
    OptionalFunctorMap<VALUE_TYPE>{};

/** Functor instance for `std::vector<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto functor<std::vector<VALUE_TYPE>> =
    VectorFunctorMap<VALUE_TYPE>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_FUNCTOR_HPP
