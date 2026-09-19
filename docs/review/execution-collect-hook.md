# Stage review — collect-hook

<!-- SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception -->

Stage 3 of [transpose-execution-plan.md](../transpose-execution-plan.md),
by slug [collect-hook](../transpose-execution-plan.md#collect-hook).
Executed 2026-09-19 on `claude/transpose-patch-plan-5cwbdy`, restarted from
`main` at `9a61123` after Stages 0–2 merged as PR #54.

New decision entry: [collect-hook](../decisions.md#collect-hook), part 1
DECIDED, **part 2 OPEN and waiting on Steve**.

---

## 1. The result

`transpose(std::vector<S>)` over a genuine P2300 sender works, and returns
`all_of_sender<S>`.

That is the claim P3200's second motivating domain makes, made good over real
senders rather than over a `std::function` thunk, and it is made good by
preference rather than by erasure: the vector Traversable asks the applicative
object whether it has a native range composition, and the object that does is
the one under `examples/`.

The whole `include/` change is one file. `sequence.hpp` gained a probe:

```cpp
template <class APPLICATIVE, class EFFECT>
concept collecting_applicative = requires(const APPLICATIVE &applicative) {
    applicative.collect(std::declval<std::vector<EFFECT>>());
};
```

and an `if constexpr` on it in each of `traverse`'s two overloads. It names no
sender, no execution header and no `all_of`, and there is no `if constexpr` on
"is a sender" anywhere — the Sentinel
[runtime-arity-composition](../decisions.md#runtime-arity-composition) carries,
surviving the one implementation most likely to break it.

**The 2026-09-13 ruling holds as a number.** `git diff` against the base
touches exactly one file under `include/`, and the option-OFF suite grew only
by the five generic cases this stage added — the sender half of the work is
under the option, where it was told to be.

---

## 2. What the plan got right, and the one thing it got half wrong

**Right, and worth saying because it was the whole bet:** the hook is the
Traversable-side instance of
[derived-op-native-preference](../decisions.md#derived-op-native-preference)
and nothing more. No new mechanism, no sender vocabulary, no second contract.
An object with `collect` is preferred; an object without it is derived against
exactly as before. Both halves are asserted with a pair of objects that differ
in nothing but `collect` (`collect_hook.test.cpp`), so "the hook fired" and
"the hook did not fire" are each a count rather than an inference.

**Half wrong: the obstacle was smaller than two stages of review believed.**
Stage 1 found that `applicative_object_for` demands `pure(element)` return
exactly `CONTEXT`, which the sender object cannot promise; Stage 2's review
called this "now the only thing between here and a working front door". The
measurement says the second half of that sentence was wrong.

`transpose` does not carry `applicative_object_for`. It carries
`transposable_structure`, which asks `applicative_context`, which is
`applicative_object` — the concept that does **not** constrain `pure`'s
return type. So the front door was never blocked, and `transpose(vector<S>)`
began working the moment the hook existed, for plain and adapted senders
alike.

What *is* blocked is the free `traverse(f, value)`, whose trailing policy
parameter does carry the refinement:

| entry point | constraint | `just(1)` | `just(1) \| then(f)` |
|---|---|---|---|
| `transpose(vector<S>)` | `applicative_object` | yes | **yes** |
| `traverse(f, vector<T>)` | `applicative_object_for` | yes | **no** |

Both rows are pinned as `static_assert`s in `transpose_senders.test.cpp`. The
stage did not act on this: the proposal and its three options are written up
at [collect-hook](../decisions.md#collect-hook) part 2, and the entry says
which one this stage would pick and why. **It is Steve's to rule, and nothing
in this stage assumes an answer.**

---

## 3. What a later stage should expect

**Both `traverse` overloads need the preference, separately.** They are two
out-of-line definitions with two bodies; teaching one and not the other is a
silent half-fix, because which one runs depends only on the value category of
the caller's vector. There is a case for each.

**The empty structure is where taking the wrong path is invisible.** A fold
over zero elements returns `pure({})` and composes nothing, so it produces the
right answer down the wrong branch. `collect_hook.test.cpp` pins it with a
count rather than with the result.

**`all_of_sender<S>` is itself a registered element**, because it completes
with exactly one value. `transpose` of a vector of transposed structures
therefore composes, and returns `all_of_sender<all_of_sender<S>>` sending a
`vector<vector<int>>`. This is the property that makes `collect` an
applicative operation rather than a terminal one, and it is tested.

**Every lambda-expression is a distinct type, and sender types remember
that.** A sender type spelled once at namespace scope from a lambda cannot be
rebuilt inside a test case: `decltype(just(0) | then(<lambda>))` names one
closure and the test's own `then(<lambda>)` names another, so `push_back`
stops compiling a long way from the cause. The test uses a named callable,
the same fix `detail::vector_append_t` exists for in `sequence.hpp`.

**The wording fragment is now drifted and was not regenerated.** Both
`traverse` overloads' `//!` prose states the `collect` path — the complexity
clause in particular, since the native path is one composition operation
rather than `values.size()` of them — so
`papers/wording/transpose.range.traverse.md` is older than the header. `make
wording` does not currently succeed (steve-downey/specgen#109) and `specgen`
is not installed here; hand-editing a generated file would make it agree with
the header by a route the pipeline cannot reproduce. Recorded under
[collect-hook](../decisions.md#collect-hook) rather than fixed. **This is the
first of the drifted fragments whose drift this project's own work caused**,
and it is a direct, concrete instance of the thing Steve named as the real
blocker: a published specgen.

---

## 4. Acceptance

| Deliverable | Result |
|---|---|
| 1 — `traverse` prefers `collect`, else the fold, unchanged | Done, both overloads |
| 2 — `transpose(vector<S>)` → `all_of_sender<S>` | Done, plain and adapted |
| 2 — `traverse(f, vector<int>)` → `all_of_sender<S>` | **Partial** — holds for `S = decltype(just(1))`, constrained out otherwise; raised as [collect-hook](../decisions.md#collect-hook) part 2 |
| 3 — decide and log whether `collect` is optional | Done: it is. [collect-hook](../decisions.md#collect-hook) part 1 |
| 4 — tests, goldens unchanged, fold path still taken | Done, 13 cases |

| Preset | Option OFF | Option ON |
|---|---|---|
| gcc-debug | 251 | 279 |
| llvm-debug | 251 | 279 |

Before this stage: 246 / 266. All green. `pre-commit run -a` clean.

**Acceptance clause.** Green: yes. Goldens unchanged: yes — byte-for-byte,
and `demo_sender_golden.test.cpp` and `baseline_deduction.test.cpp` are
untouched files, not merely passing ones. `transpose_example.cpp` still uses
the demonstration sender: untouched.

**Tripwires.** None fired. No golden changed. The hook needs no knowledge of
the Traversable's element type at registration time — it is templated on the
vector it is handed and on nothing else. `sequence.hpp` acquired no execution
include, no sender name and no mention of `all_of`. Nothing
execution-dependent landed under `include/`.

**One acceptance signal that was not a deliverable.** Stage 1's review asked
Stage 3 to treat "this message becomes clean, or becomes a success" as a
signal, having recorded that registration made `transpose(vector<S>)` die
inside `sequence.hpp` with *"no viable overloaded `=`"* and a three-frame
backtrace. It became a success.

---

## 5. What Stage 4's agent needs

**Read [collect-hook](../decisions.md#collect-hook) part 2 before writing
anything about the front door.** If Steve has ruled, the ruling is there. If
he has not, `transpose` over senders is the story and the free `traverse` over
senders is not — do not write prose that implies otherwise, and do not relax
`applicative_object_for` to make an example read better. That concept is in
`include/`, and widening it on a blog post's initiative is exactly the move
the whole plan is arranged to prevent.

**Stage 4 deliverable 1 is now mostly written for it.**
`transpose_senders.test.cpp` already contains the out-of-order-completion
scene, the laziness scene and the child-error scene as tests. The example's
job is to *show* them — printing completion order against result order — not
to establish them.

**`get_parallel_scheduler()` is still not linkable here**, as Stage 2 found;
the concurrent tests use one `std::jthread` per child with delays staggered so
the last child finishes first, and assert that it did. An example that wants a
pool will hit the same wall.

**The measurement to quote is Stage 2's**, unchanged by this stage: one
allocation at `connect` of `n × sizeof(holder)` bytes, one at `start` for the
result vector, two in total. This stage added a preference, not a cost.
