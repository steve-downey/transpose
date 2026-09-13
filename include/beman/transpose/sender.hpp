// include/beman/transpose/sender.hpp                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_SENDER_HPP
#define BEMAN_TRANSPOSE_SENDER_HPP

// Non-normative demonstration type. sender models a deferred/asynchronous
// computational context for Paper A's second motivating domain. It is a
// deliberately minimal stand-in for a std::execution sender (P2300): a lazy
// value whose computation runs only when get() is called. Traversing a
// structure of senders transposes into a single sender of the structure, e.g.
// vector<sender<T>> becomes sender<vector<T>>, with no work performed until the
// resulting sender is run. It is illustrative, not a proposed standard type.
//
// WHAT IT SHOWS, PRECISELY. That the front door preserves laziness for a
// copyable, single-value deferred computation, and that it does so through
// the same traverse and transpose a caller uses for optional. That is the
// whole of it. This type has no completion signatures, no error or stopped
// channel, no environment or scheduler, no one-shot operation state -- none
// of what makes a P2300 sender a sender. The Applicative surface is checked
// against genuine senders separately, at examples/p2300_adapter.hpp.
//
// AND WHAT IT HIDES. Being a std::function<T()> wrapper makes this type
// invariant under composition: sender<vector<T>> composed with sender<T> is
// again sender<vector<T>>, so the vector traversal's accumulator loop
// type-checks. A real sender is not invariant -- every combinator names a new
// type -- so transposing a runtime-sized structure of real senders is not
// merely slower here, it cannot be written as a loop at all. The convenience
// that makes this a good demonstration is the same property that conceals
// that limit. See docs/decisions.md#p2300-front-door-shape.

#include <beman/transpose/apply.hpp>

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

/** A minimal deferred (lazy) value: the wrapped computation runs on get().
 * @tparam T the eventual result type
 */
template <class T>
struct sender {
    using value_type = T;

    std::function<T()> run;

    /** Runs the deferred computation and returns its result. */
    auto get() const -> T { return run(); }

    /** Build a sender that is already ready with @p value. */
    static auto ready(T value) -> sender {
        return sender{[value = std::move(value)]() -> T { return value; }};
    }

    /** Equality runs both deferred computations and compares the results. */
    friend auto operator==(const sender &left, const sender &right) -> bool
        requires requires(const T &a, const T &b) {
            { a == b } -> std::convertible_to<bool>;
        }
    {
        return left.get() == right.get();
    }
};

/** Invoke-native Applicative instance for sender<T> with deferred semantics.
 *
 * pure(x) yields a ready sender of x. invoke(f, s1, ..., sn) yields a single
 * new sender that, when run, runs every operand and applies the plain
 * function f to the results -- one deferred call capturing everything, so
 * laziness is preserved by construction. No work happens until the composed
 * sender is run.
 */
template <class T>
struct SenderApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        using U = remove_cvref_t<VALUE>;
        return sender<U>::ready(U(std::forward<VALUE>(value)));
    }

    /** N-ary deferred core; SFINAE-friendly via the trailing return type.
     *
     * The operands must be captured, since the point is to defer running
     * them, but a caller who is finished with an operand can hand it over:
     * they are deduced through forwarding references and moved into the
     * capture. A traversal's accumulator is an rvalue at every step, so this
     * is one copy per element saved rather than a micro-optimization.
     */
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function, sender<FIRST> first,
                sender<REST>... rest)
        -> sender<remove_cvref_t<std::invoke_result_t<
            const remove_cvref_t<FUNCTION> &, FIRST, REST...>>> {
        using U = remove_cvref_t<std::invoke_result_t<
            const remove_cvref_t<FUNCTION> &, FIRST, REST...>>;
        return sender<U>{
            [function =
                 remove_cvref_t<FUNCTION>(std::forward<FUNCTION>(function)),
             first = std::move(first), ... rest = std::move(rest)]() -> U {
                // Running the operands as arguments to std::invoke would leave
                // their relative order unspecified -- the calls are
                // indeterminately sequenced, and GCC 16 runs them in reverse.
                // That cannot implement the composition order the traversal
                // promises. The initializer-clauses of a braced-init-list are
                // sequenced left to right ([dcl.init.list]/4), so running the
                // operands into a tuple first pins the order, and std::apply
                // then applies the plain function to the results.
                std::tuple<FIRST, REST...> values{first.get(), rest.get()...};
                return std::apply(
                    [&function](auto &&...value) -> U {
                        return std::invoke(
                            function, std::forward<decltype(value)>(value)...);
                    },
                    std::move(values));
            }};
    }
};

/** Applicative map exposing pure and the n-ary invoke core for sender<T>. */
template <class T>
struct SenderApplicativeMap : Applicative<SenderApplicativeImpl<T>> {
    using SenderApplicativeImpl<T>::invoke;
    using SenderApplicativeImpl<T>::pure;
};

/** Registers SenderApplicativeMap as the Applicative instance for sender<T>. */
template <class T>
inline constexpr auto applicative_typeclass<sender<T>> =
    SenderApplicativeMap<T>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_SENDER_HPP
