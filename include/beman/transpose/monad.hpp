// include/beman/transpose/monad.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_MONAD_HPP
#define BEMAN_TRANSPOSE_MONAD_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/grade.hpp>

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
struct derive_monad : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No monad<T> specialization found. "
                  "Specialize beman::transpose::monad<T> for your "
                  "type T and provide pure(...) and bind(...) operations.");

    using Impl::bind;
    using Impl::pure;

    /** fmap: the Functor basis, grounded in bind + pure.
     *
     *   fmap(f, ma) = ma >>= (pure . f)
     *
     * A monad is a functor, and this is that theorem spelled as an operation
     * rather than as a superclass constraint. Prefers a native Impl::fmap
     * when the instance supplies one -- an instance that can map without
     * sequencing usually should. The full Functor instance is spelled at the
     * registration site as derive_functor<ThisMap>{}; Monad grows the Functor
     * BASIS only, so Functor's derived surface stays Functor's.
     */
    template <class FUNCTION, class MA>
    auto fmap(this auto &&self, FUNCTION &&function, MA &&ma)
        requires requires(const Impl &impl) {
            impl.fmap(std::forward<FUNCTION>(function), std::forward<MA>(ma));
        } || requires(const Impl &impl) {
            impl.bind(std::forward<MA>(ma), std::declval<FUNCTION &>());
        }
    {
        if constexpr (requires {
                          impl_of(self).fmap(std::forward<FUNCTION>(function),
                                             std::forward<MA>(ma));
                      }) {
            return impl_of(self).fmap(std::forward<FUNCTION>(function),
                                      std::forward<MA>(ma));
        } else {
            return impl_of(self).bind(std::forward<MA>(ma), [&](auto &&a) {
                return impl_of(self).pure(
                    std::invoke(function, std::forward<decltype(a)>(a)));
            });
        }
    }

    // invoke: n-ary lift synthesized from bind + pure (left-nested binds):
    //   invoke(f, m1, ..., mn) = m1 >>= \a1 -> ... mn >>= \an ->
    //   pure(f(a1...an))
    // Prefers a native Impl::invoke when present, like Applicative. This
    // derivation assumes a synchronous bind (the continuation is invoked
    // before bind returns), which holds for every Monad instance in this
    // repository.
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&self, FUNCTION &&function, FIRST &&first,
                REST &&...rest)
        requires requires(const Impl &impl) {
            impl.invoke(std::forward<FUNCTION>(function),
                        std::forward<FIRST>(first),
                        std::forward<REST>(rest)...);
        } || requires(const Impl &impl) {
            // The derivation's own requirement on the basis: Impl must have
            // a bind that accepts FIRST with a single-argument callback
            // (pure is already guaranteed unconditionally by the class
            // invariant above, so it is not separately probed). FUNCTION's
            // own arity does not enter here -- the derivation applies it
            // only after unwinding through nested bind calls, never
            // directly to FIRST's element.
            impl.bind(std::forward<FIRST>(first),
                      [](auto &&value) -> decltype(auto) {
                          return std::forward<decltype(value)>(value);
                      });
        }
    {
        if constexpr (requires {
                          impl_of(self).invoke(std::forward<FUNCTION>(function),
                                               std::forward<FIRST>(first),
                                               std::forward<REST>(rest)...);
                      }) {
            return impl_of(self).invoke(std::forward<FUNCTION>(function),
                                        std::forward<FIRST>(first),
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
    // Prefers a native Impl::join. join/bind is a mutually-derivable pair
    // (join = bind(., id); bind is recoverable from join + fmap), so the
    // fallback addresses Impl directly, matching fmap's bind-basis branch --
    // the one member in this step where the derivation, not just the probe,
    // addresses Impl rather than self.
    template <class MMA>
    auto join(this auto &&self, MMA &&mma)
        requires requires(const Impl &impl) {
            impl.join(std::forward<MMA>(mma));
        } || requires(const Impl &impl) {
            impl.bind(std::forward<MMA>(mma),
                      [](auto &&inner) { return inner; });
        }
    {
        if constexpr (requires {
                          impl_of(self).join(std::forward<MMA>(mma));
                      }) {
            return impl_of(self).join(std::forward<MMA>(mma));
        } else {
            return impl_of(self).bind(std::forward<MMA>(mma),
                                      [](auto &&inner) { return inner; });
        }
    }

    // kleisli: forward Kleisli composition (>=>).
    // (f >=> g) a = f a >>= g
    // Prefers a native Impl::kleisli. Unlike the other members in this step,
    // kleisli's own basis requirement cannot be spelled as a disjunctive
    // second alternative: the derivation's use of bind is inside the
    // returned closure, over an argument type ("a") that is not known until
    // the closure is called, so there is no concrete expression to probe at
    // kleisli's own instantiation. bind's presence is already guaranteed
    // unconditionally by this class's invariant (the static_assert above),
    // so the second alternative is trivially true rather than absent.
    template <class F, class G>
    auto kleisli(this auto &&self, F f, G g)
        requires requires(const Impl &impl) { impl.kleisli(f, g); } || true
    {
        if constexpr (requires { impl_of(self).kleisli(f, g); }) {
            return impl_of(self).kleisli(f, g);
        } else {
            return [&self, f = std::move(f), g = std::move(g)](auto &&a) {
                return self.bind(f(std::forward<decltype(a)>(a)), g);
            };
        }
    }

    /** ap: one-step contextual application, derived from bind + pure.
     *
     *   ap(mf, ma) = mf >>= \f -> ma >>= \a -> pure(f a)
     *
     * The Applicative base derives ap from its n-ary invoke; a monad-derived
     * instance has no invoke to derive from until one is synthesized, so it
     * gets ap from the bind basis instead. Both derivations exist exactly
     * when the context can hold a callable, and the constraint says so, so
     * the member disappears for contexts that cannot (std::simd::vec) rather
     * than failing inside the body.
     */
    template <class MF, class MA>
    auto ap(this auto &&self, MF &&mf, MA &&ma)
        requires requires(const Impl &impl) {
            impl.ap(std::forward<MF>(mf), std::forward<MA>(ma));
        } || requires {
            typename applicative_value_t<MF>;
            typename applicative_value_t<MA>;
            requires std::invocable<const applicative_value_t<MF> &,
                                    const applicative_value_t<MA> &>;
        }
    {
        if constexpr (requires {
                          impl_of(self).ap(std::forward<MF>(mf),
                                           std::forward<MA>(ma));
                      }) {
            return impl_of(self).ap(std::forward<MF>(mf), std::forward<MA>(ma));
        } else {
            return self.bind(std::forward<MF>(mf), [&self,
                                                    &ma](auto &&function) {
                return self.bind(ma, [&self, &function](auto &&argument) {
                    return self.pure(std::invoke(
                        function, std::forward<decltype(argument)>(argument)));
                });
            });
        }
    }

    /** Uses a value at a wider grade, defaulted from the grade algebra.
     *
     * The same defaulted subsumption the Applicative base carries; see the
     * note there. Bind is the operation that joins grades, so the monad is
     * where widening is reached for most often.
     */
    template <class TARGET_GRADE, class CARRIER>
    constexpr auto subsume(this auto &&, CARRIER &&value)
        requires requires {
            grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
        }
    {
        return grade_subsume<TARGET_GRADE>(std::forward<CARRIER>(value));
    }

    // bind_with: explicit monad object override.
    template <class MONAD_MAP, class MA, class F>
    auto bind_with(this auto &&, const MONAD_MAP &monad_map, MA &&ma, F &&f) {
        return monad_map.bind(std::forward<MA>(ma), std::forward<F>(f));
    }

    /** The full Functor instance over this monad object.
     *
     * Every typeclass object is stateless and empty, so constructing the
     * full instance and "converting" are the same free type-level move.
     * This is `Monad m => Functor m` superclass subsumption, paid for with
     * one visible call that names which functor is meant, instead of a
     * std::remove_cvref_t incantation at the call site:
     *
     *     f(monad_map.as_functor(), xs);
     *
     * The functor it returns is the one derived from the object in hand,
     * law-compatible with that object's bind by construction. It
     * deliberately does NOT consult functor<T>: a caller who
     * wants the registered default says so by looking it up.
     */
    constexpr auto as_functor(this auto &&self) {
        return derive_functor<std::remove_cvref_t<decltype(self)>>{};
    }

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }
};

/** Typeclass lookup variable for Monad; specialize for each type. */
template <class T>
inline constexpr auto monad = std::false_type{};

/** Restricted `Impl` concept for Monad: satisfied when `IMPL` supplies the
 * one minimal complete basis the `Monad` CRTP base admits today -- `pure`
 * and `bind`. Monad's other complete bases -- `pure` + `fmap` + `join`, and
 * `pure` + `kleisli` -- are deliberately not admitted here; extending this
 * concept to accept them is a separate, unscheduled piece of work. This is
 * the `MINIMAL` pragma to `monad_object`'s class declaration: `fmap`,
 * `invoke`, `join`, `kleisli`, `ap`, `subsume` and `bind_with` are all
 * derived and belong to `monad_object` alone.
 */
template <class IMPL, class CONTEXT>
concept monad_impl = requires(const IMPL &impl, const CONTEXT &context,
                              const applicative_value_t<CONTEXT> &element) {
    impl.pure(element);
    impl.bind(context, detail::probe_witness<CONTEXT>{});
};

/** Deep object concept for a Monad object over `CONTEXT`: satisfied when
 * `OBJ` provides the full object surface -- `pure`, `bind`, `fmap`,
 * `invoke`, `join`, `kleisli` and `bind_with`. `ap` and `subsume` are
 * required only where their own condition licenses them, mirroring
 * `applicative_object`'s treatment.
 *
 * `kleisli` is probed for existence -- the class surface names it -- but its
 * presence is never load-bearing evidence here: `derive_monad<Impl>::kleisli`'s
 * own condition is `impl.kleisli(f, g) || true`, genuinely unconstrained,
 * because its `bind` call lives inside a returned closure whose argument
 * type is unknown until the closure is invoked. It is `bind` and `join`
 * (and `fmap`, `invoke`) that carry a real either-basis condition and do the
 * actual discriminating.
 */
template <class OBJ, class CONTEXT>
concept monad_object =
    requires(const OBJ &obj, const CONTEXT &context,
             const applicative_value_t<CONTEXT> &element) {
        obj.pure(element);
        obj.bind(context, detail::probe_witness<CONTEXT>{});
        obj.fmap(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.invoke(detail::probe_witness<applicative_value_t<CONTEXT>>{}, context);
        obj.join(obj.pure(context));
        obj.kleisli(detail::probe_witness<CONTEXT>{}, detail::probe_witness<CONTEXT>{});
        obj.bind_with(obj, context, detail::probe_witness<CONTEXT>{});
    } &&
    (!requires(const OBJ &obj) {
        obj.pure(detail::probe_witness<applicative_value_t<CONTEXT>>{});
    } || requires(const OBJ &obj, const CONTEXT &context) {
        obj.ap(obj.pure(detail::probe_witness<applicative_value_t<CONTEXT>>{}),
               context);
    }) &&
    (!graded_context<CONTEXT> ||
     requires(const OBJ &obj, const CONTEXT &context) {
         obj.template subsume<grade_of_t<CONTEXT>>(context);
     });

// -- std::optional monad instance --
// Delegates pure to the existing applicative.

template <class VALUE_TYPE>
struct OptionalMonadImpl {
    using element_type = VALUE_TYPE;

    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::optional<std::remove_cvref_t<VALUE>> {
        return applicative<std::optional<VALUE_TYPE>>.pure(
            std::forward<VALUE>(value));
    }

    template <class A, class F>
    auto bind(this auto &&, const std::optional<A> &ma, F &&f)
        -> std::remove_cvref_t<std::invoke_result_t<F, const A &>> {
        using Result = std::remove_cvref_t<std::invoke_result_t<F, const A &>>;
        if (!ma)
            return Result{};
        return Result{std::invoke(std::forward<F>(f), *ma)};
    }
};

template <class VALUE_TYPE>
struct OptionalMonadMap : derive_monad<OptionalMonadImpl<VALUE_TYPE>> {
    using OptionalMonadImpl<VALUE_TYPE>::bind;
    using OptionalMonadImpl<VALUE_TYPE>::pure;
};

/** Monad instance for `std::optional<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto monad<std::optional<VALUE_TYPE>> =
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
// that. An object needs no such dodge: `bind` is a variable, it is
// never found by ADL, and it never enters overload resolution with std::bind.
// The typeclass operation gets its real name.

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
              const auto &TC = monad<std::remove_cvref_t<MA>>>
        requires requires { TC.bind(std::declval<MA>(), std::declval<F>()); }
    constexpr auto operator()(MA &&ma, F &&f) const {
        return TC.bind(std::forward<MA>(ma), std::forward<F>(f));
    }
};

/** Sequential composition: `bind(ma, f)`. */
inline constexpr bind_fn bind{};

/** Operation object for Monad's derived `join`. */
struct join_fn {
    /** Flattens a nested monadic value; equivalent to `bind(mma, id)`. */
    template <class MMA, const auto &TC = monad<std::remove_cvref_t<MMA>>>
        requires requires { TC.join(std::declval<MMA>()); }
    constexpr auto operator()(MMA &&mma) const {
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
template <class CONTEXT, const auto &TC = monad<std::remove_cvref_t<CONTEXT>>>
struct kleisli_fn {
    /** Forward Kleisli composition: `(f >=> g) a = f a >>= g`. */
    template <class F, class G>
        requires requires { TC.kleisli(std::declval<F>(), std::declval<G>()); }
    constexpr auto operator()(F &&f, G &&g) const {
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

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_MONAD_HPP
