// tests/beman/transpose/transpose_senders.test.cpp                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// The front door over genuine std::execution senders:
// `vector<sender<T>> -> sender<vector<T>>`, with nothing erased.
//
// Built only when BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE is ON. This is the
// stage collect-hook half of the claim P3200's second motivating domain
// makes. The generic half -- that the vector traversal prefers a native
// `collect` and knows nothing about what supplies one -- is in
// collect_hook.test.cpp and needs no dependency at all.
//
// WHAT THIS FILE PINS THAT IS NOT A SUCCESS. The free `traverse` reaches
// exactly one sender shape, because its policy concept demands `pure` return
// the context type and `pure` is always `just`. Both rows are asserted below
// so that docs/decisions.md#collect-hook is decided against measurements
// rather than against recollection.

#include "p2300_adapter.hpp"

#include <beman/execution/execution.hpp>

#include <beman/transpose/transpose.hpp>
#include <beman/transpose/traverse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = beman::execution;
namespace bt = beman::transpose;

using bt::examples::all_of_sender;

namespace {

/// A named callable rather than a lambda-expression, for the reason
/// `vector_append_t` is one: every lambda-expression is a distinct,
/// unrelated closure type, so a sender type named once at namespace scope
/// and rebuilt inside a test case would not be the same type.
struct scale_by_ten {
    auto operator()(int value) const -> int { return value * 10; }
};

using just_sender = decltype(ex::just(0));
using adapted_sender = decltype(ex::just(0) | ex::then(scale_by_ten{}));

/// A child that completes on its own thread after a delay, recording the
/// order it actually finished in. Held field by field rather than as a copy
/// of the sender, for the reason all_of.test.cpp records: inside the
/// sender's own definition the sender type is still incomplete.
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

/// A child that records when it is started, so laziness can be observed
/// rather than asserted of a counter nothing touches.
struct counting_sender {
    using sender_concept = ex::sender_tag;
    using signatures = ex::completion_signatures<ex::set_value_t(int)>;

    template <class...>
    static consteval auto get_completion_signatures() noexcept -> signatures {
        return {};
    }

    int value{0};
    std::atomic<int> *started{nullptr};

    template <class RECEIVER>
    struct operation {
        using operation_state_concept = ex::operation_state_tag;

        int value;
        std::atomic<int> *started;
        RECEIVER receiver;

        auto start() & noexcept -> void {
            started->fetch_add(1);
            ex::set_value(std::move(receiver), value);
        }
    };

    template <class RECEIVER>
    auto connect(RECEIVER receiver) const -> operation<RECEIVER> {
        return operation<RECEIVER>{value, started, std::move(receiver)};
    }
};

/// A child that fails during its own start.
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

/// Whether the free `traverse` accepts a function returning this shape of
/// sender. Spelled as a concept so both the yes and the no are assertions.
template <class F>
concept traversable_with = requires(std::vector<int> values, F function) {
    bt::traverse(function, std::move(values));
};

inline constexpr auto to_just = [](int value) { return ex::just(value); };
inline constexpr auto to_adapted = [](int value) {
    return ex::just(value) | ex::then(scale_by_ten{});
};

} // namespace

// -- deliverable 2: the front door returns the un-erased sender ----------

static_assert(
    std::is_same_v<decltype(bt::transpose(
                       std::declval<std::vector<just_sender>>())),
                   all_of_sender<just_sender>>);

static_assert(
    std::is_same_v<decltype(bt::transpose(
                       std::declval<std::vector<adapted_sender>>())),
                   all_of_sender<adapted_sender>>);

// Nothing about the result type is erased: it is spelled from the element
// sender, which is docs/decisions.md#erasure-boundary's own test applied at
// the front door rather than at `all_of`.
static_assert(!std::is_polymorphic_v<all_of_sender<adapted_sender>>);

// And the transposed sender sends the structure, not a wrapper of it.
static_assert(
    std::is_same_v<bt::applicative_value_t<all_of_sender<adapted_sender>>,
                   std::vector<int>>);

TEST_CASE("transpose: a vector of senders becomes a sender of the vector") {
    std::vector<just_sender> children;
    for (int value = 1; value != 5; ++value) {
        children.push_back(ex::just(value));
    }

    auto transposed = bt::transpose(std::move(children));
    auto result = ex::sync_wait(std::move(transposed));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values == std::vector<int>{1, 2, 3, 4});
}

TEST_CASE("transpose: an adapted sender is as good as a plain one") {
    // The shape the fold could never hold: `then` makes every pairwise
    // composition a new type. Through `collect` the element type is simply
    // what it is.
    std::vector<adapted_sender> children;
    for (int value = 1; value != 4; ++value) {
        children.push_back(ex::just(value) | ex::then(scale_by_ten{}));
    }

    auto result = ex::sync_wait(bt::transpose(std::move(children)));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values == std::vector<int>{10, 20, 30});
}

TEST_CASE("transpose: laziness survives the front door") {
    // The property the demo sender was introduced to show, now over senders
    // that have an operation state to start. Transposing connects nothing
    // and starts nothing; `sync_wait` does both.
    std::atomic<int> started{0};

    std::vector<counting_sender> children;
    for (int value = 1; value != 4; ++value) {
        children.push_back(counting_sender{value, &started});
    }

    auto transposed = bt::transpose(std::move(children));
    CHECK(started.load() == 0);

    auto result = ex::sync_wait(std::move(transposed));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values == std::vector<int>{1, 2, 3});
    CHECK(started.load() == 3);
}

TEST_CASE("transpose: result order is input order when completion order is "
          "not") {
    // The same property all_of asserts, restated at the front door: the
    // traversal's promise is positional shape preservation, and for senders
    // "order" is composition and result order, never completion order.
    constexpr int count = 8;

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

    auto result = ex::sync_wait(bt::transpose(std::move(children)));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values.size() == static_cast<std::size_t>(count));
    for (int index = 0; index != count; ++index) {
        CHECK(values[static_cast<std::size_t>(index)] == index);
    }

    // Completion order really was not input order, asserted so that a change
    // serializing the children shows up as this test no longer proving
    // anything.
    CHECK(rank[static_cast<std::size_t>(count - 1)].load() < rank[0].load());
}

TEST_CASE("transpose: a child's error reaches the caller") {
    std::vector<failing_sender> children;
    children.push_back(failing_sender{});
    children.push_back(failing_sender{});

    REQUIRE_THROWS_AS(ex::sync_wait(bt::transpose(std::move(children))),
                      std::runtime_error);
}

TEST_CASE("transpose: the empty structure is a sender of an empty vector") {
    auto result = ex::sync_wait(bt::transpose(std::vector<adapted_sender>{}));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values.empty());
}

TEST_CASE("transpose: a transposed structure composes again") {
    // `all_of_sender<S>` completes with exactly one value, so it is itself a
    // registered Applicative element and transposing a vector of them is
    // just another traversal. That is what makes `collect` an applicative
    // operation rather than a terminal one.
    std::vector<all_of_sender<just_sender>> rows;
    for (int row = 0; row != 3; ++row) {
        std::vector<just_sender> cells;
        for (int cell = 0; cell != 2; ++cell) {
            cells.push_back(ex::just(row * 10 + cell));
        }
        rows.push_back(bt::transpose(std::move(cells)));
    }

    static_assert(
        std::is_same_v<decltype(bt::transpose(std::move(rows))),
                       all_of_sender<all_of_sender<just_sender>>>);

    auto result = ex::sync_wait(bt::transpose(std::move(rows)));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values.size() == 3);
    CHECK(values[0] == std::vector<int>{0, 1});
    CHECK(values[1] == std::vector<int>{10, 11});
    CHECK(values[2] == std::vector<int>{20, 21});
}

// -- the free `traverse`, and the one shape it reaches -------------------
//
// docs/decisions.md#collect-hook. `applicative_object_for`, the policy
// concept `traverse` carries, adds `pure(element) -> same_as<CONTEXT>` to
// the deep object concept. One object serves every sender type and its
// `pure` is always `just`, so the refinement holds exactly where the
// caller's sender happens to BE `just`'s own type. Both rows are pinned:
// the front door `transpose` does not carry the refinement and reaches
// every shape, which is why the two disagree.

static_assert(bt::applicative_object<bt::examples::P2300ApplicativeMap,
                                     adapted_sender>);
static_assert(!bt::applicative_object_for<bt::examples::P2300ApplicativeMap,
                                          adapted_sender>);
static_assert(bt::applicative_object_for<bt::examples::P2300ApplicativeMap,
                                         just_sender>);

static_assert(traversable_with<decltype(to_just)>);
static_assert(!traversable_with<decltype(to_adapted)>);

static_assert(
    std::is_same_v<decltype(bt::traverse(to_just,
                                         std::declval<std::vector<int>>())),
                   all_of_sender<just_sender>>);

TEST_CASE("traverse: the one sender shape the policy concept admits") {
    auto result =
        ex::sync_wait(bt::traverse(to_just, std::vector<int>{1, 2, 3}));

    REQUIRE(result.has_value());
    const auto &[values] = *result;
    REQUIRE(values == std::vector<int>{1, 2, 3});
}
