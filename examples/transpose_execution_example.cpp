// examples/transpose_execution_example.cpp                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The receipts. `examples/transpose_example.cpp` shows the second motivating
// domain over `beman::transpose::sender<T>`, a `std::function<T()>` thunk: it
// proves the typeclass story -- three unrelated types answer to one verb --
// and nothing at all about cost or concurrency, because there is no
// concurrency in it to have.
//
// This one runs the same front door over genuine std::execution senders.
// Three scenes:
//
//   1. Children completing on their own threads, out of order. The result is
//      in input order anyway, and both orders are printed side by side.
//   2. A single-threaded deferred queue. Transposing connects nothing and
//      starts nothing; the work happens when the queue is drained, on this
//      thread.
//   3. One child fails. The error reaches the caller; the siblings are asked
//      to stop.
//
// Built only when BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE is ON.

#include "p2300_adapter.hpp"

#include <beman/execution/execution.hpp>

#include <beman/transpose/transpose.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ex = beman::execution;
namespace bt = beman::transpose;

namespace {

// -- scene 1: children that really do run concurrently -------------------

// -- scene 2: a single-threaded deferred queue ---------------------------

/// Somewhere for the transposed sender's result to land, so that scene 2 can
/// connect and start by hand rather than blocking in `sync_wait`. Blocking is
/// what a single-threaded scene cannot do: there would be no other thread to
/// drain the queue.
struct collecting_receiver {
    using receiver_concept = ex::receiver_tag;

    std::optional<std::vector<int>> *result;
    ex::run_loop *loop;

    auto set_value(std::vector<int> values) noexcept -> void {
        *result = std::move(values);
        loop->finish();
    }
    auto set_error(auto &&) noexcept -> void { loop->finish(); }
    auto set_stopped() noexcept -> void { loop->finish(); }
    auto get_env() const noexcept -> ex::env<> { return {}; }
};

/** Make one child for scene 1. Keeping the lambda expression in one function
 * gives every element the same concrete sender type, as a vector requires. */
auto make_parallel_child(ex::parallel_scheduler scheduler, int index, int count,
                         std::vector<int> &log, std::mutex &log_mutex) {
    return ex::schedule(scheduler) | ex::then([index, count, &log, &log_mutex] {
               std::this_thread::sleep_for(
                   std::chrono::milliseconds{(count - index) * 20});
               {
                   const std::lock_guard<std::mutex> guard{log_mutex};
                   log.push_back(index);
               }
               return index * index;
           });
}

// -- scene 3: a child that fails -----------------------------------------

struct failing_child {
    using sender_concept = ex::sender_tag;
    using signatures =
        ex::completion_signatures<ex::set_value_t(int),
                                  ex::set_error_t(std::exception_ptr)>;

    template <class...>
    static consteval auto get_completion_signatures() noexcept -> signatures {
        return {};
    }

    int index{0};
    bool fails{false};
    std::atomic<int> *stopped_observed{nullptr};

    template <class RECEIVER>
    struct operation {
        using operation_state_concept = ex::operation_state_tag;

        int index;
        bool fails;
        std::atomic<int> *stopped_observed;
        RECEIVER receiver;

        auto start() & noexcept -> void {
            if (fails) {
                ex::set_error(std::move(receiver),
                              std::make_exception_ptr(std::runtime_error{
                                  "child " + std::to_string(index) +
                                  " could not do the work"}));
                return;
            }
            // A sibling asks whether it still needs to bother. `all_of`
            // requests stop on the others as soon as one fails, and this is
            // where that request is visible.
            if (ex::get_stop_token(ex::get_env(receiver)).stop_requested()) {
                stopped_observed->fetch_add(1);
                ex::set_stopped(std::move(receiver));
                return;
            }
            ex::set_value(std::move(receiver), index);
        }
    };

    template <class RECEIVER>
    auto connect(RECEIVER receiver) const -> operation<RECEIVER> {
        return operation<RECEIVER>{index, fails, stopped_observed,
                                   std::move(receiver)};
    }
};

auto print_order(const std::string &label, const std::vector<int> &values)
    -> void {
    std::cout << label;
    for (const auto value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';
}

} // namespace

int main() {
    // -- Scene 1 ---------------------------------------------------------
    //
    // Six children on the parallel scheduler. transpose gives back one sender
    // of the whole vector; the values come out in input order however the
    // workers finish.
    {
        constexpr int count = 6;

        std::vector<int> completion_log;
        std::mutex log_mutex;
        auto scheduler = ex::get_parallel_scheduler();

        using child_type = decltype(make_parallel_child(
            scheduler, 0, count, completion_log, log_mutex));
        std::vector<child_type> children;
        children.reserve(static_cast<std::size_t>(count));
        for (int index = 0; index != count; ++index) {
            children.push_back(make_parallel_child(scheduler, index, count,
                                                   completion_log, log_mutex));
        }

        // One sender of a vector, not a vector of senders. Its type is
        // spelled from the child's type -- nothing erased, no std::function,
        // no virtual call.
        static_assert(
            std::is_same_v<decltype(bt::transpose(std::move(children))),
                           bt::examples::all_of_sender<child_type>>);

        auto [values] = *ex::sync_wait(bt::transpose(std::move(children)));

        std::cout << "scene 1 -- concurrent children\n";
        print_order("  completion order:", completion_log);
        print_order("  result order:    ", values);
        std::cout << "  (the results are the squares, in input order;"
                     " the log is the order they finished)\n\n";
    }

    // -- Scene 2 ---------------------------------------------------------
    //
    // The same front door with no concurrency at all. Transposing builds a
    // sender and runs nothing; starting it only enqueues work in a run_loop;
    // running that loop on this thread is what performs the work.
    {
        ex::run_loop loop;
        auto scheduler = loop.get_scheduler();
        int produced = 0;
        std::optional<std::vector<int>> result;

        auto make_child = [scheduler, &produced](int value) mutable {
            return ex::schedule(scheduler) | ex::then([value, &produced] {
                       ++produced;
                       return value;
                   });
        };
        using child_type = decltype(make_child(0));
        std::vector<child_type> children;
        for (int value = 1; value != 5; ++value) {
            children.push_back(make_child(value * 100));
        }

        auto composed = bt::transpose(std::move(children));

        std::cout << "scene 2 -- single-threaded, still lazy\n";
        std::cout << "  after transpose: " << produced << " values produced\n";

        auto operation = ex::connect(std::move(composed),
                                     collecting_receiver{&result, &loop});
        ex::start(operation);

        std::cout << "  after start:     " << produced
                  << " values produced, result "
                  << (result.has_value() ? "ready" : "not ready") << '\n';

        // run() is the only thing that executes the scheduled work, and the
        // collecting receiver finishes the loop after the last child.
        loop.run();

        std::cout << "  after run:       " << produced
                  << " values produced, result "
                  << (result.has_value() ? "ready" : "not ready") << '\n';
        print_order("  values:         ", result.value());
        std::cout << '\n';
    }

    // -- Scene 3 ---------------------------------------------------------
    //
    // A child fails. The first error wins, the siblings are asked to stop,
    // and the error is what the caller sees.
    {
        std::atomic<int> stopped_observed{0};

        std::vector<failing_child> children;
        children.push_back(failing_child{0, true, &stopped_observed});
        for (int index = 1; index != 5; ++index) {
            children.push_back(failing_child{index, false, &stopped_observed});
        }

        std::cout << "scene 3 -- a child fails\n";
        try {
            auto composed = bt::transpose(std::move(children));
            [[maybe_unused]] auto ignored = ex::sync_wait(std::move(composed));
            std::cout << "  no error reached the caller -- unexpected\n";
        } catch (const std::runtime_error &error) {
            std::cout << "  caught: " << error.what() << '\n';
        }
        std::cout << "  siblings that saw the stop request: "
                  << stopped_observed.load() << " of 4\n";
    }

    return 0;
}
