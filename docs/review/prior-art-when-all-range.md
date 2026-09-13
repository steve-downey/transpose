# Prior art: joining a runtime-sized range of senders

Stage [execution-baseline](../transpose-execution-plan.md#execution-baseline),
deliverable 5. Surveyed 2026-09-13.

**Read this as a record, not a menu.** The Stage 2 design is derived from
[runtime-arity-composition](../decisions.md#runtime-arity-composition) and
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics); this
note exists so that where `all_of` differs from what everyone else built, the
difference is a choice someone made rather than a wheel nobody looked at.
Section 5 is the part with teeth: two places where prior art contradicts the
plan's own text.

---

## 1. The headline: exactly one prior implementation, and it is not in a standard

| Project | Has a range-of-senders join? |
|---|---|
| **libunifex** | **Yes** — `unifex::when_all_range`, undocumented |
| stdexec | **No** — checked at `30bb116`; zero hits repo-wide |
| bemanproject/execution (our dependency) | **No** — checked at `c55d824`; `when_all` is variadic only |
| WG21 papers | **No** — no paper proposes one |

That is the finding that matters most for the plan: **the dependency does not
ship this, the reference implementation everyone reads does not ship this, and
no paper proposes it.** Stage 2 is writing something that exists once, in a
Meta library, with no test coverage in the documentation and no specification
anywhere. [transpose-execution-plan.md §3](../transpose-execution-plan.md#all-of-algorithm)'s
"this is the piece nobody has written" is very nearly true — it is the piece
one person wrote and nobody specified.

stdexec carries an open issue asking for precisely this facility —
[NVIDIA/stdexec#1631](https://github.com/NVIDIA/stdexec/issues/1631), *"What's
the recommended way to wait a list of senders?"*, opened 2025-09-03, noting
that `when_all` cannot serve because its children must be known at compile
time. *Recorded uncertainty:* whether maintainers answered could not be
determined from here; only the issue body was retrievable.

---

## 2. libunifex `when_all_range` — the one real precedent

[`include/unifex/when_all_range.hpp`](https://github.com/facebookexperimental/libunifex/blob/main/include/unifex/when_all_range.hpp),
379 lines, last touched 2026-01-27. Absent from `doc/api_reference.md`, which
documents only the variadic `when_all`.

### Surface

```cpp
template(typename Sender)(requires(unifex::sender<Sender>))
auto operator()(std::vector<Sender> senders) const;

template <typename Iterator>
auto operator()(Iterator first, Iterator last) const
    -> _when_all_range::sender<sender_from_iterator_t<Iterator>>;
```

Homogeneous — one `Sender` type for all children. The iterator overload
materializes to `std::vector<Sender>` internally; no `std::ranges` support, no
sentinels, no views. Each child must have exactly one value completion of
exactly one value. Completes with a single `std::vector<T>`:

```cpp
template <template <typename...> class Variant, template <typename...> class Tuple>
using value_types = Variant<Tuple<std::vector<sender_value_type>>>;
template <template <typename...> class Variant>
using error_types = Variant<std::exception_ptr>;
static constexpr bool sends_done = true;
```

**Result order is input order.** Each element receiver carries an `index_` and
writes `op_.holders_[index_].value.emplace(...)`; the result is assembled by a
`std::transform` over the holders in order.

### Allocation

One array allocation for all children, with each child's value slot and
operation state **colocated in the same element**:

```cpp
struct _operation_holder final {
  std::optional<sender_nonvoid_value_type> value;
  unifex::connect_result_t<Sender, element_receiver_t> connection;
};
std::allocator<_operation_holder> allocator;
holders_ = allocator.allocate(senders.size());
```

No per-child heap allocation. Total heap traffic: the input `vector<Sender>`,
one holder array, one result vector. `std::allocator` is hardcoded — no
allocator parameter. The operation state is immovable and `connect`
unconditionally moves the senders out, so the sender is single-shot.

**This converges with [erasure-boundary](../decisions.md#erasure-boundary)
without being its source.** The boundary decision permits exactly one
allocation sized `n × sizeof(connect_result_t<S, R>)` plus the result vector;
libunifex independently landed on the same budget. That is a useful
corroboration that Stage 2's one-allocation tripwire is achievable rather than
aspirational — the tripwire asks for something that has been done.

Note the one refinement worth considering on its own merits, not because
libunifex did it: colocating the result slot *with* the child operation state
in one element, rather than keeping `vector<optional<T>>` as a separate
allocation, is what gets libunifex to one array instead of two.
[transpose-execution-plan.md §3](../transpose-execution-plan.md#all-of-algorithm)
deliverable 2 describes the slots as a separate `vector<optional<T>>`, which
is a *second* allocation and would trip that stage's own "more than one
allocation" tripwire unless the slots live inside the single block.
**Flagged for Stage 2**: the plan's deliverable 2 and its tripwire are in
tension as written, and the colocated layout is one way to resolve it.

### Failure semantics — and where they differ from ours

```cpp
template <typename Error>
void set_error(Error&& error) noexcept {
  if (!op_.doneOrError_.exchange(true, std::memory_order_relaxed)) {
    op_.error_.emplace(...);
    op_.stopSource_.request_stop();
  }
  op_.element_complete();
}
void set_done() noexcept {
  if (!op_.doneOrError_.exchange(true, std::memory_order_relaxed)) {
    op_.stopSource_.request_stop();
  }
  op_.element_complete();
}
```

- First abnormal completion **in time** wins and requests stop on siblings.
- **`set_done` and `set_error` share one flag.** A stopped child arriving
  first therefore *masks a later error*, and the whole completes `set_done`.
- Children never see the parent's stop token: `get_stop_token` on the element
  receiver returns the internal source's token only.
- External cancellation installs a stop callback that calls `request_stop()`
  but **does not set `doneOrError_`**. If every child ignores the request and
  succeeds, `when_all_range` still completes with the full vector despite a
  stop having been requested — no late-stop detection. (P3887R1 records that
  libunifex's *variadic* `when_all` does have it.)
- Empty input completes immediately with an empty vector.

**A likely bug, recorded so nobody transliterates it.** `error_types`
advertises only `std::exception_ptr`, but the operation state stores
`std::optional<sender_error_types_t<Sender, std::variant>>` and visits it into
`set_error(receiver, <original error type>)`. A child erroring with, say,
`std::error_code` calls `set_error` with a type the sender never advertised.
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) requires
`all_of` to advertise every `set_error_t(E)` of `S`, which does not have this
hole; the divergence is already correct and now has a reason on the record.

---

## 3. The standard's variadic `when_all`, which is what we chose to match

[P2300R10 §34.9.11.11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html)
`[exec.when.all]`. Pack-only; ill-formed for zero senders; each child
constrained to at most one value completion.

```cpp
enum class disposition { started, error, stopped };
atomic<size_t> count{sizeof...(sndrs)};
inplace_stop_source stop_src{};
atomic<disposition> disp{disposition::started};
```

```cpp
if constexpr (same_as<Set, set_error_t>) {
  if (disposition::error != state.disp.exchange(disposition::error)) {
    state.stop_src.request_stop();
    TRY-EMPLACE-ERROR(state.errors, std::forward<Args>(args)...);
  }
} else if constexpr (same_as<Set, set_stopped_t>) {
  auto expected = disposition::started;
  if (state.disp.compare_exchange_strong(expected, disposition::stopped)) {
    state.stop_src.request_stop();
  }
} else ...
```

The asymmetry is the point: error uses `exchange` and therefore **out-ranks a
previously recorded `stopped`**; stopped uses a compare-exchange and only wins
from `started`. `start` checks `stop_src.stop_requested()` before starting any
child and completes `set_stopped` if so. Values are stored positionally and
`tuple_cat`'d.

[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) says
"error wins over stopped; among errors the first to arrive wins", which is
this, not libunifex's shared flag. **The decision's Why — that `all_of` should
be `when_all` at runtime arity rather than a different algorithm — picks the
standard over the only existing implementation, and that is the right way
round.** Recorded here because it is the one place the plan could have been
accused of inventing, and it is not: libunifex is the outlier.

---

## 4. Live WG21 work that touches the semantics we are matching

All Robert Leahy. These matter because `all_of` is defined by reference to
`when_all`, so `when_all` moving moves us.

| Paper | Title | Status | Bearing |
|---|---|---|---|
| [P3887R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3887r1.pdf) | Make `when_all` a Ronseal Algorithm | **LWG-approved 2025-11** | `when_all` advertises `set_stopped` **only if a child does** |
| [P4217R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4217r1.pdf) | `when_all()` is just `just()` | LEWG | Zero senders allowed; motivated explicitly by generic algorithms |
| [P4269R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4269r0.pdf) | `when_all` Oughtn't Hallucinate `set_stopped` | SG1/LEWG | Don't create the stop source when it can be shown unused |
| [P3409R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3409r1.html) | More efficient stop-token cancellation | — | `when_all` to use `finite_inplace_stop_source` |

P4217R1's motivation is worth quoting in the paper if Stage 5 wants it: the
committee's stated reason for admitting the empty case is that forbidding it
*"unnecessarily creates a special case when writing generic algorithms"* —
which is exactly why
[transpose-execution-plan.md §3](../transpose-execution-plan.md#all-of-algorithm)
deliverable 4 makes `n == 0` complete with an empty vector rather than being
ill-formed. Independent agreement, and citable.

**Nothing here proposes a range `when_all`.** The full wg21.link index was
scanned; no paper matches. [transpose-execution-plan.md §5](../transpose-execution-plan.md#paper-note)
says `all_of` is not being proposed because "a range `when_all` belongs to the
execution papers" — accurate as a scoping decision, but the execution papers
do not currently contain one, and on this evidence nobody is writing it. If
P3200 wants to point at future work, there is no paper to point at.

P2300 itself uses "a range of senders" for the *opposite* concept — §4.13 "A
range of senders represents an async sequence of data" consumes them one at a
time in a coroutine loop, explicitly not concurrently. The standard's answer
for dynamic arity is [P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html)
`counting_scope`/`async_scope` (LWG-approved 2025-06), whose `join()`
**completes with void and collects no results**; `spawn_future` yields one
sender per item, which you would still need a variadic `when_all` to join.
So the gap `all_of` fills is real and the standard currently routes around it.

---

## 5. Where prior art contradicts the plan — read before Stage 2

Two items. Neither is settled here; both are flagged so Stage 2 does not
discover them at the tripwire.

**(a) The plan's completion signatures may over-advertise `set_stopped`.**
[transpose-execution-plan.md §3](../transpose-execution-plan.md#all-of-algorithm)
deliverable 1 says the signatures are `set_value_t(vector<T>)`, every
`set_error_t(E)` of `S` plus `set_error_t(exception_ptr)`, and
`set_stopped_t()` — unconditionally. **P3887R1 is LWG-approved and says
`when_all` should send `set_stopped` only if a child does**, and P4269R0 goes
further at the implementation level. Since
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) grounds
`all_of` in "follow `when_all`", following the `when_all` that was approved
means making that signature conditional. The plan's text follows the older
`when_all`. This is a What, and the Why ("`all_of` *is* `when_all` at runtime
arity") actively argues for the change rather than against it.

**(b) The plan's operation-state layout implies two allocations, and its own
tripwire forbids the second.** Deliverable 2 asks for `n` child operation
states in one allocation *and* slots as `vector<optional<T>>`; the tripwire
says "more than one allocation and the extra is not the `vector<T>` result →
STOP". The result vector is the one permitted extra, so a separate slots
vector is a third block. libunifex resolves this by colocating the slot with
the child operation state in a single holder array. Flagged above in §2.

---

## 6. Design-shape comparison (non-sender, recorded for contrast only)

| | result order | first error | stopped/cancel | empty | allocation |
|---|---|---|---|---|---|
| unifex `when_all_range` | input | first in time, stops siblings | shares flag with error; stopped masks later error | `set_value({})` | one holder array (slot + op-state colocated) |
| P2300 `when_all` (pack) | input (tuple) | first in time, `exchange` | error out-ranks stopped; eager stop check before start | ill-formed (P4217 would change) | in-place in op state |
| Folly `collectAllRange` | input | first in time, cancels siblings, partials discarded | merged ambient token | empty vector | per-child coroutine frame |
| Folly `collectAllTryRange` | input | **no** sibling cancellation; `Try<T>` per slot | — | empty vector | per-child coroutine frame |
| cppcoro `when_all(vector)` | input | first **by index**, not by time; no cancellation | none | empty vector | per-child coroutine frame |
| Asio `ranged_parallel_group` | input **+ explicit `completion_order` vector** | policy parameter; errors are data | `cancellation_type` from the condition | **throws `logic_error`** | one `allocate_shared`, allocator-aware |
| Rust `join_all` | input | n/a (no error channel) | drop | empty `Vec` | one boxed slice ≤30 elems, else `FuturesOrdered` |
| Rust `try_join_all` | input | short-circuits, drops rest | drop | `Ok(vec![])` | as above |
| Rust `FuturesUnordered` | **completion** | caller's | drop | empty stream | per-future, intrusive |

Two capabilities only Asio offers, noted as gaps rather than proposals:

- **Both orders at once.** `ranged_parallel_group` returns per-slot results in
  input order *and* a `vector<size_t> completion_order` permutation.
  [all-of-failure-semantics](../decisions.md#all-of-failure-semantics) says
  result order is input order and completion order is "the scheduler's
  business" — true, and Asio shows that surfacing both is nevertheless
  possible. Relevant to
  [transpose-execution-plan.md §3](../transpose-execution-plan.md#transpose-receipts)
  Stage 4, whose example wants to *print* completion order against result
  order and will otherwise have to instrument the children to get it.
- **Failure policy as a parameter** (`wait_for_all`, `wait_for_one`,
  `wait_for_one_error`, `wait_for_one_success`) rather than baked in. Out of
  scope for `all_of`, and worth knowing exists before anyone calls
  first-error-wins the only option.

Folly's `collectAllTryRange` is the other shape worth a sentence: input order,
`Try<T>` per slot, no sibling cancellation — "give me everything, I will sort
it out". That is what an *accumulating* applicative object over senders would
want, and it is the sender-side echo of
[applicative-objects](../decisions.md#applicative-objects)'s two-objects
split. Not in scope; recorded because the plan has no slug for it and someone
will ask.

---

## Sources

libunifex
[`when_all_range.hpp`](https://github.com/facebookexperimental/libunifex/blob/main/include/unifex/when_all_range.hpp),
[test](https://github.com/facebookexperimental/libunifex/blob/main/test/when_all_range_test.cpp) ·
[NVIDIA/stdexec](https://github.com/NVIDIA/stdexec) (`30bb116`),
[issue #1631](https://github.com/NVIDIA/stdexec/issues/1631) ·
[bemanproject/execution](https://github.com/bemanproject/execution) (`c55d824`) ·
[P2300R10](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html) ·
[P3887R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3887r1.pdf) ·
[P4217R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4217r1.pdf) ·
[P4269R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4269r0.pdf) ·
[P4320R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4320r0.pdf) ·
[P3409R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3409r1.html) ·
[P3149R11](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3149r11.html) ·
[wg21.link index](https://wg21.link/index.json) ·
[Folly `Collect.h`](https://github.com/facebook/folly/blob/main/folly/coro/Collect.h) ·
[cppcoro `when_all.hpp`](https://github.com/lewissbaker/cppcoro/blob/master/include/cppcoro/when_all.hpp) ·
[Asio `parallel_group.hpp`](https://github.com/boostorg/asio/blob/develop/include/boost/asio/experimental/parallel_group.hpp) ·
[futures-rs `join_all.rs`](https://github.com/rust-lang/futures-rs/blob/master/futures-util/src/future/join_all.rs)
