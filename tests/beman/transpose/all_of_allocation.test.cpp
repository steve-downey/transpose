// tests/beman/transpose/all_of_allocation.test.cpp                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// THE MEASUREMENT docs/decisions.md#erasure-boundary names as its sentinel.
//
// The claim under test is "no wrapper materialized". Its measurable form,
// fixed by that decision, is ONE allocation whose size is n times a
// compile-time constant, plus the result vector -- and nothing else.
//
// WHY THIS IS ITS OWN TRANSLATION UNIT. Counting allocations means replacing
// global `operator new`/`operator delete`. The ThreadSanitizer runtime
// defines those symbols itself, so a TU that replaces them does not link
// under TSan. Separating the measurement from all_of.test.cpp's concurrency
// test is what lets each be checked under the sanitizer it needs: this file
// under the address/UB sanitizers the debug presets use, and the behaviour
// file additionally under TSan. The guard below keeps this file harmless if
// it is ever built with TSan anyway.

#include "all_of.hpp"

#include <beman/execution/execution.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__SANITIZE_THREAD__)
#define BEMAN_TRANSPOSE_UNDER_TSAN 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define BEMAN_TRANSPOSE_UNDER_TSAN 1
#endif
#endif

namespace ex = beman::execution;
namespace ax = beman::transpose::examples;

#if !defined(BEMAN_TRANSPOSE_UNDER_TSAN)

namespace {

std::atomic<bool> counting{false};
std::atomic<std::size_t> allocation_count{0};
std::atomic<std::size_t> allocation_bytes{0};

/** Counting is on only inside the measured region, so Catch2's own
 * allocations and the fixture's do not enter the count. */
struct counting_scope {
    counting_scope() {
        allocation_count.store(0);
        allocation_bytes.store(0);
        counting.store(true);
    }
    ~counting_scope() { counting.store(false); }
};

} // namespace

void *operator new(std::size_t size) {
    if (counting.load(std::memory_order_relaxed)) {
        allocation_count.fetch_add(1);
        allocation_bytes.fetch_add(size);
    }
    void *pointer = std::malloc(size != 0u ? size : 1u);
    if (pointer == nullptr) {
        throw std::bad_alloc{};
    }
    return pointer;
}

void *operator new(std::size_t size, std::align_val_t alignment) {
    if (counting.load(std::memory_order_relaxed)) {
        allocation_count.fetch_add(1);
        allocation_bytes.fetch_add(size);
    }
    const auto step = static_cast<std::size_t>(alignment);
    const auto rounded = ((size + step - 1u) / step) * step;
    void *pointer = std::aligned_alloc(step, rounded != 0u ? rounded : step);
    if (pointer == nullptr) {
        throw std::bad_alloc{};
    }
    return pointer;
}

void *operator new[](std::size_t size) { return ::operator new(size); }

void operator delete(void *pointer) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::size_t) noexcept {
    std::free(pointer);
}
void operator delete(void *pointer, std::align_val_t) noexcept {
    std::free(pointer);
}
void operator delete(void *pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}
void operator delete[](void *pointer) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::size_t) noexcept {
    std::free(pointer);
}

namespace {

using just_int = decltype(ex::just(1));

struct discarding_receiver {
    using receiver_concept = ex::receiver_tag;
    auto set_value(std::vector<int> &&) noexcept -> void {}
    auto set_error(std::exception_ptr) noexcept -> void {}
    auto set_stopped() noexcept -> void {}
};

} // namespace

TEST_CASE("all_of: the cost is one block of n times a compile-time constant, "
          "plus the result") {
    // THE MEASUREMENT docs/decisions.md#erasure-boundary calls its sentinel.
    // The claim under test is "no wrapper materialized"; its measurable form
    // is one allocation sized n * sizeof(holder) with holder a
    // compile-time-known type, plus the result vector -- and nothing else.
    constexpr std::size_t count = 1000u;

    std::vector<just_int> children;
    children.reserve(count);
    for (std::size_t index = 0u; index != count; ++index) {
        children.push_back(ex::just(static_cast<int>(index)));
    }
    auto composed = ax::all_of(std::move(children));

    using operation_type =
        decltype(ex::connect(std::move(composed), discarding_receiver{}));
    using holder_type = typename operation_type::holder;

    // The block's size is a function of n and a compile-time constant, which
    // is the property the boundary decision states. It is known at connect
    // because connect_result_t is.
    static_assert(
        sizeof(holder_type) ==
        sizeof(std::optional<int>) +
            sizeof(ex::connect_result_t<
                   just_int, typename operation_type::element_receiver>));
    static_assert(!std::is_polymorphic_v<operation_type>);
    static_assert(!std::is_polymorphic_v<holder_type>);

    std::size_t after_connect_count = 0u;
    std::size_t after_connect_bytes = 0u;
    std::size_t after_start_count = 0u;

    {
        counting_scope scope;

        auto operation =
            ex::connect(std::move(composed), discarding_receiver{});

        after_connect_count = allocation_count.load();
        after_connect_bytes = allocation_bytes.load();

        ex::start(operation);

        after_start_count = allocation_count.load();
    }

    // ONE allocation at connect, holding every child operation state and
    // every result slot.
    CHECK(after_connect_count == 1u);
    CHECK(after_connect_bytes == count * sizeof(holder_type));

    // The children here complete synchronously inside start, so the result
    // vector is allocated during start. It is the ONLY other allocation, and
    // it is the one the boundary decision permits.
    CHECK(after_start_count - after_connect_count == 1u);
    CHECK(after_start_count == 2u);
}

#else

TEST_CASE("all_of: the allocation measurement does not run under TSan") {
    // Replacing operator new does not link against the TSan runtime. The
    // measurement runs under every other configuration; recorded here as a
    // skipped case rather than a silently absent one.
    SUCCEED("allocation counting is incompatible with ThreadSanitizer");
}

#endif
