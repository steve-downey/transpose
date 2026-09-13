# Transpose over real senders — Contextful Evolution Plan

Status: APPROVED 2026-09-11 (keying and erasure-boundary ratified; remaining entries default as drafted at Stage 0)
Audience: a Claude Code agent executing concrete work in `beman.transpose`
(P3200). Fresh agent per stage; this document and `decisions.md` are the only
inherited context. **The Why sections are load-bearing.**

Companion: [decisions.md](decisions.md). The entries this plan introduces are
drafted in [decisions-execution-addendum.md](decisions-execution-addendum.md)
and are to be merged into the log at Stage 0. Same conventions as the grading
plan: slugs name the question, never the answer; stages carry outline numbers
*and* slugs; cross-reference by slug.

The divergence protocol of
[transpose-grading-plan.md §0](transpose-grading-plan.md#divergence-protocol)
applies verbatim. It is not repeated here.

## 1. Context: what is being claimed and why it is not yet true {#context}

P3200's second motivating domain is the deferred result:
`vector<sender<T>> → sender<vector<T>>`. The paper, the blog series
("Transposing Structure and Context", "Transpose at Work"), and
`examples/transpose_example.cpp` all show it — over `beman::transpose::sender<T>`,
a `std::function<T()>` thunk in `include/beman/transpose/sender.hpp`, labeled
non-normative. The claim being made to readers is that `transpose` works over
*senders*. What the repository demonstrates is that it works over a callable
that has been type-erased to look like one. That is precisely the move
(`task`, `any_sender`, `std::function`) the design claims not to need.

A survey of every steve-downey repository on 2026-09-11 found no
implementation over real `std::execution` senders anywhere: `compile-time-scheme`'s
Phase 8 uses `when_all` only at fixed arity (builtins take two arguments)
and documents that the runtime-arity case falls back to `sync_wait` in a
loop, naming "a `when_all` that accepts a range of senders" as the missing
piece; `callcc` and the talk repositories use fixed arity only. So this is
new work, and it is the work the paper's second domain rests on.

**Why the obvious thing does not work.** `sequence.hpp`'s vector `traverse`
is a left fold:

```
acc = applicative.pure(vector<Element>{});
for (v : values) acc = applicative.invoke(push_back, acc, f(v));
```

The accumulator's *type* is loop-invariant for `optional`, `expected`,
`zip_list`, and the demo sender (erasure makes every step `sender<vector<T>>`
again). For a P2300 sender each `when_all(acc, s) | then(push_back)` is a new
expression-template type. A loop cannot hold it. Every prior attempt hit this
wall and either sequenced or erased. The way through is not a cleverer fold;
it is recognizing that the applicative object for a runtime-arity structure
needs a *native n-ary composition* — one sender algorithm, `all_of`, owning a
`vector<S>` — and that the Traversable instance should prefer that native
operation when the object offers it. That is exactly the shape
[derived-op-native-preference](decisions.md#derived-op-native-preference)
already mandates for every other derived member.

**Why this is the paper's argument, not a chore.** The demo sender proves
the *typeclass* story: three unrelated types answer to one verb. It proves
nothing about *cost*, and cost is why anyone reaches for senders. The real
instance is where the library's promise — no wrapper materialized, no
conversion, no erasure — is either kept or not. Show the work.

## 2. Standing decisions the agent must know cold {#standing-decisions}

From the existing log, unchanged, and binding here:

- [applicative-objects](decisions.md#applicative-objects) — the applicative
  object is an NTTP-pinned value; traversal order is normatively
  left-to-right. (For senders "order" means *composition* order and result
  order, never completion order — see
  [all-of-failure-semantics](decisions-execution-addendum.md#all-of-failure-semantics).)
- [traverse-policy-surface](decisions.md#traverse-policy-surface) — the
  applicative object is the policy, passed as a trailing defaulted value
  parameter. The sender object is found the same way; nothing new at the
  call site.
- [derived-op-native-preference](decisions.md#derived-op-native-preference) —
  derived members probe `Impl` for a native version and forward to it. The
  range-composition hook in Stage 3 is an application of this rule to the
  *Traversable* side, not a new mechanism.
- [typeclass-conformance-depth](decisions.md#typeclass-conformance-depth) —
  the sender object must satisfy `applicative_object<OBJ, S>` structurally.
  Conformance is probed with a witness; senders are checked by connecting
  and running under `sync_wait`, never by inspecting the type.
- [grading-footprint](decisions.md#grading-footprint) — senders are not
  graded by this plan. The error channel *looks* like a grade (see
  [sender-error-grade](decisions-execution-addendum.md#sender-error-grade),
  OPEN); acting on that resemblance is out of scope.

New decisions, drafted PROPOSED in the addendum and ratified at Stage 0:
[execution-dependency-shape](decisions-execution-addendum.md#execution-dependency-shape),
[sender-instance-keying](decisions-execution-addendum.md#sender-instance-keying),
[sender-value-type-reading](decisions-execution-addendum.md#sender-value-type-reading),
[runtime-arity-composition](decisions-execution-addendum.md#runtime-arity-composition),
[erasure-boundary](decisions-execution-addendum.md#erasure-boundary),
[all-of-failure-semantics](decisions-execution-addendum.md#all-of-failure-semantics),
[demo-sender-fate](decisions-execution-addendum.md#demo-sender-fate).

## 3. Work plan {#work-plan}

Branch: `execution-stage-0-baseline`, then one branch per stage off the
previous, same cadence as the grading work. Each stage ends with a
`docs/review/` note (what was pinned, what diverged) and a log entry under
each slug it touched.

### Stage 0 — [execution-baseline](#execution-baseline) {#execution-baseline}

**Why.** Every later stage is "does the real thing compile and behave"; the
dependency, its spelling, and the untouched goldens have to be nailed down
before any of that is meaningful. The dependency shape is a one-way door for
Beman conformance and for anyone building the paper's examples.

**Deliverables.**
1. Merge the addendum entries into `decisions.md`; set `Decided by` after
   Steve's ruling on each (default: as drafted).
2. Bring in `beman.execution` per
   [execution-dependency-shape](decisions-execution-addendum.md#execution-dependency-shape):
   optional dependency, CMake option `BEMAN_TRANSPOSE_WITH_EXECUTION`
   (default ON when the package is found, OFF otherwise), pinned to a
   recorded commit/tag. Record the namespace actually exported
   (`beman::execution`; `compile-time-scheme` used the older
   `beman::execution26`) and the standard level required.
3. A compile probe `tests/beman/transpose/execution_probe.test.cpp`:
   `sync_wait(when_all(just(1), just(2)) | then(...))` under every preset.
4. Golden capture: the existing sender demo tests and examples are
   *unchanged* by the whole plan
   ([demo-sender-fate](decisions-execution-addendum.md#demo-sender-fate)).
   Pin them as goldens the way `baseline-capture` pinned the carriers.
5. Prior-art note (`docs/review/prior-art-when-all-range.md`): libunifex's
   `when_all_range`, anything in stdexec/`exec::` or Beman that takes a
   range of senders, and any WG21 paper proposing one. Record the surface
   and the failure semantics each chose. **Record only; do not copy.** The
   Stage 2 design is derived from the decision log, and the note exists so
   divergence from prior art is deliberate.

**Acceptance.** Probe green on gcc and llvm presets with the option ON;
whole existing suite green with the option OFF and ON; addendum merged.

**Tripwires.** Package not found under vcpkg → propose FetchContent/submodule
under the slug, wait. Exported namespace differs from the decision text →
log, proceed with the real one (a What, not a Why). Any change to an
existing golden → STOP.

### Stage 1 — [sender-registration](#sender-registration) {#sender-registration}

**Why.** The typeclass object is looked up by carrier type
(`applicative_typeclass<S>`), and P2300 has no single `M`: every adaptor is
its own type. Registration therefore has to be by *concept*, and the
element type has to be read from completion signatures rather than a
`value_type` member. Both are decided in
[sender-instance-keying](decisions-execution-addendum.md#sender-instance-keying)
and
[sender-value-type-reading](decisions-execution-addendum.md#sender-value-type-reading);
this stage makes them compile.

**Deliverables.**
1. New header `include/beman/transpose/execution.hpp` (compiled only under
   the option; included from `transpose.hpp` under `__has_include` +
   option guard). Contents:
   - `applicative_value<S>` specialization for `S` satisfying
     `ex::sender` with exactly one value completion of exactly one
     argument (`value_types_of_t<S, empty_env, std::type_identity_t,
     std::type_identity_t>`, decayed). Senders with zero, multiple, or
     multi-argument value completions are *not registered* — they fail the
     constraint, so the framework's "no applicative_typeclass<T>"
     diagnostic fires, and it should name this reason.
   - `ExecutionApplicativeImpl`: `pure(x) = ex::just(x)`;
     `invoke(f, s...) = ex::when_all(std::forward<S>(s)...) | ex::then(f)`.
     Operands are forwarded, not taken `const&` (senders may be move-only
     or hold move-only values; the demo's `const sender<T>&` spelling is
     not the model).
   - Constrained partial specialization of `applicative_typeclass` for
     sender types, per keying decision. Check for ambiguity against the
     `void_t<value_type>` primary path in `applicative_value` (tripwire).
2. Tests (`execution.test.cpp`): laziness by counter (nothing runs before
   `sync_wait`); n-ary `invoke` at 2, 3, 5; lifted-callable-through-invoke
   (the demo's fourth test, transliterated); move-only payload; result of
   `invoke` is a plain sender usable in further pipelines (`| then(...)`).
3. Law harness: run the existing Stage-8 applicative law harness over the
   sender object with `sync_wait` as the observation — identity,
   homomorphism, interchange, composition — so the instance is checked the
   same way `optional` and `expected` are, not by a separate ad-hoc suite.
4. `applicative_object<decltype(applicative_typeclass<S>), S>` holds for
   `S = decltype(just(1))` and for a `then`-adapted sender.

**Acceptance.** All above green; goldens unchanged; no `transpose` over a
vector yet (that is Stage 3 — do not "make it work" early).

**Tripwires.** `when_all` rejects an operand (it requires exactly one value
completion per child — the same constraint `callcc` hit) → the registration
constraint was too loose; tighten, log. `applicative_value` ambiguity →
STOP: the keying decision's Why assumed the two paths are disjoint.

### Stage 2 — [all-of-algorithm](#all-of-algorithm) {#all-of-algorithm}

**Why.** This is the piece nobody has written and the one the paper's claim
rests on. `when_all` is variadic; the structure is runtime-sized; the only
non-erased answer is a sender whose operation state owns *n* child operation
states and joins them itself. Design fixed by
[runtime-arity-composition](decisions-execution-addendum.md#runtime-arity-composition)
and
[all-of-failure-semantics](decisions-execution-addendum.md#all-of-failure-semantics).

**Deliverables.** `include/beman/transpose/all_of.hpp`:
1. `all_of(std::vector<S>) -> all_of_sender<S>` (also a range overload that
   materializes to `vector<S>` — the sender owns its children).
   Completion signatures: `set_value_t(std::vector<T>)` where `T` is `S`'s
   single value; every `set_error_t(E)` of `S`, plus
   `set_error_t(std::exception_ptr)` if `S` lacks it (the
   `vector<T>` allocation can throw); `set_stopped_t()`.
2. Operation state: `n` child operation states in **one** allocation, in
   place, never moved (child op-states are immovable, so `vector<OpState>`
   is not an option; use aligned storage + `construct_at` with explicit
   destruction — or `std::deque`'s no-move `emplace_back`, if the agent
   verifies that guarantee on all three standard libraries). Slots
   `vector<optional<T>>` (or an equivalent no-default-construct slot) for
   results, an atomic remaining-count, an `inplace_stop_source` for
   sibling cancellation, a first-error/stopped record.
3. Receiver: forwards the outer environment with the stop token replaced
   by the internal source (the `when_all` pattern); each child's
   `set_value` writes its slot and decrements; the last completion moves
   the slots into `vector<T>` and completes the outer receiver. Errors and
   stops request stop on siblings and win as decided.
4. `n == 0` completes immediately on `start` with an empty vector; `n == 1`
   is not special-cased.
5. Tests (`all_of.test.cpp`): order of *results* equals order of *inputs*
   under a thread pool that completes out of order (log completions,
   assert result order); laziness; empty; error in child *k* → outer error,
   siblings observe stop request; stop request from outside propagates;
   move-only `S`; **exactly one dynamic allocation attributable to
   `all_of`** across `connect`+`start` for `n = 1000` (counting
   `operator new` in the test, as an explicit measurement of the "no
   erasure" claim); no `#include <functional>`, and a `static_assert`
   that `decltype(all_of(v))` is `all_of_sender<S>` — the type is spelled
   from `S`, nothing hidden.
6. Add `all_of` as the native n-ary member on the Stage-1 object:
   `collect(std::vector<S>) = all_of(std::move(v))` — the name is fixed by
   the runtime-arity decision; it is the range twin of `invoke`.

**Acceptance.** All tests green on gcc/llvm; TSAN clean under the pool
test; goldens unchanged.

**Tripwires.** Child op-state size or alignment not available at
`connect` (it always is — `connect_result_t<S, R>`) → STOP, premise wrong.
More than one allocation and the extra is not the `vector<T>` result → STOP
(the design promised one). Needing `std::function` or a virtual anywhere →
STOP; that is the [erasure-boundary](decisions-execution-addendum.md#erasure-boundary).

### Stage 3 — [collect-hook](#collect-hook) {#collect-hook}

**Why.** The vector Traversable is generic over the applicative object and
must stay so; it cannot know about senders. What it *can* do is what every
derived operation in this library already does: prefer a native operation
when the object supplies one. A runtime-arity composition hook (`collect`)
is the Traversable-side instance of
[derived-op-native-preference](decisions.md#derived-op-native-preference).
Every existing instance keeps taking the fold path; the goldens prove it.

**Deliverables.**
1. `VectorTraversableImpl::traverse`: `if constexpr` the applicative object
   offers `collect(std::vector<Effect>)`, map `function` over `values` into
   `vector<Effect>` and call it; else the existing fold, unchanged.
   Spelling follows the disjunctive-requires discipline in
   [derived-op-native-preference](decisions.md#derived-op-native-preference)
   part 2.
2. `transpose(std::vector<S>)` for a P2300 `S` returns
   `all_of_sender<S>`; `traverse(f, vector<int>)` with `f : int -> S`
   returns `all_of_sender<S>`.
3. `traversable_object` / `applicative_object` concepts: decide (and log
   under the hook's slug) whether `collect` is *optional* for
   `applicative_object` — it is: the `optional` object must still conform.
4. Tests: `transpose` over `vector<decltype(just(int))>`; over a vector of
   pool-scheduled senders with out-of-order completion; goldens for
   `optional`/`expected`/`zip_list`/demo-sender unchanged byte-for-byte;
   the fold path still taken for them (assert via a counting applicative
   object without `collect`).

**Acceptance.** Green; goldens unchanged; the example in
`transpose_example.cpp` still uses the demo sender (do not swap it).

**Tripwires.** Any golden changes → STOP. The hook needing to know the
Traversable's element type at object-registration time → STOP (the object
is per-carrier `S`; `collect` is templated on nothing but the vector it is
handed).

### Stage 4 — [transpose-receipts](#transpose-receipts) {#transpose-receipts}

**Why.** The blog series shows work "with receipts". The receipt that
matters here is the one the demo sender could not give: concurrency
actually happening and cost actually being linear and un-erased.

**Deliverables.**
1. `examples/transpose_execution_example.cpp`: `vector<S>` of
  `schedule(pool) | then(work)` senders, transposed, run; prints completion
  order vs result order; a second scene with `run_loop` showing the
  single-threaded, still-lazy case; a third with a child error.
2. Blog post `papers/blog/transpose-over-real-senders.org` (+ `.md` via
   the existing pipeline), slotted after "Transpose at Work" in
   `index.org`: the demo-sender/real-sender contrast; why the fold could
   not work; `all_of` as the native n-ary composition; the one-allocation
   measurement from Stage 2. Written in Steve's voice; draft only, flagged
   for his edit.
3. A one-paragraph erratum-style note added to "Transpose at Work" stating
   that its sender section uses the demonstration type, with a link forward.

**Acceptance.** Example builds and runs under the option; post renders;
Steve has the draft.

### Stage 5 — [paper-note](#paper-note) {#paper-note}

**Why.** P3200 is non-normative about senders and should stay that way
(`all_of` is not being proposed; a range `when_all` belongs to the
execution papers). But the design rationale should say what was shown.

**Deliverables.** In `papers/D3200R0.md` (or its successor), non-normative:
the second domain's example now references the real instance; a short
design note that the Traversable/Applicative split is what let the runtime-
arity composition live in the applicative object without the library
knowing about senders; a pointer to
[sender-error-grade](decisions-execution-addendum.md#sender-error-grade) as
future work. No wording changes.

**Acceptance.** Paper builds; wording pipeline output unchanged.

## 4. Follow-ons explicitly not in this plan {#follow-ons}

- `compile-time-scheme` Phase 8's closure-argument loop becomes `all_of`'s
  second client. That lands in that repository after Stage 3 is on main
  here.
- A Monad instance for P2300 senders (`bind = let_value`). The
  2026-09-07 worklist ruled real senders out of the monad-basis traits
  because "same context, nested" is a normalization problem; that ruling
  stands. See [sender-monad-instance](decisions-execution-addendum.md#sender-monad-instance).
- `std::array<S, N>`: static arity, so `when_all(arr[Is]...)` works with no
  `all_of` at all — a nice contrast for the paper, but the array
  Traversable currently has no such path. See
  [static-arity-array](decisions-execution-addendum.md#static-arity-array).
- Grading the sender error channel. See
  [sender-error-grade](decisions-execution-addendum.md#sender-error-grade).
