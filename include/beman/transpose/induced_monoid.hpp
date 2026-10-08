// include/beman/transpose/induced_monoid.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_INDUCED_MONOID_HPP
#define BEMAN_TRANSPOSE_INDUCED_MONOID_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/monoid.hpp>

#include <functional>
#include <utility>

// EVIDENCE, NOT PROPOSED WORDING. These are two monoids the library's own
// typeclass instances induce, not a facility D3200R0 proposes: a Monad
// induces a monoid on its Kleisli arrows, and an Applicative induces a
// monoid on its context lifted from a monoid on the element. Both are
// monoid *instances* over bare value types -- an erased arrow type, and the
// context itself -- and neither registers anything for any raw carrier; see
// docs/decisions.md#monoid-selection. This header exists to show the
// mechanism carries the induction without strain, and to record, once, the
// one thing worth not rediscovering: the categorical statement "a monad is
// a monoid in the category of endofunctors" is inexpressible at `Monoid<T>`,
// whose tensor is a product and whose carrier is a type, not a type
// constructor. The Kleisli endomorphism monoid below is the value-level
// statement that survives that gap -- evidence of the theorem, not the
// theorem itself.

namespace beman::transpose {

/** The Kleisli endomorphism monoid of `MONAD_OBJECT` at `A`: an instance
 * over Kleisli arrows `A -> M<A>`, type-erased through `std::function` so
 * that `combine` can return the same type it takes -- the same erasure
 * `detail::left_compose_monoid` (`fold.hpp`) uses for the same reason: a
 * monoid's value type must close under `combine`, and a bare lambda type
 * does not.
 *
 * Identity is `pure`, combine is Kleisli composition (`>=>`). The
 * Kleisli-form monad laws -- `pure` is the two-sided unit of `>=>`, and
 * `>=>` is associative -- are exactly this monoid's laws, which is the whole
 * reason to name the instance: it is the value-level statement of "a monad
 * is a monoid in the category of endofunctors", the only form of that
 * statement this library can make.
 *
 * `MONAD_OBJECT` is a type parameter, not a stored member: every typeclass
 * object in this library is stateless and empty, so default-constructing a
 * fresh `MONAD_OBJECT{}` wherever one is needed is free, not a workaround.
 *
 * `combine` builds its own `MONAD_OBJECT{}` inside the stored lambda, at the
 * moment the lambda is called, rather than storing the result of
 * `MONAD_OBJECT{}.kleisli(...)` directly. `Monad<Impl>::kleisli`'s derived
 * branch returns a closure that captures `self` by reference; storing that
 * closure here would capture a reference to a `MONAD_OBJECT{}` temporary
 * that no longer exists once `combine` has returned. Constructing the
 * temporary and invoking `kleisli` on it in the same full expression that
 * calls the result keeps the temporary alive for exactly as long as it is
 * used.
 */
template <class MONAD_OBJECT, class A>
struct kleisli_monoid {
    /** What `MONAD_OBJECT::pure` returns when applied to an `A`; the context
     * every arrow in this monoid returns into.
     */
    using result_type = decltype(std::declval<const MONAD_OBJECT &>().pure(
        std::declval<const A &>()));

    using value_type = std::function<result_type(const A &)>;

    auto identity() const -> value_type {
        return [](const A &a) -> result_type { return MONAD_OBJECT{}.pure(a); };
    }

    auto combine(const value_type &lhs, const value_type &rhs) const
        -> value_type {
        return [lhs, rhs](const A &a) -> result_type {
            return MONAD_OBJECT{}.kleisli(lhs, rhs)(a);
        };
    }
};

/** `CONTEXT` (e.g. `std::optional<int>`) as a monoid, lifted from an
 * instance `ELEMENT_MONOID` over its element type through an
 * `APPLICATIVE_OBJECT` over `CONTEXT`: identity is `pure(identity_A)`,
 * combine is `invoke(combine_A, ·, ·)`. The element instance defaults to
 * the registered Monoid of the element type and is held by value, so a
 * lift of `std::optional<int>` under addition is spelled
 * `lifted_monoid<OptionalApplicativeMap<int>, std::optional<int>,
 * sum_monoid<int>>` -- the context stays `std::optional<int>`, unwrapped.
 *
 * Uses `invoke`, not `ap`: `invoke` is the interface that works for every
 * context, including one that cannot hold a callable (`apply.hpp`'s
 * "Applicative pattern invariants" block), whereas `ap` requires the context
 * to be able to hold the (curried) combining function.
 *
 * `APPLICATIVE_OBJECT` is a type parameter for the same reason
 * `MONAD_OBJECT` is above: every typeclass object is stateless and empty, so
 * default-constructing a fresh one wherever this instance needs one costs
 * nothing.
 */
template <class APPLICATIVE_OBJECT, class CONTEXT,
          class ELEMENT_MONOID = Monoid<applicative_value_t<CONTEXT>>>
    requires monoid_object<ELEMENT_MONOID, applicative_value_t<CONTEXT>>
struct lifted_monoid {
    using A = applicative_value_t<CONTEXT>;
    using value_type = CONTEXT;

    ELEMENT_MONOID d_element{};

    auto identity() const -> CONTEXT {
        return APPLICATIVE_OBJECT{}.pure(d_element.identity());
    }

    auto combine(const CONTEXT &lhs, const CONTEXT &rhs) const -> CONTEXT {
        return APPLICATIVE_OBJECT{}.invoke(
            [this](const A &a, const A &b) { return d_element.combine(a, b); },
            lhs, rhs);
    }
};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_INDUCED_MONOID_HPP
