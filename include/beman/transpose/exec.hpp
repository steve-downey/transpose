// include/beman/transpose/exec.hpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXEC_HPP
#define BEMAN_TRANSPOSE_EXEC_HPP

// Real-async context for Paper A's deferred domain, on top of beman.execution
// (a std::execution / P2300 implementation).
//
// transpose(vector<S>) -> sender<vector<T>>, where S is a real std::execution
// sender producing T. Because a std::vector is homogeneous, every element is
// the *same* concrete sender type S, so the result is a single concrete sender
// type and NO type erasure (no task<>, no std::function) is required.
//
// The generic vector traverse in sequence.hpp is a left fold
//   acc = invoke(append, acc, sender_i)
// whose accumulator type grows every step (when_all(acc, s) | then(...)), so a
// runtime-sized fold would only close over an erased type. We avoid that by
// registering a *sender-aware* vector Traversable (the constrained
// traversable_typeclass<vector<S>> below) that gathers the homogeneous children
// in one concrete sender, `gather_sender<S>`, rather than folding. Nothing runs
// until the gathered sender is run (e.g. via sync_wait).
//
// Non-normative demonstration support, not a proposed standard instance.
// Requires beman.execution on the include path; inert otherwise.

#if defined(__has_include)
#if __has_include(<beman/execution/execution.hpp>)
#define BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION 1
#endif
#endif

#if defined(BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION)

#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/traverse.hpp>

#include <beman/execution/execution.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace beman::transpose {

namespace ex = beman::execution;

namespace detail {

/** Single value type produced by a sender S (the T in `sender<T>`). */
template <class S>
using sender_value_t = ex::detail::single_sender_value_type<S, ex::env<>>;

template <class S, class Recv>
struct gather_op;

/** Internal receiver: one per child, knows its slot index and its op-state. */
template <class S, class Recv>
struct gather_receiver {
    using receiver_concept = ex::receiver_tag;
    gather_op<S, Recv> *op;
    std::size_t index;

    template <class V>
    void set_value(V &&value) && noexcept {
        op->complete_value(index, std::forward<V>(value));
    }
    void set_error(std::exception_ptr error) && noexcept {
        op->complete_error(std::move(error));
    }
    template <class E>
    void set_error(E &&error) && noexcept {
        op->complete_error(
            std::make_exception_ptr(std::forward<E>(error)));
    }
    void set_stopped() && noexcept { op->complete_stopped(); }
};

/** Holds one child sender's operation state at a stable heap address. The op
 * member is direct-initialized from the connect() prvalue (guaranteed copy
 * elision), so child op-states need not be movable. */
template <class S, class Recv>
struct gather_child {
    using receiver_t = gather_receiver<S, Recv>;
    using op_t = decltype(ex::connect(std::declval<S>(), std::declval<receiver_t>()));
    op_t op;

    gather_child(S sender, gather_op<S, Recv> *parent, std::size_t index)
        : op(ex::connect(std::move(sender), receiver_t{parent, index})) {}
};

/** Operation state for gather_sender. Immovable (atomics + children hold
 * `this`); materialized in place via guaranteed elision through connect(). */
template <class S, class Recv>
struct gather_op {
    using operation_state_concept = ex::operation_state_tag;
    using T = sender_value_t<S>;

    Recv recv;
    std::size_t n;
    std::vector<std::optional<T>> results;
    std::atomic<std::size_t> remaining;
    std::atomic<bool> has_error{false};
    std::atomic<bool> has_stopped{false};
    std::exception_ptr error{};
    std::vector<std::unique_ptr<gather_child<S, Recv>>> children;

    gather_op(std::vector<S> senders, Recv receiver)
        : recv(std::move(receiver)), n(senders.size()), results(n),
          remaining(n) {
        children.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            children.push_back(std::make_unique<gather_child<S, Recv>>(
                std::move(senders[i]), this, i));
        }
    }

    void start() & noexcept {
        if (n == 0) {
            ex::set_value(std::move(recv), std::vector<T>{});
            return;
        }
        for (auto &child : children) {
            ex::start(child->op);
        }
    }

    template <class V>
    void complete_value(std::size_t index, V &&value) noexcept {
        results[index].emplace(std::forward<V>(value));
        finish_one();
    }
    void complete_error(std::exception_ptr e) noexcept {
        bool expected = false;
        if (has_error.compare_exchange_strong(expected, true)) {
            error = std::move(e);
        }
        finish_one();
    }
    void complete_stopped() noexcept {
        has_stopped.store(true);
        finish_one();
    }

    void finish_one() noexcept {
        if (remaining.fetch_sub(1) != 1) {
            return; // not the last completion
        }
        if (has_error.load()) {
            ex::set_error(std::move(recv), std::move(error));
        } else if (has_stopped.load()) {
            ex::set_stopped(std::move(recv));
        } else {
            std::vector<T> out;
            out.reserve(n);
            for (auto &slot : results) {
                out.push_back(std::move(*slot));
            }
            ex::set_value(std::move(recv), std::move(out));
        }
    }
};

} // namespace detail

/** A concrete sender that transposes a homogeneous `vector<S>` of senders into
 * one sender of `vector<T>`: it starts every child, collects their results in
 * order, and completes with the vector (or the first error / stopped). No type
 * erasure: S is one concrete type, so this sender is one concrete type. */
template <class S>
struct gather_sender {
    using sender_concept = ex::sender_tag;
    using T = detail::sender_value_t<S>;

    std::vector<S> senders;

    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
        return ex::completion_signatures<ex::set_value_t(std::vector<T>),
                                         ex::set_error_t(std::exception_ptr),
                                         ex::set_stopped_t()>{};
    }

    template <class Recv>
    auto connect(Recv receiver) {
        return detail::gather_op<S, Recv>{std::move(senders),
                                          std::move(receiver)};
    }
};

/** Sender-aware vector Traversable. Maps each element through `function` to a
 * sender and gathers the homogeneous result into one `gather_sender`. The
 * applicative argument is unused: the gather IS the composition. */
template <class S>
struct VectorSenderTraversableImpl {
    using element_type = S;

    template <class APPLICATIVE, class FUNCTION>
    auto traverse(this auto &&, const APPLICATIVE &, FUNCTION &&function,
                  const std::vector<S> &values) {
        using Sender =
            remove_cvref_t<std::invoke_result_t<FUNCTION, const S &>>;
        std::vector<Sender> senders;
        senders.reserve(values.size());
        for (const auto &value : values) {
            senders.push_back(std::invoke(function, value));
        }
        return gather_sender<Sender>{std::move(senders)};
    }
};

template <class S>
struct VectorSenderTraversableMap
    : Traversable<VectorSenderTraversableImpl<S>> {
    using VectorSenderTraversableImpl<S>::traverse;
};

/** Sender-aware Traversable for `std::vector<S>` when S is a real sender. This
 * constrained specialization is more specialized than the generic
 * `traversable_typeclass<std::vector<VALUE_TYPE>>`, so vectors of senders route
 * here while every other vector keeps the generic instance. */
template <class S>
    requires ex::sender<S>
inline constexpr auto traversable_typeclass<std::vector<S>> =
    VectorSenderTraversableMap<S>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_HAS_BEMAN_EXECUTION

#endif // BEMAN_TRANSPOSE_EXEC_HPP
