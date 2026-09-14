// examples/all_of.hpp                                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_EXAMPLES_ALL_OF_HPP
#define BEMAN_TRANSPOSE_EXAMPLES_ALL_OF_HPP

// Non-normative evidence, not library surface. `when_all` at RUNTIME ARITY:
// a sender owning a std::vector of children, completing with a
// std::vector of their values.
//
// WHY THIS EXISTS. docs/decisions.md#runtime-arity-composition. The vector
// Traversable composes contexts with a left fold, `acc = invoke(f, acc, x)`,
// and that assignment requires the context type to be invariant under
// composition. It is for optional<vector<T>>. It is not for a real sender:
// `when_all(acc, elem) | then(f)` names a new type at every step, so the
// loop cannot hold the accumulator at all. The way through is not a
// cleverer fold but a native n-ary composition -- one algorithm that owns
// all n children at once -- which the applicative object then offers as
// `collect`, leaving the Traversable generic.
//
// WHAT IT IS MEASURED AGAINST. docs/decisions.md#erasure-boundary forbids
// std::function/move_only_function, any_sender/task-style wrappers, virtual
// dispatch, and per-element heap allocation of operation states. It permits
// exactly two allocations: ONE block holding the n child operation states
// and their result slots, whose size is `n * sizeof(holder)` with `holder`
// a compile-time-known type; and the result vector<T>. `all_of.test.cpp`
// counts both, separately, for n = 1000.
//
// Nothing here is type-erased. `all_of_sender<S>` is spelled from `S`, so a
// caller keeps composing on the real thing. This header includes no
// <functional>, declares no virtual function, and stores no callable.
//
// FAILURE SEMANTICS follow docs/decisions.md#all-of-failure-semantics, which
// is to say they follow `when_all`: the first error or stop to arrive
// requests stop on all siblings and is reported once every child has
// completed; error outranks stopped; among errors the first to ARRIVE wins.
// Result ORDER is input order, never completion order -- transpose promises
// shape preservation, and for a vector that is positional.
//
// It lives under examples/ and behind BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE
// because docs/decisions.md#p2300-front-door-shape says nothing an
// installation of this library pulls in may depend on beman.execution.

#include <beman/execution/execution.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace beman::transpose::examples {

namespace ex = ::beman::execution;

namespace detail_all_of {

/** The environment a child runs under: the outer environment with the stop
 * token replaced by the internal source's, so that a stop request reaches
 * siblings without reaching past this algorithm. The `when_all` pattern. */
inline constexpr auto make_child_env = [](const ex::inplace_stop_source &source,
                                          const auto &outer) noexcept {
    return ex::env{ex::prop{ex::get_stop_token, source.get_token()}, outer};
};

template <class ENV>
using child_env_t =
    decltype(make_child_env(::std::declval<const ex::inplace_stop_source &>(),
                            ::std::declval<const ENV &>()));

/** Append a completion signature only if it is not already present.
 * `set_error_t(exception_ptr)` is added unconditionally below, and a child
 * that already raises it must not make the set carry it twice. */
template <class SIGNATURES, class SIGNATURE>
struct add_signature;

template <class... PRESENT, class SIGNATURE>
struct add_signature<ex::completion_signatures<PRESENT...>, SIGNATURE> {
    using type =
        ::std::conditional_t<(::std::is_same_v<PRESENT, SIGNATURE> || ...),
                             ex::completion_signatures<PRESENT...>,
                             ex::completion_signatures<PRESENT..., SIGNATURE>>;
};

template <class SIGNATURES, class... SIGNATURE>
struct add_signatures;

template <class SIGNATURES>
struct add_signatures<SIGNATURES> {
    using type = SIGNATURES;
};

template <class SIGNATURES, class FIRST, class... REST>
struct add_signatures<SIGNATURES, FIRST, REST...>
    : add_signatures<typename add_signature<SIGNATURES, FIRST>::type, REST...> {
};

/** Every `set_error_t(E)` the child may raise, as a signature list. */
template <class... ERROR>
using errors_as_signatures =
    ex::completion_signatures<ex::set_error_t(ERROR)...>;

/** The element type a child sends, decayed -- read from its completion
 * signatures, per docs/decisions.md#sender-value-type-reading. */
template <class SENDER, class ENV>
using child_value_t = ::std::remove_cvref_t<ex::value_types_of_t<
    SENDER, ENV, ::std::type_identity_t, ::std::type_identity_t>>;

/** The composed signatures.
 *
 * `set_value_t(vector<T>)`; every `set_error_t(E)` of the child, plus
 * `set_error_t(exception_ptr)` because building the result vector and
 * moving elements into it may throw; and `set_stopped_t()` **only if a
 * child sends it**.
 *
 * That last condition is not what the execution plan originally said -- it
 * listed `set_stopped_t()` unconditionally. P3887R1, "Make `when_all` a
 * Ronseal Algorithm", was LWG-approved in 2025-11 and says `when_all`
 * advertises `set_stopped` only if a child does. Since
 * docs/decisions.md#all-of-failure-semantics defines this algorithm BY
 * REFERENCE to `when_all`, following the approved `when_all` is following
 * the decision; an `all_of` that hallucinates a stopped completion is not
 * `when_all` at runtime arity. The plan's text is corrected in place.
 */
template <class SENDER, class ENV>
struct signatures_for {
    using child_env = child_env_t<ENV>;

    using value_signature = ex::completion_signatures<ex::set_value_t(
        ::std::vector<child_value_t<SENDER, child_env>>)>;

    // add_signatures takes signatures, not a list; unwrap the child errors.
    template <class ACCUMULATED, class LIST>
    struct merge;

    template <class ACCUMULATED, class... SIGNATURE>
    struct merge<ACCUMULATED, ex::completion_signatures<SIGNATURE...>>
        : add_signatures<ACCUMULATED, SIGNATURE...> {};

    using with_errors = typename merge<
        value_signature,
        ex::error_types_of_t<SENDER, child_env, errors_as_signatures>>::type;

    using with_exception =
        typename add_signature<with_errors,
                               ex::set_error_t(::std::exception_ptr)>::type;

    using type = ::std::conditional_t<
        ex::sends_stopped<SENDER, child_env>,
        typename add_signature<with_exception, ex::set_stopped_t()>::type,
        with_exception>;
};

/** A type list with unique elements, used to build the error variant.
 *
 * The variant cannot simply be
 * `variant<monostate, exception_ptr, child errors...>`: a child that already
 * raises `exception_ptr` would give the variant two identical alternatives,
 * and `emplace<T>` on a variant with a duplicated alternative is deleted.
 * Deduplicating is not an optimization here, it is what makes the storage
 * usable at all. */
template <class... TYPE>
struct type_list {};

template <class LIST, class TYPE>
struct push_unique;

template <class... PRESENT, class TYPE>
struct push_unique<type_list<PRESENT...>, TYPE> {
    using type = ::std::conditional_t<(::std::is_same_v<PRESENT, TYPE> || ...),
                                      type_list<PRESENT...>,
                                      type_list<PRESENT..., TYPE>>;
};

template <class LIST, class... TYPE>
struct push_unique_all;

template <class LIST>
struct push_unique_all<LIST> {
    using type = LIST;
};

template <class LIST, class FIRST, class... REST>
struct push_unique_all<LIST, FIRST, REST...>
    : push_unique_all<typename push_unique<LIST, FIRST>::type, REST...> {};

template <class LIST>
struct as_variant;

template <class... TYPE>
struct as_variant<type_list<TYPE...>> {
    using type = ::std::variant<TYPE...>;
};

/** Which completion the whole is going to make. `started` means no child has
 * failed or stopped yet. Error outranks stopped, so `set_error` exchanges
 * unconditionally while `set_stopped` only claims the slot from `started` --
 * the asymmetry P2300's own `when_all` uses, and the reason a stopped child
 * cannot mask a later error. */
enum class disposition { started, error, stopped };

} // namespace detail_all_of

template <class SENDER>
struct all_of_sender;

/** The operation state: n children, one allocation, never moved. */
template <class SENDER, class RECEIVER>
struct all_of_operation {
    using operation_state_concept = ex::operation_state_tag;

    using outer_env = ex::env_of_t<RECEIVER>;
    using child_env = detail_all_of::child_env_t<outer_env>;
    using value_type = detail_all_of::child_value_t<SENDER, child_env>;

    /** The receiver each child is connected to. Holds a pointer and an
     * index: two words, no callable, nothing erased. */
    struct element_receiver {
        using receiver_concept = ex::receiver_tag;

        all_of_operation *operation;
        ::std::size_t index;

        auto set_value(auto &&...value) noexcept -> void {
            operation->element_value(index,
                                     ::std::forward<decltype(value)>(value)...);
        }

        template <class ERROR>
        auto set_error(ERROR &&error) noexcept -> void {
            operation->element_error(::std::forward<ERROR>(error));
        }

        auto set_stopped() noexcept -> void { operation->element_stopped(); }

        auto get_env() const noexcept -> child_env {
            return detail_all_of::make_child_env(
                operation->stop_source_, ex::get_env(operation->receiver_));
        }
    };

    /** One element of the single allocation: the child's result slot sitting
     * immediately beside the child's operation state.
     *
     * Colocating them is what keeps the budget at one block. A separate
     * `vector<optional<T>>` for the slots would be a second allocation
     * before the result vector, which docs/decisions.md#erasure-boundary
     * does not permit and the Stage 2 tripwire stops on. The operation
     * state is immovable, so this is constructed in place and never
     * relocated: `connect` is called in the mem-initializer, where
     * guaranteed elision initializes `operation` directly. */
    struct holder {
        ::std::optional<value_type> value{};
        ex::connect_result_t<SENDER, element_receiver> operation;

        holder(all_of_operation *parent, ::std::size_t position, SENDER &&child)
            : operation(ex::connect(::std::move(child),
                                    element_receiver{parent, position})) {}

        holder(const holder &) = delete;
        holder(holder &&) = delete;
        auto operator=(const holder &) -> holder & = delete;
        auto operator=(holder &&) -> holder & = delete;
        ~holder() = default;
    };

    /** Forwarding the outer stop request inward. A plain struct holding one
     * pointer: a stop callback is a place `std::function` would be reached
     * for, and docs/decisions.md#erasure-boundary forbids it here. */
    struct on_stop_request {
        ex::inplace_stop_source *source;
        auto operator()() const noexcept -> void { source->request_stop(); }
    };

    using stop_callback =
        ex::stop_callback_for_t<ex::stop_token_of_t<outer_env>,
                                on_stop_request>;

    RECEIVER receiver_;
    ::std::size_t count_;
    holder *holders_;

    ::std::atomic<::std::size_t> remaining_;
    ::std::atomic<detail_all_of::disposition> disposition_{
        detail_all_of::disposition::started};
    ex::inplace_stop_source stop_source_{};
    ::std::optional<stop_callback> stop_callback_{};

    /** The error the whole will report, if any. `monostate` is the "no error
     * recorded" state; `exception_ptr` is always admissible because building
     * the result vector can throw. Alternatives are deduplicated -- a child
     * that itself raises `exception_ptr` must not give the variant two
     * identical alternatives, which would delete `emplace<T>`. */
    template <class... ERROR>
    using error_storage = typename detail_all_of::as_variant<
        typename detail_all_of::push_unique_all<
            detail_all_of::type_list<::std::monostate, ::std::exception_ptr>,
            ERROR...>::type>::type;

    using error_variant =
        ex::error_types_of_t<SENDER, child_env, error_storage>;

    error_variant error_{};

    template <class R>
    all_of_operation(::std::vector<SENDER> &&children, R &&receiver)
        : receiver_(::std::forward<R>(receiver)), count_(children.size()),
          holders_(count_ == 0u ? nullptr
                                : static_cast<holder *>(::operator new(
                                      count_ * sizeof(holder),
                                      ::std::align_val_t{alignof(holder)}))),
          remaining_(count_ + 1u) {
        // THE ONE ALLOCATION, made here, at connect, sized n * a
        // compile-time constant. Every child operation state is constructed
        // into it in place and destroyed in place; none is ever moved.
        ::std::size_t constructed = 0u;
        try {
            for (; constructed != count_; ++constructed) {
                ::std::construct_at(holders_ + constructed, this, constructed,
                                    ::std::move(children[constructed]));
            }
        } catch (...) {
            while (constructed != 0u) {
                ::std::destroy_at(holders_ + --constructed);
            }
            ::operator delete(holders_, ::std::align_val_t{alignof(holder)});
            throw;
        }
    }

    all_of_operation(const all_of_operation &) = delete;
    all_of_operation(all_of_operation &&) = delete;
    auto operator=(const all_of_operation &) -> all_of_operation & = delete;
    auto operator=(all_of_operation &&) -> all_of_operation & = delete;

    ~all_of_operation() {
        for (::std::size_t position = count_; position != 0u;) {
            ::std::destroy_at(holders_ + --position);
        }
        if (holders_ != nullptr) {
            ::operator delete(holders_, ::std::align_val_t{alignof(holder)});
        }
    }

    auto start() & noexcept -> void {
        if (count_ == 0u) {
            // Nothing to wait for. An empty structure transposes to an empty
            // one held in context, which is the identity every other carrier
            // already obeys; P4217R1 admits the same case for `when_all` and
            // for the same reason -- forbidding it makes generic algorithms
            // carry a special case.
            ex::set_value(::std::move(receiver_), ::std::vector<value_type>{});
            return;
        }

        stop_callback_.emplace(ex::get_stop_token(ex::get_env(receiver_)),
                               on_stop_request{&stop_source_});

        // Left to right, per docs/decisions.md#applicative-objects. A child
        // may complete synchronously and inside its own `start`, so the
        // count begins at n + 1 and this loop's own arrival is the +1:
        // without it a synchronous first child could finish the whole before
        // the second is started.
        for (::std::size_t position = 0u; position != count_; ++position) {
            ex::start(holders_[position].operation);
        }

        arrive();
    }

  private:
    template <class... VALUE>
    auto element_value(::std::size_t position, VALUE &&...value) noexcept
        -> void {
        if (disposition_.load(::std::memory_order_relaxed) ==
            detail_all_of::disposition::started) {
            try {
                holders_[position].value.emplace(
                    ::std::forward<VALUE>(value)...);
            } catch (...) {
                record_error(::std::current_exception());
            }
        }
        arrive();
    }

    template <class ERROR>
    auto element_error(ERROR &&error) noexcept -> void {
        record_error(::std::forward<ERROR>(error));
        arrive();
    }

    auto element_stopped() noexcept -> void {
        auto expected = detail_all_of::disposition::started;
        if (disposition_.compare_exchange_strong(
                expected, detail_all_of::disposition::stopped)) {
            stop_source_.request_stop();
        }
        arrive();
    }

    template <class ERROR>
    auto record_error(ERROR &&error) noexcept -> void {
        // `exchange`, not a compare-exchange: an error claims the slot even
        // from a recorded `stopped`, which is how error outranks stopped.
        // The first error to arrive is the one kept, because a second
        // exchange sees `error` already there.
        if (disposition_.exchange(detail_all_of::disposition::error) !=
            detail_all_of::disposition::error) {
            try {
                error_.template emplace<::std::remove_cvref_t<ERROR>>(
                    ::std::forward<ERROR>(error));
            } catch (...) {
                error_.template emplace<::std::exception_ptr>(
                    ::std::current_exception());
            }
            stop_source_.request_stop();
        }
    }

    auto arrive() noexcept -> void {
        if (remaining_.fetch_sub(1u, ::std::memory_order_acq_rel) == 1u) {
            finish();
        }
    }

    auto finish() noexcept -> void {
        stop_callback_.reset();

        switch (disposition_.load(::std::memory_order_relaxed)) {
        case detail_all_of::disposition::started:
            complete_with_values();
            return;
        case detail_all_of::disposition::error:
            complete_with_error();
            return;
        case detail_all_of::disposition::stopped:
            if constexpr (requires {
                              ex::set_stopped(::std::move(receiver_));
                          }) {
                ex::set_stopped(::std::move(receiver_));
            }
            return;
        }
    }

    auto complete_with_values() noexcept -> void {
        try {
            // THE SECOND ALLOCATION, and the only other one: the result.
            // Reserved once, so the vector does not grow.
            ::std::vector<value_type> values;
            values.reserve(count_);
            for (::std::size_t position = 0u; position != count_; ++position) {
                values.push_back(::std::move(*holders_[position].value));
            }
            ex::set_value(::std::move(receiver_), ::std::move(values));
        } catch (...) {
            ex::set_error(::std::move(receiver_), ::std::current_exception());
        }
    }

    auto complete_with_error() noexcept -> void {
        ::std::visit(
            [this]<class ERROR>(ERROR &&error) noexcept -> void {
                if constexpr (::std::is_same_v<::std::remove_cvref_t<ERROR>,
                                               ::std::monostate>) {
                    // Unreachable: the disposition is only `error` after an
                    // error was stored. Completing with a synthesized
                    // exception rather than terminating keeps the receiver's
                    // contract if it ever is reached.
                    ex::set_error(::std::move(receiver_),
                                  ::std::make_exception_ptr(
                                      ::std::logic_error{"all_of: no error"}));
                } else {
                    ex::set_error(::std::move(receiver_),
                                  ::std::forward<ERROR>(error));
                }
            },
            ::std::move(error_));
    }

    friend struct all_of_sender<SENDER>;
};

/** A sender over a runtime-sized collection of senders.
 *
 * The type is spelled from `SENDER`. Nothing is erased, so the caller keeps
 * composing on the real thing -- which is the half of the "no wrapper
 * materialized" claim that a type can carry. The other half is the
 * allocation count, and only a test can carry that. */
template <class SENDER>
struct all_of_sender {
    using sender_concept = ex::sender_tag;

    ::std::vector<SENDER> children_;

    template <class SELF, class... ENV>
    static consteval auto get_completion_signatures() noexcept {
        if constexpr (sizeof...(ENV) == 0u) {
            return typename detail_all_of::signatures_for<SENDER,
                                                          ex::env<>>::type{};
        } else {
            return
                typename detail_all_of::signatures_for<SENDER, ENV...>::type{};
        }
    }

    template <class RECEIVER>
    auto connect(RECEIVER &&receiver)
        && -> all_of_operation<SENDER, ::std::remove_cvref_t<RECEIVER>> {
        return all_of_operation<SENDER, ::std::remove_cvref_t<RECEIVER>>(
            ::std::move(children_), ::std::forward<RECEIVER>(receiver));
    }

    template <class RECEIVER>
        requires ::std::copy_constructible<SENDER>
    auto connect(RECEIVER &&receiver)
        const & -> all_of_operation<SENDER, ::std::remove_cvref_t<RECEIVER>> {
        auto copy = children_;
        return all_of_operation<SENDER, ::std::remove_cvref_t<RECEIVER>>(
            ::std::move(copy), ::std::forward<RECEIVER>(receiver));
    }
};

/** `when_all` at runtime arity.
 *
 * The sender OWNS its children: a vector handed in is moved from, and a
 * range is materialized into one. There is no borrowing option, because an
 * operation state that outlives the expression that built it cannot hold
 * references to senders that do not. */
template <class SENDER>
auto all_of(::std::vector<SENDER> children) -> all_of_sender<SENDER> {
    return all_of_sender<SENDER>{::std::move(children)};
}

template <class RANGE>
    requires(!::std::is_same_v<
                ::std::remove_cvref_t<RANGE>,
                ::std::vector<::std::ranges::range_value_t<RANGE>>>) &&
            ex::sender<::std::ranges::range_value_t<RANGE>>
auto all_of(RANGE &&range)
    -> all_of_sender<::std::ranges::range_value_t<RANGE>> {
    using sender_type = ::std::ranges::range_value_t<RANGE>;
    ::std::vector<sender_type> children;
    if constexpr (::std::ranges::sized_range<RANGE>) {
        children.reserve(::std::ranges::size(range));
    }
    for (auto &&child : range) {
        children.push_back(::std::forward<decltype(child)>(child));
    }
    return all_of_sender<sender_type>{::std::move(children)};
}

} // namespace beman::transpose::examples

#endif // BEMAN_TRANSPOSE_EXAMPLES_ALL_OF_HPP
