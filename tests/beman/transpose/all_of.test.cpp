// tests/beman/transpose/all_of.test.cpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// `when_all` at runtime arity, and the measurement the design rests on.
//
// Built only when BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE is ON. See
// examples/all_of.hpp for what this establishes, and
// docs/decisions.md#erasure-boundary for what "no wrapper materialized"
// was defined to mean: ONE allocation whose size is n times a compile-time
// constant, plus the result vector, and a result type spelled from S.
//
// THE MEASUREMENT IS NOT HERE. Counting allocations means replacing global
// `operator new`, and the ThreadSanitizer runtime defines those symbols
// itself -- the two cannot share a translation unit, and this file has the
// concurrency test that TSan exists to check. The measurement therefore
// lives in all_of_allocation.test.cpp, which TSan never builds.

#include "all_of.hpp"
#include "p2300_adapter.hpp"

#include <beman/execution/execution.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = beman::execution;
namespace ax = beman::transpose::examples;

using ax::all_of;

namespace {

using just_int = decltype(ex::just(1));

auto make_children(int count) -> std::vector<just_int> {
    std::vector<just_int> children;
    children.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index != count; ++index) {
        children.push_back(ex::just(index));
    }
    return children;
}

// -- 1. The type is spelled from S. ---------------------------------------
// Nothing is erased, so the caller keeps composing on the real thing. This
// is the half of the claim a type can carry; the allocation count below is
// the half only a measurement can.

static_assert(
    std::is_same_v<decltype(all_of(std::declval<std::vector<just_int>>())),
                   ax::all_of_sender<just_int>>);
static_assert(ex::sender<ax::all_of_sender<just_int>>);

// No virtual dispatch anywhere in the shape --
// docs/decisions.md#erasure-boundary forbids it by name.
static_assert(!std::is_polymorphic_v<ax::all_of_sender<just_int>>);

// The composition CLOSES: all_of's own result is itself a registered
// Applicative element, because it completes with exactly one value. So a
// transposed structure can be transposed again, and `collect` is a real
// applicative operation rather than a terminal one. Stage collect-hook needs
// this to hold; measured here rather than assumed there.
static_assert(ax::single_value_sender<ax::all_of_sender<just_int>>);
static_assert(
    std::is_same_v<
        beman::transpose::applicative_value_t<ax::all_of_sender<just_int>>,
        std::vector<int>>);
static_assert(
    !std::is_same_v<
        std::remove_const_t<decltype(beman::transpose::applicative_typeclass<
                                     ax::all_of_sender<just_int>>)>,
        std::false_type>);

// -- 2. Completion signatures, including the P3887R1 correction. ----------
// The plan listed set_stopped_t() unconditionally. P3887R1 (LWG-approved
// 2025-11) says when_all advertises set_stopped only if a child does, and
// all-of-failure-semantics defines all_of BY REFERENCE to when_all. So a
// vector of `just` senders -- none of which can stop -- must NOT advertise
// a stopped completion.

template <class SENDER>
concept advertises_stopped = ex::sends_stopped<SENDER, ex::env<>>;

static_assert(!advertises_stopped<just_int>);
static_assert(!advertises_stopped<ax::all_of_sender<just_int>>);

// A child that CAN stop makes the whole advertise it.
struct stoppable_sender {
    using sender_concept = ex::sender_tag;
    using signatures =
        ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>;

    template <class...>
    static consteval auto get_completion_signatures() noexcept -> signatures {
        return {};
    }

    std::atomic<bool> *observed_stop_request{nullptr};

    template <class RECEIVER>
    struct operation {
        using operation_state_concept = ex::operation_state_tag;

        std::atomic<bool> *observed;
        RECEIVER receiver;

        auto start() & noexcept -> void {
            const auto token = ex::get_stop_token(ex::get_env(receiver));
            if (token.stop_requested()) {
                if (observed != nullptr) {
                    observed->store(true);
                }
                ex::set_stopped(std::move(receiver));
                return;
            }
            ex::set_value(std::move(receiver), 1);
        }
    };

    template <class RECEIVER>
    auto connect(RECEIVER receiver) const -> operation<RECEIVER> {
        return operation<RECEIVER>{observed_stop_request, std::move(receiver)};
    }
};

static_assert(advertises_stopped<stoppable_sender>);
static_assert(advertises_stopped<ax::all_of_sender<stoppable_sender>>);

// A child that completes on its own thread after a staggered delay, so the
// children genuinely race. Written here rather than reached for from the
// dependency because get_parallel_scheduler's backend is not linked into
// this executable; what the test needs is real concurrency, and a thread
// per child supplies it without another build dependency.
struct threaded_sender {
    using sender_concept = ex::sender_tag;
    using signatures = ex::completion_signatures<ex::set_value_t(int)>;

    template <class...>
    static consteval auto get_completion_signatures() noexcept -> signatures {
        return {};
    }

    int index{0};
    std::chrono::microseconds delay{0};
    std::atomic<int> *finished{nullptr};
    std::vector<std::atomic<int>> *rank{nullptr};

    template <class RECEIVER>
    struct operation {
        using operation_state_concept = ex::operation_state_tag;

        // The configuration is held field by field rather than as a copy of
        // the sender: inside the sender's own definition the sender type is
        // still incomplete, so it cannot be a member of its nested state.
        int index;
        std::chrono::microseconds delay;
        std::atomic<int> *finished;
        std::vector<std::atomic<int>> *rank;
        RECEIVER receiver;
        std::optional<std::jthread> worker{};

        auto start() & noexcept -> void {
            worker.emplace([this] {
                std::this_thread::sleep_for(delay);
                (*rank)[static_cast<std::size_t>(index)].store(
                    finished->fetch_add(1));
                ex::set_value(std::move(receiver), index);
            });
        }
    };

    template <class RECEIVER>
    auto connect(RECEIVER receiver) const -> operation<RECEIVER> {
        return operation<RECEIVER>{index, delay, finished, rank,
                                   std::move(receiver)};
    }
};

// A child that fails during its own start, so the stop request it triggers
// reaches later children before they are started -- which makes the sibling
// observation below deterministic rather than a race.
struct failing_sender {
    using sender_concept = ex::sender_tag;
    using signatures =
        ex::completion_signatures<ex::set_value_t(int),
                                  ex::set_error_t(std::exception_ptr)>;

    template <class...>
    static consteval auto get_completion_signatures() noexcept -> signatures {
        return {};
    }

    template <class RECEIVER>
    struct operation {
        using operation_state_concept = ex::operation_state_tag;

        RECEIVER receiver;

        auto start() & noexcept -> void {
            ex::set_error(
                std::move(receiver),
                std::make_exception_ptr(std::runtime_error{"child failed"}));
        }
    };

    template <class RECEIVER>
    auto connect(RECEIVER receiver) const -> operation<RECEIVER> {
        return operation<RECEIVER>{std::move(receiver)};
    }
};

// A receiver whose environment carries a stop token the test controls.
template <class TOKEN>
struct token_receiver {
    using receiver_concept = ex::receiver_tag;

    TOKEN token;
    std::atomic<bool> *completed_value{nullptr};
    std::atomic<bool> *completed_stopped{nullptr};

    auto set_value(auto &&...) noexcept -> void {
        if (completed_value != nullptr) {
            completed_value->store(true);
        }
    }
    auto set_error(auto &&) noexcept -> void {}
    auto set_stopped() noexcept -> void {
        if (completed_stopped != nullptr) {
            completed_stopped->store(true);
        }
    }
    auto get_env() const noexcept {
        return ex::env{ex::prop{ex::get_stop_token, token}};
    }
};

} // namespace

TEST_CASE("all_of: results are in input order and the vector is the value") {
    auto result = ex::sync_wait(all_of(make_children(5)));

    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(values.size() == 5u);
    for (int index = 0; index != 5; ++index) {
        CHECK(values[static_cast<std::size_t>(index)] == index);
    }
}

TEST_CASE("all_of: nothing runs until the composed sender is started") {
    int started = 0;

    auto counting_child = [&started](int value) {
        ++started;
        return value;
    };

    std::vector<decltype(ex::just(1) | ex::then(counting_child))> children;
    children.reserve(3u);
    for (int index = 0; index != 3; ++index) {
        children.push_back(ex::just(index) | ex::then(counting_child));
    }

    auto composed = all_of(std::move(children));
    CHECK(started == 0);

    auto result = ex::sync_wait(std::move(composed));
    REQUIRE(result.has_value());
    CHECK(started == 3);
}

TEST_CASE("all_of: an empty structure completes immediately with an empty "
          "vector") {
    auto result = ex::sync_wait(all_of(make_children(0)));

    REQUIRE(result.has_value());

    const auto &[values] = *result;
    CHECK(values.empty());
}

TEST_CASE("all_of: one child is not a special case") {
    auto result = ex::sync_wait(all_of(make_children(1)));

    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(values.size() == 1u);
    CHECK(values[0] == 0);
}

TEST_CASE("all_of: result order is input order even when completion order is "
          "not") {
    // The children genuinely race on their own threads, with delays staggered
    // so that later children finish FIRST. The completion log records the
    // order they actually finished in; the result vector must still be in
    // input order, because transpose promises shape preservation and for a
    // vector that is positional.
    constexpr int count = 16;

    std::atomic<int> finished{0};
    std::vector<std::atomic<int>> rank(static_cast<std::size_t>(count));
    for (auto &entry : rank) {
        entry.store(-1);
    }

    std::vector<threaded_sender> children;
    children.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index != count; ++index) {
        children.push_back(threaded_sender{
            index, std::chrono::microseconds{(count - index) * 2000}, &finished,
            &rank});
    }

    auto result = ex::sync_wait(all_of(std::move(children)));

    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(values.size() == static_cast<std::size_t>(count));
    for (int index = 0; index != count; ++index) {
        CHECK(values[static_cast<std::size_t>(index)] == index);
    }

    CHECK(finished.load() == count);

    // Completion order really was not input order: the staggering means the
    // last child finished before the first. Asserted rather than assumed, so
    // that a scheduler change which serialized the children would show up as
    // this test no longer proving anything.
    CHECK(rank[static_cast<std::size_t>(count - 1)].load() < rank[0].load());
}

TEST_CASE("all_of: a child's error propagates and siblings observe the stop "
          "request") {
    // The failing child is first and fails during its own start, so the stop
    // request reaches the later children before they are started. That makes
    // the sibling observation deterministic rather than a race.
    std::atomic<bool> second_observed{false};
    std::atomic<bool> third_observed{false};

    std::vector<failing_sender> failing;
    failing.push_back(failing_sender{});

    REQUIRE_THROWS_AS(ex::sync_wait(all_of(std::move(failing))),
                      std::runtime_error);

    // Siblings observing the stop request: an already-requested outer token
    // reaches the children through all_of's own source.
    std::vector<stoppable_sender> children;
    children.push_back(stoppable_sender{&second_observed});
    children.push_back(stoppable_sender{&third_observed});

    ex::inplace_stop_source source;
    source.request_stop();

    std::atomic<bool> stopped{false};
    auto operation = ex::connect(all_of(std::move(children)),
                                 token_receiver<ex::inplace_stop_token>{
                                     source.get_token(), nullptr, &stopped});
    ex::start(operation);

    CHECK(second_observed.load());
    CHECK(third_observed.load());
    CHECK(stopped.load());
}

TEST_CASE("all_of: a move-only value composes and keeps its position") {
    std::vector<decltype(ex::just(std::make_unique<int>(0)))> children;
    children.reserve(4u);
    for (int index = 0; index != 4; ++index) {
        children.push_back(ex::just(std::make_unique<int>(index)));
    }

    static_assert(!std::is_copy_constructible_v<decltype(ex::just(
                      std::make_unique<int>(0)))>);

    auto result = ex::sync_wait(all_of(std::move(children)));

    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(values.size() == 4u);
    for (int index = 0; index != 4; ++index) {
        REQUIRE(values[static_cast<std::size_t>(index)] != nullptr);
        CHECK(*values[static_cast<std::size_t>(index)] == index);
    }
}

TEST_CASE("all_of: the applicative object offers it as the native collect") {
    // docs/decisions.md#runtime-arity-composition: collect is the range twin
    // of invoke, and for senders it IS all_of. The Traversable side of this
    // is stage collect-hook's; what is checked here is only that the object
    // supplies the operation and that it agrees with all_of.
    auto children = make_children(4);

    auto collected = ax::p2300_applicative.collect(make_children(4));

    static_assert(
        std::is_same_v<decltype(collected), ax::all_of_sender<just_int>>);

    auto result = ex::sync_wait(std::move(collected));
    REQUIRE(result.has_value());

    const auto &[values] = *result;
    REQUIRE(values.size() == 4u);
    for (int index = 0; index != 4; ++index) {
        CHECK(values[static_cast<std::size_t>(index)] == index);
    }
}
