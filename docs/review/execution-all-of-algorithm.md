# Stage review — all-of-algorithm

Stage [all-of-algorithm](../transpose-execution-plan.md#all-of-algorithm),
the third stage of [transpose-execution-plan.md](../transpose-execution-plan.md).
Run 2026-09-14.

**Status: COMPLETE.** All seven deliverables done. The contested claim is
settled, by measurement, in the plan's favour — and the refutation is written
where the contested claim lives, per the standing instruction.

---

## 1. The result

`examples/all_of.hpp` is `when_all` at runtime arity: a sender owning a
`std::vector<S>`, completing with a `std::vector<T>`, with an operation state
that owns *n* child operation states directly.

Measured at **n = 1000**, in `all_of_allocation.test.cpp`:

| | |
|---|---|
| Allocations during `connect` | **1** |
| Its size | **32,000 bytes = n × `sizeof(holder)`**, `sizeof(holder) == 32` |
| Allocations during `start` | **1** — the result `vector<T>` |
| **Total** | **2** — and the second is exactly the one the boundary permits |

[erasure-boundary](../decisions.md#erasure-boundary) defines the claim "no
wrapper materialized" as precisely this: one allocation whose size is `n` times
a compile-time constant, known at `connect`, plus the result vector. That is now
an assertion that passes rather than an intention.

Also asserted, because the boundary names them: `decltype(all_of(v))` is
`all_of_sender<S>` — the type is spelled from `S`, so a caller keeps composing
on the real thing; and neither the sender, the operation state nor the holder is
polymorphic. The header stores no callable and includes no `<functional>`.

**Therefore** [p2300-front-door-shape](../decisions.md#p2300-front-door-shape)'s
Consequences paragraph — *"Transposing a runtime-sized structure of senders
needs a type-erased sender, which `bemanproject/execution` does not ship"* — is
**refuted**. It needs a native n-ary composition. The refutation is logged under
that entry, dated, as Steve's ruling of 2026-09-13 required either way the
measurement came out.

**What the refutation does not cover**, stated so it is not read wider than it
is: `transpose(std::vector<S>)` over a real sender *still does not work*. The
vector Traversable still folds; teaching it to prefer `collect` is Stage 3.
The second half of that entry's finding — that the front door cannot take real
senders — remains true today, but now for a reason with a fix rather than for
the reason the entry gave.

---

## 2. Design, and the two places the plan was wrong

**The operation state.** One block, `n` holders, each
`{optional<T> value; connect_result_t<S, element_receiver> operation;}`.
Children are constructed in place with `construct_at` and destroyed in place;
none is ever moved, which is what an immovable operation state requires.
`element_receiver` is a pointer and an index — two words, no callable.

**Correction 1 — the drafted layout could not meet the drafted tripwire.** The
plan asked for child operation states in one allocation *and* result slots in a
separate `vector<optional<T>>`. That is two blocks before the result vector,
while the same stage's tripwire stops at more than one. Colocating each slot
with its child's operation state is what gets to one. Found at Stage 0 from the
prior-art survey, corrected in the plan on 2026-09-13, and confirmed here by the
number.

**Correction 2 — `set_stopped` is conditional.** The plan listed
`set_stopped_t()` unconditionally. P3887R1 (LWG-approved 2025-11) says
`when_all` advertises it only if a child does, and
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) defines
`all_of` *by reference to* `when_all`. Pinned both ways: a vector of `just`
senders does not advertise a stopped completion; a vector of stoppable senders
does.

**Failure semantics follow `when_all`, and that choice shows.** `set_error` uses
`exchange`, so an error claims the disposition even from a recorded `stopped`;
`set_stopped` uses a compare-exchange and claims it only from `started`. That
asymmetry is why a stopped child cannot mask a later error — and it is exactly
where libunifex's `when_all_range` differs, sharing one flag between the two.
The prior-art note recorded the difference at Stage 0; the decision's Why is
what chose against it.

---

## 3. What was hard, and what a later stage should expect

**The allocation measurement cannot share a translation unit with
ThreadSanitizer.** Counting allocations means replacing global `operator new`
and `operator delete`; the TSan runtime defines those symbols itself, and the
TU does not link. Discovered by trying it.

The split is the fix and it is not cosmetic: `all_of.test.cpp` holds the
behaviour, including the concurrent case, and builds under TSan;
`all_of_allocation.test.cpp` holds the measurement and carries a
`__has_feature(thread_sanitizer)` guard so that a TSan build of it degrades to
a skipped case rather than a link error. A later stage adding allocation
measurement should expect to pay the same cost.

**`get_parallel_scheduler()` is not linkable from these tests.** Its backend
symbol (`parallel_scheduler_replacement::query_parallel_scheduler_backend`) is
not in what `beman::execution` exports to a consumer here, so the pool the plan
suggested for the ordering test is unavailable. The test uses one `std::jthread`
per child instead, with delays staggered so the *last* child finishes *first*.
That is stronger than a pool for this purpose: the out-of-order completion is
deliberate rather than hoped for, and the test asserts it happened
(`rank[n-1] < rank[0]`) so that a future change serializing the children would
show up as the test no longer proving anything.

**Two C++ shapes worth not rediscovering.** A member alias that mentions a data
member inside a lambda does not see members declared later — an alias
declaration is not a complete-class context, so the stop-callback functor is a
named struct rather than a lambda. And a sender's nested operation state cannot
hold a copy of the sender: inside the sender's own definition the sender type is
still incomplete, so the test's threaded child holds its configuration field by
field.

**The error variant must be deduplicated.** It is
`variant<monostate, exception_ptr, child errors…>`, and a child that itself
raises `exception_ptr` would give the variant two identical alternatives —
`emplace<T>` on such a variant is *deleted*, not merely ambiguous. That is a
compile error a long way from its cause; the header deduplicates deliberately.

---

## 4. Acceptance

| Deliverable | Result |
|---|---|
| 1 — `all_of(vector<S>)`, range overload, completion signatures | Done, with the P3887R1 correction |
| 2 — n op-states in one allocation, in place, never moved | Done; measured |
| 3 — receiver forwarding env with the stop token replaced | Done |
| 4 — `n == 0` immediate, `n == 1` not special-cased | Done |
| 5 — tests: order, laziness, empty, error+sibling stop, external stop, move-only, allocation, type | Done, 9 cases |
| 6 — `collect` on the Stage-1 object | Done; `collect(vector<S>) = all_of(...)` |
| 7 — write the result back under `p2300-front-door-shape` | **Done** — the refutation, dated |

| Preset | Option OFF | Option ON |
|---|---|---|
| gcc-debug | 240 | 260 |
| llvm-debug | 240 | 260 |

Before this stage: 240 / 251. **The option-OFF count is unchanged**, which is
the "nothing execution-dependent enters `include/`" constraint as a number.
TSan: `all_of.test.cpp` builds and runs clean under a TSan configuration, 55
assertions, no races. Goldens untouched; `git diff main..HEAD -- include` is
still empty.

**Tripwires.** None fired. Child operation-state size and alignment *were*
available at `connect`, as the plan said they always are. No allocation beyond
the one block and the result vector. No `std::function`, no virtual, nothing
execution-dependent under `include/`.

---

## 5. What Stage 3's agent needs

**Everything Stage 1's review said about the policy concept still stands, and
it is now the only thing between here and a working front door.**
`applicative_object_for<POLICY, CONTEXT>` demands `pure(element)` return
exactly `CONTEXT`; `pure(x)` is always `just(x)`; so the refinement holds for
`decltype(just(1))` and fails for every adapted sender — including, now,
`all_of_sender<S>` itself. `collect` has to be reachable *before* that
refinement is applied, or the refinement has to relax for objects that supply
it.

**`all_of_sender<S>` is itself a registered Applicative element**, because it
completes with exactly one value. So `transpose` of a transposed structure
composes, once the hook exists — worth a test in Stage 3, since it is the
property that makes `collect` a real applicative operation rather than a
terminal one.

**The `collect` signature the hook must probe** is
`collect(std::vector<Effect>)`, a member of the applicative *object*, taking
the vector by value. It is `protected`-inherited through the CRTP base like the
rest of the basis, so a `Map` must re-export it — `P2300ApplicativeMap` does,
beside `invoke` and `pure`.

**Do not let `sequence.hpp` learn about senders.** The hook probes for a member
and forwards; that is all. Both
[runtime-arity-composition](../decisions.md#runtime-arity-composition)'s
Sentinel and the 2026-09-13 ruling say so, from opposite directions.
