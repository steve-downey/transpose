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
// For the *real* deferred domain — transpose over actual std::execution (P2300)
// senders via beman.execution, with no type erasure — see exec.hpp. This toy
// remains only as a minimal, dependency-free teaching stand-in.

#include <beman/transpose/apply.hpp>

#include <functional>
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

/** Applicative typeclass instance for sender<T> with deferred semantics.
 *
 * pure(x) yields a ready sender of x. apply(sf, sa) yields a new sender that,
 * when run, runs both operands and applies the resulting function to the
 * resulting argument. No work happens until the composed sender is run.
 */
template <class T>
struct SenderApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        using U = remove_cvref_t<VALUE>;
        return sender<U>::ready(U(std::forward<VALUE>(value)));
    }

    template <class F, class A>
    auto apply(this auto &&, const sender<F> &functions,
               const sender<A> &arguments) {
        using Result =
            decltype(std::invoke(std::declval<F>(), std::declval<A>()));
        using U = remove_cvref_t<Result>;
        return sender<U>{[functions, arguments]() -> U {
            return std::invoke(functions.get(), arguments.get());
        }};
    }
};

/** Applicative map exposing pure and apply for sender<T>. */
template <class T>
struct SenderApplicativeMap : Applicative<SenderApplicativeImpl<T>> {
    using SenderApplicativeImpl<T>::apply;
    using SenderApplicativeImpl<T>::pure;
};

/** Registers SenderApplicativeMap as the Applicative instance for sender<T>. */
template <class T>
inline constexpr auto applicative_typeclass<sender<T>> =
    SenderApplicativeMap<T>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_SENDER_HPP
