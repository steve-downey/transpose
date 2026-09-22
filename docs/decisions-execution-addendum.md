# Decision Log addendum — transpose over real senders

> **SUPERSEDED 2026-09-13. This is a historical draft. Do not read decisions
> out of it, and do not edit it.**
>
> These entries were merged into [decisions.md](decisions.md) at Stage 0
> [execution-baseline](transpose-execution-plan.md#execution-baseline) —
> **reconciled, not verbatim.** The log is the source; where it and this file
> differ, the log is right and the difference is deliberate.
>
> What changed in the merge, so nobody has to diff it:
> - `execution-dependency-shape` — **WITHDRAWN**, superseded by
>   [p2300-front-door-shape](decisions.md#p2300-front-door-shape). The option
>   is `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, OFF by default, pinned
>   `d24898d`, and **nothing execution-dependent enters `include/`**.
> - `demo-sender-fate` — **WITHDRAWN**, already ruled under the same entry.
> - `sender-value-type-reading` — kept **PROPOSED**, not graduated; its
>   `empty_env` is spelled `env<>` at the pinned commit.
> - `runtime-arity-composition`, `erasure-boundary`,
>   `all-of-failure-semantics` — kept. `all_of` lands as
>   `examples/all_of.hpp`, not under `include/`.
> - This file's "PROPOSED entries default to as drafted at Stage 0" rule was
>   **not** applied wholesale; Steve ruled on 2026-09-13 instead.
>
> The reason this file is kept rather than deleted: the execution plan was
> drafted from it without
> [p2300-front-door-shape](decisions.md#p2300-front-door-shape) in view, and
> that is the mistake the reconciliation exists to correct. Keeping the draft
> keeps the correction legible.

Original header follows.

To be merged into `decisions.md` at Stage 0 of
[transpose-execution-plan.md](transpose-execution-plan.md). Same conventions:
one question per section, slug names the question, Question / Status /
Decision / Why / Log, `Decided by` on ratification. Entries marked PROPOSED default to "as drafted" at Stage 0 unless Steve
rules otherwise; `sender-instance-keying` and `erasure-boundary` were
ratified by Steve on 2026-09-11.

---

## execution-dependency-shape

**Question:** How does `beman.execution` enter the build, and is it required?
**Status:** PROPOSED 2026-09-11
**Decision:** Optional dependency behind `BEMAN_TRANSPOSE_WITH_EXECUTION`,
found via the package manager the repo already uses (vcpkg manifest; if no
port exists, a pinned FetchContent of a recorded tag, logged here). The
execution-dependent headers (`execution.hpp`, `all_of.hpp`) are included
from `transpose.hpp` only under the option. The demo `sender.hpp` remains
unconditional.
**Why:** P3200 proposes Traversable/Applicative machinery, not senders; a
hard dependency on an execution implementation would make the paper's
reference implementation unbuildable for readers who only want the
`optional`/`expected` story, and would tie the library's Beman conformance to
another library's release cadence. Optional keeps the front door light and
still lets CI prove the real instance on every commit.
**Log:**
- 2026-09-11 — Drafted.

## sender-instance-keying

**Question:** How is the Applicative object found for a P2300 sender, given
there is no single context template `M` — each adaptor is its own type?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve Downey, 2026-09-11 (planning discussion).
**Decision:** A constrained partial specialization of the
`applicative_typeclass` variable template:
`template <class S> requires single_value_sender<S> inline constexpr auto
applicative_typeclass<S> = ExecutionApplicativeMap{};` where
`single_value_sender` is `ex::sender<S>` plus "exactly one value completion,
exactly one argument" under `empty_env`. The object itself is not
templated on `S`; `pure` and `invoke` are member templates.
**Why:** Every other instance is keyed on a concrete carrier
(`optional<T>`, `vector<T>`) because there *is* a template to key on. Senders
have none, and the demo's `sender<T>` hid that by being one. Keying by
concept is the honest spelling: the object is chosen because the type
*behaves* as a sender, which is the duck-typing posture the library already
takes at use sites. The single-value restriction is the arity `when_all`
imposes on its children and the arity an Applicative element needs; senders
outside it are simply not registered, so the framework's existing
"no applicative_typeclass<T>" diagnostic fires. One object for all sender
types (rather than one per `S`) is what lets `pure(x)` return `just(x)` — a
different sender type — without the object having to know it.
**Sentinel:** any `applicative_typeclass<some_specific_adaptor_type>`
specialization is a regression to per-type keying.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-11 — Ratified by Steve: one concept-keyed object for all sender
  types. Tripwire for Stage 1: ambiguity between this and
  any future concept-keyed registration must be resolved by subsumption,
  never by adding a tie-breaker tag.

## sender-value-type-reading

**Question:** How is `applicative_value_t<S>` read for a sender?
**Status:** PROPOSED 2026-09-11
**Decision:** A specialization of `applicative_value` for
`single_value_sender` types reading
`value_types_of_t<S, empty_env, type_identity_t, type_identity_t>`, decayed.
The `void_t<typename T::value_type>` path is not used for senders even if
an adaptor happens to expose a `value_type`; the sender specialization is
constrained to types *without* a `value_type` member so the two partial
specializations stay disjoint (tripwire if this exclusion is ever the
reason a real sender fails to register).
**Why:** A sender's element type is what it *sends*, and the only
authoritative statement of that is its completion signatures. Reading a
`value_type` member would be reading a coincidence.
**Log:**
- 2026-09-11 — Drafted.

## runtime-arity-composition

**Question:** How does a runtime-sized Traversable compose contexts whose
n-ary combination is a new type at every step?
**Status:** PROPOSED 2026-09-11
**Decision:** The Applicative object may offer a native range composition
`collect(std::vector<S>) -> S'` with `applicative_value_t<S'> =
vector<applicative_value_t<S>>`. The vector Traversable's `traverse` prefers
`collect` when the object offers it and otherwise takes the existing
`pure`/`invoke` left fold. For P2300 senders `collect` is `all_of`, a
sender algorithm whose operation state owns *n* child operation states and
joins them. `collect` is optional in `applicative_object`.
**Why:** The fold is correct for every instance whose combination is
type-stable. It is not *wrong* for senders; it is unspellable, because a
loop cannot hold a value whose type changes. The alternatives are
sequencing (loses the independence the structure states) or erasing (pays
allocation and indirection per element, and hides the type the caller
wants to keep composing). `collect` moves the n-ary knowledge to the one
place that has it — the applicative object — and leaves the Traversable
generic. This is
[derived-op-native-preference](decisions.md#derived-op-native-preference)
applied on the Traversable side. The name is deliberately not `when_all_range`
or `sequence`: it is the range twin of `invoke`, and other applicatives
could offer it for performance (a single-pass `expected` collect avoids
`n` vector moves) without any sender vocabulary.
**Sentinel:** the vector Traversable naming any sender type, or any
`if constexpr` on "is a sender", violates this decision.
**Log:**
- 2026-09-11 — Drafted.

## erasure-boundary

**Question:** What counts as the erasure the design forbids?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve Downey, 2026-09-11 (planning discussion).
**Decision:** Forbidden in `execution.hpp` and `all_of.hpp`:
`std::function`/`move_only_function`, `any_sender`/`task`-style wrappers,
virtual dispatch, and per-element heap allocation of operation states.
Permitted: the single allocation holding the *n* child operation states
and result slots (its size is a function of `n` and
`sizeof(connect_result_t<S, R>)`, known at `connect`), and the result
`vector<T>` allocation. The composed sender's type is spelled from `S`.
**Why:** The claim under test is "no wrapper materialized". The
measurable form of that claim is *one* allocation whose size is
`n × (a compile-time constant)`, and a result type that carries `S` so
the caller keeps composing on the real thing. Anything else is the
demo sender in a different coat.
**Sentinel:** the Stage-2 allocation-count test.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-11 — Ratified by Steve: the boundary is measured (allocation
  count + result type spelled from `S`), not described.

## all-of-failure-semantics

**Question:** When a child of `all_of` errors or stops, what does the whole
do, and what does result order mean?
**Status:** PROPOSED 2026-09-11
**Decision:** Follow `when_all`: the first error or stop to arrive requests
stop on all siblings and is reported once all children have completed;
error wins over stopped; among errors the first to arrive wins. Result
*order* is input order, never completion order. Traversal order
([applicative-objects](decisions.md#applicative-objects), left-to-right)
governs `connect`/`start` order and result order; completion order is the
scheduler's business.
**Why:** `transpose` promises shape preservation; for a vector that is
positional. Matching `when_all` on failure means `transpose` over a
vector behaves exactly as the hand-written variadic form would for the
same children, which is the property the paper wants to state: `all_of`
*is* `when_all` at runtime arity, not a different algorithm.
**Log:**
- 2026-09-11 — Drafted. Stage 0 prior-art note to record where libunifex's
  `when_all_range` differs, if it does.

## demo-sender-fate

**Question:** Does the `std::function` demonstration `sender<T>` stay?
**Status:** PROPOSED 2026-09-11
**Decision:** Stays, unchanged, unconditional. It is renamed only in prose:
docs and paper call it the *demonstration* sender and say why it exists.
**Why:** It is the pedagogical instance in three published posts and the
test carrier the 2026-09-07 worklist designated for the sender-shaped lazy
monad (its erasure is what makes `sender<sender<A>>` a nameable type). It
also builds without the execution dependency. Deleting it would break
published examples for no gain; keeping it *and* the real instance is the
contrast the paper needs.
**Log:**
- 2026-09-11 — Drafted.

---

## sender-error-grade

**Question:** Are a sender's error completion signatures a grade, and is
`when_all`'s error union the join?
**Status:** OPEN 2026-09-11
**Note:** `completion_signatures` records the set of `set_error_t(E)` a
sender may raise; `when_all` unions them; `let_value` unions them; a sender
that cannot error is at ∅. That is `error_set` with the join, computed by
the execution framework itself. If so, the grading machinery could read a
sender's grade from its signatures with no error_set materialized, and
subsumption would be the signature-widening senders already do. Recording
because it is a paper-worthy observation and because it is a temptation:
not in scope for the execution plan; grading-footprint applies.

## sender-monad-instance

**Question:** Should P2300 senders get a Monad object (`bind = let_value`)?
**Status:** OPEN 2026-09-11
**Note:** Deferred, per the 2026-09-07 worklist: "same context, nested" is
a normalization problem for senders, not a trait fix. Revisit after
`collect` lands and after the monad-basis work has its sender-shaped
(demo) carrier tests.

## static-arity-array

**Question:** Should `std::array<S, N>` compose senders with a variadic
`when_all` directly, bypassing `all_of`?
**Status:** OPEN 2026-09-11
**Note:** Static arity makes the fold spellable as a pack expansion, so no
allocation at all is needed. Worth doing for the contrast (static shape →
zero allocations; runtime shape → one), possibly as `collect` on
`array<S, N>` rather than `vector<S>`. Not required by the plan.
