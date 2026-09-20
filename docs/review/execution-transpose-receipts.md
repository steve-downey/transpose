# Stage review — transpose-receipts

<!-- SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception -->

Stage 4 of [transpose-execution-plan.md](../transpose-execution-plan.md),
by slug [transpose-receipts](../transpose-execution-plan.md#transpose-receipts).
Executed 2026-09-20 on `claude/transpose-patch-plan-5cwbdy`, on top of
Stage 3 (PR #61, draft).

New decision entry:
[execution-runloop-signatures](../decisions.md#execution-runloop-signatures) —
an upstream defect, recorded rather than decided.

---

## 1. The result

`examples/transpose_execution_example.cpp` builds and runs under the option,
on gcc-debug and llvm-debug, as a registered test. Its whole output:

```text
scene 1 -- concurrent children
  completion order: 5 4 3 2 1 0
  result order:     0 1 4 9 16 25
  (the results are the squares, in input order; the log is the order they finished)

scene 2 -- single-threaded, still lazy
  after transpose: 0 children started, 0 queued
  after start:     4 children started, 4 queued, result not ready
  after draining:  result ready
  values:          100 200 300 400

scene 3 -- a child fails
  caught: child 0 could not do the work
  siblings that saw the stop request: 4 of 4
```

Scene 1 is the receipt the demonstration sender could never give: the
completion order is exactly reversed against the result order, because the
delays are staggered to force it, so the run *proves* rather than hopes that
shape preservation is positional and indifferent to what the threads did.

Scene 2 is laziness at three observations instead of one. The earlier blog
post could show "nothing has run yet"; it could not show *started but not
performed*, because a `std::function<T()>` thunk has no operation state to
start.

Scene 3 shows the error reaching the caller and all four siblings observing
the stop request — the `when_all` contract, running.

---

## 2. The divergence, and it is not a small one

**Neither scheduler the plan named is usable against the pinned
`beman.execution` (`d24898d`).** Deliverable 1 asked for
`schedule(pool) | then(work)` and a `run_loop` scene.

1. `get_parallel_scheduler()` does not link — Stage 2 found this and it is
   unchanged.
2. `run_loop::sender` **cannot report its completion signatures with no
   environment.** `run_loop::sender::get_completion_signatures` evaluates
   `get_stop_token(std::declval<Env>()...)`, which for an empty `Env` pack is
   `get_stop_token()` with no arguments. That is ill-formed inside the return
   type deduction of a `consteval` function, so it is a hard error, not a
   constraint failure.

**The second is not ours, and that was established rather than assumed.**
`ex::when_all` over two `run_loop` senders fails with the same four
diagnostics, with no part of this library in the translation unit. A
`run_loop` sender on its own is fine: `sync_wait(schedule(sch) | then(f))`
runs, and `single_value_sender` of it is `true`. The defect is reached only by
an algorithm that asks a child for its signatures with no environment — which
is what `when_all` does, and therefore what `all_of` does.

It was tempting to have `all_of` invent an environment to ask under. That
would make this library's algorithm differ from the standard one it is
modelled on in order to paper over a bug in a pinned dependency, and the
agreement is worth more than the scene.

**What the example does instead, and what it needs from Steve.** Scene 2 uses
a deferred queue written in the example itself: children that enqueue their
completion when started, drained on the main thread. It serves the
deliverable's Why exactly — single-threaded, still lazy — and needs no
scheduler. **This is an adaptation of a What. The Why holds, and Steve has not
ruled on the substitution.** Scene 2 is self-contained; if he would rather it
wait for a dependency bump, it lifts out cleanly.

---

## 3. The post, and what a build container cannot finish

`papers/blog/transpose-over-real-senders.org` is written, slotted as item 8 in
`index.org`, with the series navigation rewired on both sides. The erratum
paragraph deliverable 3 asks for is in `transpose-at-work.org`, as a note
block at the head of its "Deferral survives the composition" section, saying
plainly that its `sender` is the demonstration type and linking forward.

**Three things about the post are deliberately unfinished**, and they are
called out in a `#+begin_comment` block at its head so they cannot be missed:

- Its code blocks are pasted, not transcluded. Every other post in the series
  pins its code with `[[orgit-file:...]]` against an annotated
  `blog/<post-basename>` tag. Creating that tag is not something to do from
  here.
- Its output blocks are captured verbatim from a real gcc-14 Debug run rather
  than produced by the babel shell blocks the other "with running code" posts
  use against the gcc-16 RelWithDebInfo build.
- **The voice is an imitation.** The plan said "written in Steve's voice;
  draft only, flagged for his edit", and it should be read as exactly that.

**The `.md` was not generated, and could not safely have been.** `make
blog-md` needs Emacs (absent here) plus a first-run ELPA install, and its
export *re-runs* the babel shell blocks in the existing posts against gcc-16
binaries that do not exist in this container — so a run from here would not
merely fail, it could strip the captured output out of
`transpose-at-work.md`. What was done instead is the mechanical part only:
the navigation links and the index entry in the three affected `.md` files,
matched exactly to the surrounding generated lines. The erratum prose and the
new post's whole `.md` are pending `make blog-md` on a machine with the
toolchain.

This is now the third artifact class this branch has left for a tool it does
not have, after the two wording fragments. All three are recorded, none are
hand-forged.

---

## 4. Acceptance

| Deliverable | Result |
|---|---|
| 1 — the example, three scenes | Done; scene 2 by an adaptation, [execution-runloop-signatures](../decisions.md#execution-runloop-signatures) |
| 2 — the blog post, slotted after "Transpose at Work" | `.org` done and wired; `.md` pending the pipeline |
| 3 — erratum on "Transpose at Work" | Done in `.org`; `.md` pending the pipeline |

**Acceptance clause.** "Example builds and runs under the option" — yes, on
both presets, as a registered test. "Post renders" — **not verified, and
cannot be from here**; no Emacs, and the exporter would run binaries this
container does not have. "Steve has the draft" — that is this.

| Preset | Option OFF | Option ON |
|---|---|---|
| gcc-debug | 252 | 282 |
| llvm-debug | 252 | 282 |

Before this stage: 252 / 281. The one added test is the example itself. All
green; all nine pre-commit hooks clean; `include/` untouched by this stage.

---

## 5. What Stage 5's agent needs

**Stage 5's acceptance clause needs reinterpreting before it is used.** It
says "paper builds; wording pipeline output unchanged". Two fragments are
already drifted by this branch's own work
([collect-hook](../decisions.md#collect-hook)), so "unchanged" can only mean
"unchanged beyond those two" until specgen is available. Do not take a
diff-clean `make wording-check` as the bar, and do not regenerate to make one.

**The paper note is non-normative and the plan means it.** No wording changes.
The three things Stage 5 has to say — the second domain's example references
the real instance, the Traversable/Applicative split is the seam that let
runtime-arity composition live in the applicative object, and
`sender-error-grade` is future work — are all design rationale.

**`all_of` is not being proposed, and the post says so too.** If the paper
text starts to read like a proposal for a range `when_all`, that belongs to
the execution papers and the note has gone wrong.

**The measurement to quote is still Stage 2's**, unchanged by Stages 3 and 4:
one allocation at `connect` of *n* × `sizeof(holder)` bytes, one at `start`
for the result vector, two in total, and `decltype(all_of(v))` spelled from
`S`.
