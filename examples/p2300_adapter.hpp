// examples/p2300_adapter.hpp                                         -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXAMPLES_P2300_ADAPTER_HPP
#define BEMAN_TRANSPOSE_EXAMPLES_P2300_ADAPTER_HPP

// Non-normative evidence, not library surface. An Applicative object over
// genuine std::execution (P2300) senders, written against
// bemanproject/execution.
//
// WHY THIS EXISTS. `include/beman/transpose/sender.hpp` demonstrates the
// deferred domain with a std::function<T()> wrapper. That is enough to show
// the front door preserves laziness, and it exercises none of what makes a
// P2300 sender a sender: completion signatures, the error and stopped
// channels, environments, one-shot operation states. This adapter registers
// the same Applicative surface over real senders so the claim can be checked
// rather than asserted. It lives here, and behind an off-by-default CMake
// option, because anything under include/ becomes a dependency of every
// installation of this library and this must not be one.
//
// WHAT IT ESTABLISHES. `p2300_applicative` supplies only the pure + invoke
// basis; `map`, `zip_with`, `lift` and the rest are the library's own
// derivations, so exercising them exercises the CRTP base over a context it
// was not designed against. Values, the error channel, laziness and
// move-only values all compose (see p2300.test.cpp).
//
// WHAT IT DOES NOT ESTABLISH, AND WHY -- two limits found by writing it, both
// recorded at docs/decisions.md#p2300-front-door-shape:
//
//   1. THE FIRST LIMIT IS NOW HALF LIFTED -- see the registration at the
//      bottom of this header, added by stage sender-registration on
//      2026-09-13. There IS an `applicative_typeclass` registration here
//      now, keyed by concept, and `applicative_value` reads a sender's
//      element type from its completion signatures. What is still absent is
//      `transpose(vector<sender>)`: the vector traversal accumulates with
//      `accumulated = applicative.invoke(...)`, an assignment, so the
//      context type must be invariant under composition. It is for
//      `optional<vector<T>>`; it is not for a real sender, where
//      `then(when_all(acc, elem))` names a new type at every step.
//      This header originally continued "a runtime-sized traversal of real
//      senders needs a type-erased sender". That claim is now CONTESTED by
//      docs/decisions.md#runtime-arity-composition, which says it needs a
//      native n-ary composition instead, and it is settled by measurement at
//      stage all-of-algorithm -- not by either comment being more recent.
//      The toy sender<T> passes only because std::function<T()> is already
//      type-erased.
//   2. `just_error(...)` and `just_stopped()` cannot be operands: `when_all`
//      requires each child to have exactly one value completion, and those
//      have none. Error propagation is therefore shown with an operand that
//      can complete with a value and fails at run time, which is the honest
//      shape of the claim.

#include <beman/execution/execution.hpp>

#include "all_of.hpp"

#include <beman/transpose/apply.hpp>

#include <utility>
#include <vector>

namespace beman::transpose::examples {

namespace ex = ::beman::execution;

/** Applicative instance over P2300 senders: the pure + invoke basis.
 *
 * pure(x) is a sender already holding x. invoke(f, s...) runs the operands
 * and applies the plain function f to their results -- `when_all` for the
 * composition, `then` for the application. Nothing is started until the
 * composed sender is connected and started, so laziness is preserved by
 * construction rather than by care.
 *
 * `when_all` does not sequence its children's effects; it completes when all
 * of them have. That is the composition order the wording promises, and it
 * is the reason that promise is about the order results are assembled into
 * the structure, not the order in which effects run. A synchronous context
 * makes the two coincide; a concurrent one cannot.
 */
struct P2300ApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        return ex::just(std::forward<VALUE>(value));
    }

    template <class FUNCTION, class... SENDERS>
    auto invoke(this auto &&, FUNCTION &&function, SENDERS &&...senders) {
        return ex::when_all(std::forward<SENDERS>(senders)...) |
               ex::then(std::forward<FUNCTION>(function));
    }

    /** The range twin of `invoke`: compose a RUNTIME-SIZED collection of
     * operands into one context holding a vector of their values.
     *
     * docs/decisions.md#runtime-arity-composition. `invoke` is variadic, so
     * it cannot serve a structure whose size is not known until run time,
     * and the vector Traversable's left fold cannot either -- its
     * accumulator assignment needs the context type to be invariant under
     * composition, and no real sender is. `collect` moves the n-ary
     * knowledge to the one place that has it, this object, and leaves the
     * Traversable generic.
     *
     * It is optional in `applicative_object`: every other registered
     * instance composes type-stably and keeps the fold. Stage collect-hook
     * is what teaches the Traversable to prefer this when an object offers
     * it; until then nothing calls it but the tests. */
    template <class SENDER>
    auto collect(this auto &&, std::vector<SENDER> senders) {
        return all_of(std::move(senders));
    }
};

/** Applicative map over P2300 senders: the basis above, everything else
 * derived by the library's own CRTP base. */
struct P2300ApplicativeMap : Applicative<P2300ApplicativeImpl> {
    using P2300ApplicativeImpl::collect;
    using P2300ApplicativeImpl::invoke;
    using P2300ApplicativeImpl::pure;
};

inline constexpr P2300ApplicativeMap p2300_applicative{};

/** Exactly the senders that can be an Applicative element.
 *
 * `ex::sender<S>`, plus "exactly one value completion, of exactly one
 * argument", read under the empty environment. This is the arity `when_all`
 * imposes on its children and the arity an Applicative element needs, so it
 * is one restriction serving two masters rather than a convenience.
 *
 * THE EMPTY ENVIRONMENT IS SPELLED `env<>`. P2300 called it `empty_env`
 * until LWG replaced the dedicated type with the zero-argument case of the
 * `env<...>` template, and the pinned beman.execution exports only the newer
 * spelling. See docs/decisions.md#sender-value-type-reading.
 *
 * THE ARITY CHECK IS SFINAE-FRIENDLY, which is not obvious and was initially
 * recorded the other way round. `std::type_identity_t` is not variadic, so
 * naming `value_types_of_t<S, env<>, type_identity_t, type_identity_t>` for
 * a sender with two value arguments is ill-formed -- but ill-formed INSIDE A
 * requires-expression, which is a constraint failure and not a hard error.
 * `just(1, 2)`, `just()`, `just_error(...)` and `just_stopped()` therefore
 * report false here rather than diagnosing, which is what lets them be
 * simply unregistered. Measured, with each case pinned in p2300.test.cpp.
 */
template <class SENDER>
concept single_value_sender =
    ::beman::execution::sender<SENDER> &&
    ::beman::execution::sender_in<SENDER, ::beman::execution::env<>> &&
    requires {
        typename ::beman::execution::value_types_of_t<
            SENDER, ::beman::execution::env<>, ::std::type_identity_t,
            ::std::type_identity_t>;
    };

/** Whether `T` has a nested `value_type`, which is what the framework's
 * primary `applicative_value` path reads. Used only to keep this adapter's
 * specialization disjoint from that one; see the specialization below. */
template <class T, class = void>
inline constexpr bool has_value_type_member = false;

template <class T>
inline constexpr bool
    has_value_type_member<T, ::std::void_t<typename T::value_type>> = true;

} // namespace beman::transpose::examples

namespace beman::transpose {

/** The element type of a sender is what it SENDS.
 *
 * The only authoritative statement of that is the sender's completion
 * signatures, so this reads them rather than looking for a `value_type`
 * member. A sender that happened to expose a `value_type` would be stating a
 * coincidence, not its element type.
 *
 * Constrained to senders WITHOUT a `value_type` member so that this and the
 * framework's `void_t<typename T::value_type>` path stay disjoint -- they
 * would otherwise both match with a second argument of `void` and the
 * specialization would be ambiguous. No sender in beman.execution has a
 * `value_type` today, so the exclusion is currently vacuous. TRIPWIRE: if
 * this exclusion is ever the reason a real sender fails to register, the
 * disjointness assumption has broken and the keying decision's Why needs
 * revisiting rather than the constraint loosening.
 */
template <class SENDER>
struct applicative_value<
    SENDER, ::std::enable_if_t<examples::single_value_sender<SENDER> &&
                               !examples::has_value_type_member<SENDER>>> {
    using type = ::std::remove_cvref_t<::beman::execution::value_types_of_t<
        SENDER, ::beman::execution::env<>, ::std::type_identity_t,
        ::std::type_identity_t>>;
};

/** The Applicative object for every P2300 sender, keyed by CONCEPT.
 *
 * Every other instance in this library is keyed on a concrete carrier --
 * `optional<T>`, `zip_list<T>`, `array<T, N>` -- because there is a template
 * to key on. P2300 has none: `just(1)`, `just(1) | then(f)` and
 * `when_all(a, b) | then(g)` are unrelated types with no common template,
 * and there is no single context template `M` to write down. Keying by
 * concept is therefore the honest spelling, and it matches the duck-typing
 * posture the library already takes at use sites.
 *
 * ONE OBJECT FOR ALL SENDER TYPES, not one per `S`. That is what lets
 * `pure(x)` return `just(x)` -- a different sender type from whatever `S`
 * the object was found under -- without the object having to know it. An
 * object templated on `S` would have to promise that `pure` returns `S`,
 * which no sender applicative can keep.
 *
 * Senders outside `single_value_sender` are simply NOT REGISTERED: they fail
 * the constraint, this specialization does not apply, and the primary
 * template's `std::false_type` gives the framework's existing
 * "No applicative_typeclass<T> specialization found" diagnostic.
 *
 * SENTINEL (docs/decisions.md#sender-instance-keying): any
 * `applicative_typeclass<some_specific_adaptor_type>` specialization is a
 * regression to per-type keying. Ambiguity between this and any future
 * concept-keyed registration must be resolved by subsumption, never by
 * adding a tie-breaker tag.
 */
template <class SENDER>
    requires examples::single_value_sender<SENDER>
inline constexpr auto applicative_typeclass<SENDER> =
    examples::P2300ApplicativeMap{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_EXAMPLES_P2300_ADAPTER_HPP
