// include/beman/transpose/monad.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_MONAD_HPP
#define BEMAN_TRANSPOSE_MONAD_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

// EVIDENCE, NOT PROPOSED WORDING. D3200R0 does not propose a Monad
// abstraction — deferred, not rejected. Everything about the typeclass
// design leads to a consistent generic name for sequential composition
// eventually (today's and_then/transform/let_value are per-type members,
// not generic), and this header is the proof the mechanism carries Monad
// without strain. It is unproposed only because nothing in the coordinated
// set currently needs bind; the slot stays open for the paper that does.
// See D3200R0 "Why not Monad".

/** CRTP base for Monad instances.
 * `Impl` must provide `pure(value)` and `bind(ma, f)`.
 * The n-ary `invoke` is synthesized; `join` and `kleisli` are derived.
 * Monad does not inherit from Applicative, but provides the equivalent
 * applicative operation once `invoke` is synthesized from `bind` + `pure`.
 */
template <class Impl>
struct Monad : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No monad_typeclass<T> specialization found. "
                  "Specialize beman::transpose::monad_typeclass<T> for your "
                  "type T and provide pure(...) and bind(...) operations.");

    using Impl::bind;
    using Impl::pure;

    // invoke: n-ary lift synthesized from bind + pure (left-nested binds):
    //   invoke(f, m1, ..., mn) = m1 >>= \a1 -> ... mn >>= \an ->
    //   pure(f(a1...an))
    // Prefers a native Impl::invoke when present, like Applicative. This
    // derivation assumes a synchronous bind (the continuation is invoked
    // before bind returns), which holds for every Monad instance in this
    // repository.
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&self, FUNCTION &&function, FIRST &&first,
                REST &&...rest) {
        using SELF = std::remove_reference_t<decltype(self)>;
        using IMPL_BASE =
            std::conditional_t<std::is_const_v<SELF>, const Impl, Impl>;
        if constexpr (requires(IMPL_BASE &impl) {
                          impl.invoke(std::forward<FUNCTION>(function),
                                      std::forward<FIRST>(first),
                                      std::forward<REST>(rest)...);
                      }) {
            return static_cast<IMPL_BASE &>(self).invoke(
                std::forward<FUNCTION>(function), std::forward<FIRST>(first),
                std::forward<REST>(rest)...);
        } else {
            return self.bind(std::forward<FIRST>(first), [&](auto &&head) {
                if constexpr (sizeof...(REST) == 0) {
                    return self.pure(std::invoke(
                        function, std::forward<decltype(head)>(head)));
                } else {
                    return self.invoke(
                        [&function, &head](auto &&...tail) {
                            return std::invoke(
                                function, head,
                                std::forward<decltype(tail)>(tail)...);
                        },
                        std::forward<REST>(rest)...);
                }
            });
        }
    }

    // join: flatten nested monad.
    // join mma = mma >>= id
    template <class MMA>
    auto join(this auto &&self, MMA &&mma) {
        return self.bind(std::forward<MMA>(mma),
                         [](auto &&inner) { return inner; });
    }

    // kleisli: forward Kleisli composition (>=>).
    // (f >=> g) a = f a >>= g
    template <class F, class G>
    auto kleisli(this auto &&self, F f, G g) {
        return [&self, f = std::move(f), g = std::move(g)](auto &&a) {
            return self.bind(f(std::forward<decltype(a)>(a)), g);
        };
    }

    // bind_with: explicit monad object override.
    template <class MONAD_MAP, class MA, class F>
    auto bind_with(this auto &&, const MONAD_MAP &monad_map, MA &&ma, F &&f) {
        return monad_map.bind(std::forward<MA>(ma), std::forward<F>(f));
    }
};

/** Typeclass lookup variable for Monad; specialize for each type. */
template <class T>
inline constexpr auto monad_typeclass = std::false_type{};

// -- std::optional monad instance --
// Delegates pure to the existing applicative_typeclass.

template <class VALUE_TYPE>
struct OptionalMonadImpl {
    using element_type = VALUE_TYPE;

    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::optional<remove_cvref_t<VALUE>> {
        return applicative_typeclass<std::optional<VALUE_TYPE>>.pure(
            std::forward<VALUE>(value));
    }

    template <class A, class F>
    auto bind(this auto &&, const std::optional<A> &ma, F &&f) {
        using Result = remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }
};

template <class VALUE_TYPE>
struct OptionalMonadMap : Monad<OptionalMonadImpl<VALUE_TYPE>> {
    using OptionalMonadImpl<VALUE_TYPE>::bind;
    using OptionalMonadImpl<VALUE_TYPE>::pure;
};

/** Monad instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto monad_typeclass<std::optional<VALUE_TYPE>> =
    OptionalMonadMap<VALUE_TYPE>{};

// -- Operation objects --
//
// See functor.hpp for the shape, the motivation, and why the namespace is
// nested. Still evidence, not proposed wording -- but evidence of one more
// thing now: that the operation-object layer is uniform across the whole
// typeclass family, including the member nobody is proposing. If the spelling
// only worked for the operations D3200R0 happens to propose, that would say
// it had been fitted to those operations instead of to the mechanism.
//
// It also settles a naming question the free-function API had ducked. `bind`
// could not be a function template here without reading as a competitor to
// std::bind; the old spelling was `mbind`, a wart adopted to dodge exactly
// that. An object needs no such dodge: `typeclass::bind` is a variable, it is
// never found by ADL, and it never enters overload resolution with std::bind.
// The typeclass operation gets its real name.

namespace typeclass {

/** Operation object for Monad's `bind` primitive.
 *
 * The context keys the lookup and is deduced from the first argument.
 */
struct bind_fn {
    /** Sequences a monadic value `ma` through `f` (Haskell's `>>=`).
     *
     * @tparam TC  The Monad instance, from the lookup for the keying argument.
     * Not for callers to supply; pin with mode 2.
     */
    template <class MA, class F,
              const auto &TC = monad_typeclass<remove_cvref_t<MA>>>
    constexpr auto operator()(MA &&ma, F &&f) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Monad instance for this type. Specialize "
                      "beman::transpose::monad_typeclass<T> with an object "
                      "providing pure(...) and bind(ma, f).");
        return TC.bind(std::forward<MA>(ma), std::forward<F>(f));
    }
};

/** Sequential composition: `bind(ma, f)`. */
inline constexpr bind_fn bind{};

/** Operation object for Monad's derived `join`. */
struct join_fn {
    /** Flattens a nested monadic value; equivalent to `bind(mma, id)`. */
    template <class MMA, const auto &TC = monad_typeclass<remove_cvref_t<MMA>>>
    constexpr auto operator()(MMA &&mma) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Monad instance for this type. Specialize "
                      "beman::transpose::monad_typeclass<T> with an object "
                      "providing pure(...) and bind(ma, f).");
        return TC.join(std::forward<MMA>(mma));
    }
};

/** Flattens one level of nesting: `join(mma)`. */
inline constexpr join_fn join{};

/** Operation object for Monad's derived `kleisli` composition.
 *
 * Neither argument is in-context -- both are functions returning a monadic
 * value -- so, like `pure`, the context cannot be deduced and is named:
 * `kleisli<std::optional<int>>(f, g)`. Two of the family's operations fall
 * out of the deducing pattern this way, and they are exactly the two that
 * produce a context instead of consuming one.
 */
template <class CONTEXT,
          const auto &TC = monad_typeclass<remove_cvref_t<CONTEXT>>>
struct kleisli_fn {
    /** Forward Kleisli composition: `(f >=> g) a = f a >>= g`. */
    template <class F, class G>
    constexpr auto operator()(F &&f, G &&g) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Monad instance for this type. Specialize "
                      "beman::transpose::monad_typeclass<T> with an object "
                      "providing pure(...) and bind(ma, f).");
        return TC.kleisli(std::forward<F>(f), std::forward<G>(g));
    }
};

/** Kleisli composition in a named context: `kleisli<C>(f, g)`. */
template <class CONTEXT>
inline constexpr kleisli_fn<CONTEXT> kleisli{};

// -- Free-function API --

/** Sequences a monadic value `ma` through function `f` (Haskell's `>>=`).
 *
 * Retained for source compatibility; `bind` is the spelling to use.
 */
template <class MA, class F>
auto mbind(MA &&ma, F &&f) {
    return bind(std::forward<MA>(ma), std::forward<F>(f));
}
} // namespace typeclass

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_MONAD_HPP
