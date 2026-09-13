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
//   1. There is no `applicative_typeclass` registration here, and no
//      `transpose(vector<sender>)`. The vector traversal accumulates with
//      `accumulated = applicative.invoke(...)`, an assignment, so the
//      context type must be invariant under composition. It is for
//      `optional<vector<T>>`; it is not for a real sender, where
//      `then(when_all(acc, elem), f)` names a new type at every step. A
//      runtime-sized traversal of real senders needs a type-erased sender,
//      which bemanproject/execution does not ship. The toy sender<T> passes
//      only because std::function<T()> is already type-erased.
//
//   2. `just_error(...)` and `just_stopped()` cannot be operands: `when_all`
//      requires each child to have exactly one value completion, and those
//      have none. Error propagation is therefore shown with an operand that
//      can complete with a value and fails at run time, which is the honest
//      shape of the claim.

#include <beman/execution/execution.hpp>

#include <beman/transpose/apply.hpp>

#include <utility>

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
};

/** Applicative map over P2300 senders: the basis above, everything else
 * derived by the library's own CRTP base. */
struct P2300ApplicativeMap : Applicative<P2300ApplicativeImpl> {
    using P2300ApplicativeImpl::invoke;
    using P2300ApplicativeImpl::pure;
};

inline constexpr P2300ApplicativeMap p2300_applicative{};

} // namespace beman::transpose::examples

#endif // BEMAN_TRANSPOSE_EXAMPLES_P2300_ADAPTER_HPP
