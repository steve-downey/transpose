# Stage review — execution-baseline

Stage [execution-baseline](../transpose-execution-plan.md#execution-baseline),
the first stage of [transpose-execution-plan.md](../transpose-execution-plan.md).
Run 2026-09-13.

**Status: four of five deliverables complete; deliverable 2 is STOPPED and
needs Steve.** Nothing is blocked behind it — the suite is green in every
configuration and the later stages have what they need — but the question it
raises is a one-way door and the plan's own protocol forbids taking it by
default.

---

## 1. The thing to read first

The plan was drafted without
[p2300-front-door-shape](../decisions.md#p2300-front-door-shape) in view, and
that entry is a Steve ruling dated 2026-09-11 — the same day the plan was
drafted — answering very nearly the same question.
[transpose-execution-plan.md §1](../transpose-execution-plan.md#context) states
that "a survey of every steve-downey repository on 2026-09-11 found no
implementation over real `std::execution` senders anywhere". That is false of
*this* repository on that date: `examples/p2300_adapter.hpp` and
`tests/beman/transpose/p2300.test.cpp` are exactly such an implementation.

This does not sink the plan. Its load-bearing argument — the fold cannot work,
the way through is a native n-ary composition, and nobody has written one —
survives intact, and §2 below says why. But three stage premises moved, and
the consequences are logged under the implicated slugs rather than left in
this note, per divergence protocol rule 3.

---

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

**The addendum, merged.** Ten entries now live in
[decisions.md](../decisions.md): seven from the addendum, the three OPEN
questions it carried, and one new OPEN slug this stage raised
([execution-toolchain-floor](../decisions.md#execution-toolchain-floor)).
Attribution is explicit: `sender-instance-keying` and `erasure-boundary` carry
Steve's 2026-09-11 ratification; `sender-value-type-reading`,
`runtime-arity-composition`, `all-of-failure-semantics` and `demo-sender-fate`
carry **"Default-as-drafted at Stage 0 … not individually ruled by Steve"**, so
a later reader can tell a default from a ruling.

---

## 3. What diverged

Five items. Each is logged under its slug; this is the index.

**(a) Deliverable 2 is STOPPED.**
[execution-dependency-shape](../decisions.md#execution-dependency-shape) is
merged as **OPEN**, not default-as-drafted, because taking the default would
reverse a Steve ruling by inaction. The drafted shape (option
`BEMAN_TRANSPOSE_WITH_EXECUTION`, default ON when found, headers in
`include/` reachable from `transpose.hpp`) is weaker on both axes than the
decided shape (`BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, OFF always, adapter
under `examples/`). The two *Whys* agree completely — both want the front door
light — so this is divergence protocol rule 2: propose and wait. Three
separable axes are set out in the slug; **axis 2 (header location) is the only
one that changes what an installation pulls in, and it is what Stage 1 and
Stage 2's file paths are written against.**

**(b) `empty_env` does not exist.** Logged under
[sender-value-type-reading](../decisions.md#sender-value-type-reading). The
pinned dependency spells the empty environment `env<>`, the newer P2300
spelling after LWG replaced the dedicated type. A What, handled the way the
Stage 0 tripwire handles the namespace case: log, proceed with the real one.

**(c) Stage 1's basis already exists.** Logged under
[p2300-front-door-shape](../decisions.md#p2300-front-door-shape).
`P2300ApplicativeImpl` already supplies `pure = just` and
`invoke = when_all | then` with forwarded operands — Stage 1's deliverable 1,
second bullet, written and green. Stage 1's real content is the registration
and the value reading, which the adapter deliberately did not do.

**(d) The plan's Stage 2 completion signatures are a revision behind.** Logged
under [all-of-failure-semantics](../decisions.md#all-of-failure-semantics).
P3887R1 (LWG-approved 2025-11) says `when_all` advertises `set_stopped` only
if a child does; the plan lists it unconditionally. Because the decision
grounds `all_of` in "follow `when_all`", the approved `when_all` is the one to
follow.

**(e) The plan's Stage 2 layout trips the plan's Stage 2 tripwire.** Logged
under [erasure-boundary](../decisions.md#erasure-boundary). Deliverable 2 asks
for child operation states in one allocation *and* slots as a separate
`vector<optional<T>>` — two blocks before the result vector — while the
tripwire stops on more than one allocation that is not the result. The
tension is internal to the plan.

---

## 4. What the next stage's agent needs that the plan does not say

**Read [execution-dependency-shape](../decisions.md#execution-dependency-shape)
before writing a single file path.** Stage 1's deliverable 1 says
`include/beman/transpose/execution.hpp`, included from `transpose.hpp`. Whether
that location is permitted is exactly what is open. If Steve rules for
`examples/`, Stage 1's deliverables are unchanged in substance and relocated in
fact — adapt under rule 2, do not follow the plan's paths literally, and do not
treat the relocation as licence to change what the header contains.

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
| 1 | Merge addendum into `decisions.md` | **Done**, with `execution-dependency-shape` held OPEN — see §3(a) |
| 2 | Bring in `beman.execution` per the drafted shape | **STOPPED**, rule 2. Recording half done: namespace, standard level, pin, resolution path all in §2 |
| 3 | Compile probe under every preset | **Done** — `execution_probe.test.cpp`, green on both presets under the existing option guard |
| 4 | Golden capture of the demo sender | **Done** — `demo_sender_golden.test.cpp`; existing goldens untouched |
| 5 | Prior-art note | **Done** — [prior-art-when-all-range.md](prior-art-when-all-range.md), with two findings that contradict Stage 2's text |

**Acceptance.** "Probe green on gcc and llvm presets with the option ON" —
yes. "Whole existing suite green with the option OFF and ON" — yes, all four
combinations, zero failures, no existing assertion changed. "Addendum merged"
— yes, with one entry deliberately merged as OPEN. Subject to the compiler
substitution recorded in §4, which is the one respect in which this stage's
acceptance is weaker than the words ask for.

**Tripwires.** None fired that were not already answered. Package-not-found-
under-vcpkg: already answered, 2026-09-11. Namespace differs: it does not.
Any existing golden changed: none — verified by diff, zero deletions.
