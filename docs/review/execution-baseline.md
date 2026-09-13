# Stage review — execution-baseline

Stage [execution-baseline](../transpose-execution-plan.md#execution-baseline),
the first stage of [transpose-execution-plan.md](../transpose-execution-plan.md).
Run 2026-09-13.

**Status: COMPLETE.** Ruled by Steve 2026-09-13, same day: the dependency
question is withdrawn rather than answered, so Stage 0 shrank to three
deliverables and all three are done. The suite is green in every
configuration.

---

## 1. The thing to read first

The plan was drafted without
[p2300-front-door-shape](../decisions.md#p2300-front-door-shape) in view, and
that entry is a Steve ruling dated 2026-09-11 — the same day the plan was
drafted — answering very nearly the same question.
[transpose-execution-plan.md §1](../transpose-execution-plan.md#context)
originally stated that "a survey of every steve-downey repository on
2026-09-11 found no implementation over real `std::execution` senders
anywhere". That was false of *this* repository on that date:
`examples/p2300_adapter.hpp` and `tests/beman/transpose/p2300.test.cpp` are
exactly such an implementation. **That paragraph has been replaced**; §1 now
points at the adapter and tells the next agent to read it first.

**The ruling, in one line: `p2300-front-door-shape` stands, and nothing
execution-dependent enters `include/`.** Consequences, all now written into
the plan and the log:

- Stages 1–3 build on `examples/p2300_adapter.hpp` where it lives.
- `all_of` lands as `examples/all_of.hpp` under
  `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, pinned `d24898d`.
- The Stage 3 `collect` hook in `sequence.hpp` is the **only** `include/`
  change the plan makes, and it stays sender-free — which
  [runtime-arity-composition](../decisions.md#runtime-arity-composition)'s own
  Sentinel already demanded, from the opposite direction.
- [execution-dependency-shape](../decisions.md#execution-dependency-shape) and
  [demo-sender-fate](../decisions.md#demo-sender-fate) are **WITHDRAWN** as
  superseded. Both slugs are kept, redirecting, so existing links resolve.
- **Stage 1 becomes an audit**, not a build: hold the adapter up against the
  keying and value-reading entries and bring it to them, plus the law harness
  and move-only coverage. Gaps get logged under
  [sender-instance-keying](../decisions.md#sender-instance-keying), which is
  now Stage 1's logbook by explicit direction.
- Stage 0's dependency-wiring deliverable is **struck**, the dependency
  having been wired since 2026-09-11.

The one thing the ruling deliberately did *not* settle:
`p2300-front-door-shape`'s Consequences paragraph saying runtime-sized
transposition "needs a type-erased sender" is **not** ratified.
[runtime-arity-composition](../decisions.md#runtime-arity-composition) and
[erasure-boundary](../decisions.md#erasure-boundary) are kept and they say the
opposite. **Stage 2 carries a standing instruction as deliverable 7:** when
the allocation-count test passes, write a dated Log entry under
`p2300-front-door-shape` recording the refutation — and if it does not pass,
write that there instead. Until one of those entries exists, the paragraph
stands as the live claim.

## 2. What was pinned

**The dependency, measured rather than assumed.** All of the following were
verified in this stage, not read off documentation:

| Fact | Value |
|---|---|
| Resolution path | `lockfile.json` FetchContent — **not** vcpkg (`vcpkg.json` lists only `catch2`) |
| Pinned commit | `d24898d7264e74fb723b50d6275a5d05f65ddb20` |
| Exported namespace | `beman::execution` (516 uses), matching the decision text |
| Legacy namespace | `beman::execution26` still ships, two headers under `include/beman/execution26/` — this is what `compile-time-scheme` was using |
| Standard level | C++23 or greater; declares `cxx_std_${CMAKE_CXX_STANDARD}`, so it inherits ours |
| Empty environment | `beman::execution::env<>` — **there is no `empty_env`** |

**The suite, in every configuration.** Four combinations, all green, identical
counts across presets:

| Preset | Option | Tests |
|---|---|---|
| gcc-debug | OFF | 240 |
| gcc-debug | ON | 247 |
| llvm-debug | OFF | 240 |
| llvm-debug | ON | 247 |

Baseline before this stage was 239 / 244. The four added tests are this
stage's own: one in `demo_sender_golden.test.cpp` (built always) and two in
`execution_probe.test.cpp` plus its static assertions (built under the
option). **No existing test, golden, header, or example was modified.** The
whole diff is additions: `git diff --stat` reports 483 insertions and zero
deletions across `docs/decisions.md` and
`tests/beman/transpose/CMakeLists.txt`.

**The demonstration sender's "unconditional".**
`tests/beman/transpose/demo_sender_golden.test.cpp` is a translation unit
whose only library include is `sender.hpp`, built in every configuration. It
pins what [demo-sender-fate](../decisions.md#demo-sender-fate) claims and
nothing that was already pinned: the front-door *deductions* were already
golden in `baseline_deduction.test.cpp`, which is likewise built in every
configuration, so repeating them would have been duplication rather than
coverage. What it adds is that the demonstration sender needs no execution
dependency to exist, and that it is invariant under composition — the property
that makes it a demonstration and that
[erasure-boundary](../decisions.md#erasure-boundary) forbids in the real
instance.

**The dependency canary.** `tests/beman/transpose/execution_probe.test.cpp`
includes `beman.execution` and nothing of ours, so a broken dependency fails
there rather than inside the adapter's diagnostics. It probes exactly the four
names the plan's registration is built on — `just`, `when_all`, `then`,
`sync_wait` — plus the completion-signature reading Stage 1 needs.

**The addendum, merged and reconciled.** Eleven slugs now live in
[decisions.md](../decisions.md), and the addendum file carries a SUPERSEDED
banner so no later agent reads decisions out of the draft — which is the
mistake that caused this reconciliation. Statuses after the ruling:

| Slug | Status |
|---|---|
| `sender-instance-keying` | DECIDED — Steve, 2026-09-11. Now also **Stage 1's logbook** |
| `erasure-boundary` | DECIDED — Steve, 2026-09-11 |
| `sender-value-type-reading` | **PROPOSED** — kept as written, not graduated |
| `runtime-arity-composition` | DECIDED default-as-drafted; **CONTESTED**, settled by Stage 2 measurement |
| `all-of-failure-semantics` | DECIDED default-as-drafted |
| `execution-dependency-shape` | **WITHDRAWN** — superseded, redirects |
| `demo-sender-fate` | **WITHDRAWN** — superseded, redirects; its golden stays |
| `execution-toolchain-floor` | OPEN — new, raised by this stage |
| `sender-error-grade`, `sender-monad-instance`, `static-arity-array` | OPEN — carried over |

Attribution is explicit throughout: entries that are Steve rulings say so,
and entries taken by default say **"Default-as-drafted at Stage 0 … not
individually ruled by Steve"**, so a later reader can tell one from the other
without archaeology.

---

## 3. What diverged

Five items. Each is logged under its slug; this is the index.

**(a) The dependency question — RESOLVED by withdrawal.** Raised at Stage 0 as
a rule-2 divergence and stopped rather than defaulted, because taking the
draft would have reversed a Steve ruling by inaction. Ruled the same day:
[execution-dependency-shape](../decisions.md#execution-dependency-shape) is
**WITHDRAWN**, [p2300-front-door-shape](../decisions.md#p2300-front-door-shape)
stands, and the plan's Stage 0 deliverable 2 is struck rather than performed.
No new option, no new pin, no `include/` dependency.

**(b) `empty_env` does not exist.** Logged under
[sender-value-type-reading](../decisions.md#sender-value-type-reading). The
pinned dependency spells the empty environment `env<>`, the newer P2300
spelling after LWG replaced the dedicated type. A What, handled the way the
Stage 0 tripwire handles the namespace case: log, proceed with the real one.
The plan's Stage 1 text now says `env<>`.

**(c) Stage 1's basis already exists.** Logged under
[p2300-front-door-shape](../decisions.md#p2300-front-door-shape).
`P2300ApplicativeImpl` already supplies `pure = just` and
`invoke = when_all | then` with forwarded operands. This is why the ruling
turned Stage 1 into an audit; the plan's Stage 1 has been rewritten
accordingly, and its known starting gap is stated: no `applicative_typeclass`
registration, no `applicative_value` reading.

**(d) The plan's Stage 2 completion signatures were a revision behind.**
Logged under
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics).
P3887R1 (LWG-approved 2025-11) makes `when_all`'s `set_stopped` conditional on
a child sending it; the plan listed it unconditionally. **Corrected in the
plan**, inline at the deliverable, with the reasoning quoted.

**(e) The plan's Stage 2 layout tripped the plan's Stage 2 tripwire.** Logged
under [erasure-boundary](../decisions.md#erasure-boundary). Deliverable 2
asked for child op-states in one allocation *plus* a separate
`vector<optional<T>>` — two blocks before the result vector, while the
tripwire stops at more than one. **Corrected in the plan**: the tripwire is
the sensor and stands, the layout is what gives.

---

## 4. What the next stage's agent needs that the plan does not say

**Stage 1 is an audit. Read `examples/p2300_adapter.hpp` first.** It is 90
lines, already does the `pure`/`invoke` half correctly — including the
forwarding the plan calls out as a correction to the demo sender's `const&`
spelling — and its header comment states what writing it established that it
*cannot* establish. The registration and the value reading are the gap.
Everything stays in `examples/`; **nothing execution-dependent enters
`include/`**, and the plan's Stage 1 tripwires now say so. Log every gap the
audit finds under
[sender-instance-keying](../decisions.md#sender-instance-keying), which the
ruling designated as Stage 1's logbook.

**The arity constraint has to be checked before the alias is instantiated.**
Measured this stage and pinned in the probe: with `Tuple` and `Variant` both
`std::type_identity_t`, `value_types_of_t<decltype(just(1)), env<>>` is exactly
`int`, but the same alias over `decltype(just(1, 2))` is **ill-formed, not
merely different**, because `type_identity_t` is not variadic. So a `requires`
clause that names the alias is a hard error on a two-argument sender rather
than a graceful constraint failure. This is the same non-SFINAE-friendly hazard
[functor-monad-grounding](../decisions.md#functor-monad-grounding) records for
`OptionalMonadImpl::bind`, in a new place, and it is the difference between
Stage 1's registration failing cleanly and Stage 1's registration diagnosing.

**There is a working adapter to read.** `examples/p2300_adapter.hpp` is 90
lines and already does the `pure`/`invoke` half correctly, including the
forwarding that the plan calls out as a correction to the demo sender's
`const&` spelling. Its header comment states what writing it established that
it *cannot* establish. Start there rather than from scratch.

**The toolchain is narrower than either project's docs suggest.** New OPEN slug
[execution-toolchain-floor](../decisions.md#execution-toolchain-floor). This
repository's log records verification on GCC 15.2 / GCC 16 / Clang 23;
`beman.execution` documents GCC 15–14 and Clang 22–19. The intersection is GCC
14–15 and Clang 19–22 — **the two compilers this repository most recently
verified on are both outside the dependency's stated support.** Measured here,
and the reason it is a question rather than a note: GCC 13.3 cannot build this
library at all (no deducing-this), and Clang 18.1.3 **crashes the frontend** on
`error_set.hpp`. The cliff is one version below where the dependency's matrix
starts.

*This stage therefore ran on GCC 14.2.0 and Clang 20.1.2*, installed into the
container, with the toolchain files untouched and the version selected by
symlinks ahead of them on `PATH`. That is a **deviation from the acceptance
criterion as literally worded** — it says the gcc-debug and llvm-debug presets,
and those presets name bare `gcc`/`clang`. The presets were used unmodified;
what changed is which compiler those names resolve to. Both chosen versions sit
inside the dependency's supported matrix and below this repository's verified
set, so a green result here is weaker evidence than a green result on GCC 16 /
Clang 23 would be. Anyone re-running should not read these counts as
reproducing CI.

**The vcpkg tripwire has already fired and is already answered.** Stage 0's
tripwire says propose FetchContent and wait; the repository has done that since
2026-09-11 and it works. Nothing to propose.

**`all_of` really is unwritten.** The prior-art survey
([prior-art-when-all-range.md](prior-art-when-all-range.md)) found exactly one
implementation anywhere — libunifex's undocumented `when_all_range` — and clear
negatives for stdexec, for `beman.execution`, and for every WG21 paper. stdexec
carries an open issue asking for the facility. The plan's "the piece nobody has
written" is very nearly literally true.

---

## 5. Deliverable-by-deliverable

| # | Deliverable | Outcome |
|---|---|---|
| 1 | Merge addendum into `decisions.md` | **Done**, reconciled per the ruling: two withdrawn, one kept PROPOSED, three kept; addendum banner-marked SUPERSEDED |
| 2 | Bring in `beman.execution` | **STRUCK** by the ruling — already wired since 2026-09-11. Recording half done: namespace, standard level, pin, resolution path in §2 |
| 3 | Compile probe under every preset | **Done** — `execution_probe.test.cpp`, green on both presets |
| 4 | Golden capture of the demo sender | **Done** — `demo_sender_golden.test.cpp`; existing goldens untouched |
| 5 | Prior-art note | **Done** — [prior-art-when-all-range.md](prior-art-when-all-range.md), with two findings that corrected Stage 2's text |

Also done under the ruling, beyond the original deliverables: the plan's §1
survey paragraph replaced; Stage 0 shrunk; Stage 1 rewritten as an audit;
Stage 2 relocated to `examples/all_of.hpp`, corrected on both counts above,
and given the refutation log entry as deliverable 7; Stage 3 marked as the
only `include/` change with a sender-free tripwire.

**Acceptance.** "Probe green on gcc and llvm presets with the option ON" —
yes. "Whole existing suite green with the option OFF and ON" — yes, all four
combinations, zero failures, no existing assertion changed. "Addendum merged"
— yes, reconciled. Subject to the compiler substitution recorded in §4, which
is the one respect in which this stage's acceptance is weaker than the words
ask for.

**Tripwires.** None fired that were not already answered.
Package-not-found-under-vcpkg: already answered 2026-09-11, and the
deliverable that would have acted on it is struck. Namespace differs: it does
not. Any existing golden changed: none — verified by diff, zero deletions to
any pre-existing file.
