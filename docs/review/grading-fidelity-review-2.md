# Design-Fidelity Review 2 — Remediation and Paper-Revision (follow-up)

Follow-up to [grading-fidelity-review.md](grading-fidelity-review.md)
(reviewed `e955c62`). This pass covers the branch's movement to `482d27b`:
the merge of review PR #36, `3c6938f` ("Revise P3200 grading rationale",
stage paper-revision), and `482d27b` ("Remediate grading fidelity review").
Same charter, read-only. Method note: the branch build was again not re-run
(no gcc-16 here); dispositions marked *verified* below were confirmed by
re-running the review's compile probes with clang-18 against the shipped
headers at `482d27b`.

## Boot check delta

Two slugs the first review proposed as OPEN were answered at creation and
enter the log DECIDED:

- `evidence-combine-surface` — no generic public `combine_errors`; the model
  exposes native `error_set_combine`; framework-facing evidence dispatch
  stays private as `detail::combine_grade_evidence`.
- `graded-context-role` — `graded_context` IS the public spelling of
  grading-footprint's graded/ungraded exclusivity; shipped mixing-boundary
  constraints use it.

OPEN is now: `uniform-form-surface`, `datum-entry-point`,
`optional-grade-model` (the last with its "registered but not usable"
objection logged as superseded). No contradictions between the documents;
the plan's §2 and §6 were updated where the remediation touched them.

## Disposition of review-1 findings

| Finding | Disposition | Verification |
|---|---|---|
| A-1 cross-model hole in the bare cores | **Fixed, verified.** `declares_or_bare_v` now reads `!graded_context<CARRIER> \|\| is_expected_with_error_v<...>` (`expected.hpp:98-102`): "bare" means semantically ungraded, exactly the proposed resolution. Divergence + fix logged under `cross-model-mixing`. | Review-1's probe now fails to compile, and for the right reason (`all_declare_or_bare_v` false, graded core's `mixes_with_model` unsatisfied). A second probe confirms both the short-circuit and accumulating instances reject a foreign-model graded carrier while a truly bare `int` operand is still accepted with an unchanged `expected<int, errc>` deduction — no regression into `grading-footprint` territory. Constraint math also rejects a *bottom*-graded foreign carrier (`Fallible<T, never_fails>` is `graded_context`, model-mismatched); no sensor pins that corner, one-line note only |
| A-2 golden edited in stage commit | **Ruled.** Narrow retroactive ruling logged under `golden-vs-scheduled-assertions`: a stage may append an expired scheduled block's positive form to the golden file only when no existing golden assertion is weakened or edited. | Log entry present; scope is appropriately narrow ("not permission to change established golden coverage") |
| B-1 `combine_errors` / `is_error_set_of_v` | **Decided and removed.** New DECIDED `evidence-combine-surface`; both names deleted from `error_set.hpp`. | Repo-wide grep: zero residue in code, tests, or paper; `error_set_combine` and `detail::combine_grade_evidence` remain, matching the entry's letter |
| B-2 `graded_context` role | **Decided and made true.** New DECIDED `graded-context-role`; the concept is now load-bearing in the shipped constraint (A-1's fix), so `grade.hpp:130-137`'s comment is no longer aspirational. | grep: `graded_context` consumed by `expected.hpp:100` |
| C-1 no cross-model sensor via shipped instance | **Fixed.** `laws.test.cpp:392-414`: negative sensors through both shipped expected instances with positive controls (bare `int` accepted) so neither can pass vacuously. The probe function's second parameter implicitly converts from `int`, letting one function serve both polarities. | Read; semantics replicated and confirmed by the second probe above |
| C-2 hand-verified compile-error negatives | **Open, carried.** Not addressed; low severity, established repo pattern. | — |
| C-3 grep-shaped absences | Noted-only, still true: no parallel grade-reading trait; helpers still in `detail`. | grep |
| D-1 plan §6 stale open list | **Fixed.** `traverse-policy-surface` removed. | Read |
| D-2 unclosed divergence under `grading-footprint` | **Fixed.** "awaiting resolution" dropped; dated closing entry added. | Read |
| D-3 stale "(structural presence of `error_set`)" | **Fixed.** Decision text now: semantic `grade_of`/`graded_context` exclusivity, with declared-error matching on expected's lazy-spelling path — which is what the code does. | Read against `expected.hpp:85-104` |
| D-4 stale "CRTP base absorbs promotion" | **Fixed.** `grade-machinery-home` now says the CRTP base carries ap-from-bind and the defaulted `grade_subsume` coercion; mixing-point bare-operand lifting lives in the model-dispatched deduction helpers. Matches code. | Read against `apply.hpp`/`monad.hpp`/`grade.hpp:161-173` |
| D-5 missing `Decided by:`; unlogged subsume/subsumes split | **Fixed.** Fields added to the four implicated entries; `grade-operation-spelling` amended to name the predicate (`grade_subsumes`) and coercion (`grade_subsume`) distinctly, with a dated log entry; plan §2 one-liner updated. | Read |

The remediation commit was itself audited both directions: every code
change it makes is accounted for by a (new or amended) decision entry, and
every entry it adds or amends describes the code as it now is. No
collateral drift found. The remediation touched neither
`baseline_deduction.test.cpp` nor any deduced type a golden pins.

## Stage 10 — paper-revision (`3c6938f`) against the plan

Ordering constraint respected: the `mixing-point-vocabulary` ruling required
the model-dispatched repair to land before paper-revision starts, and it did
(`e955c62` precedes `3c6938f`).

Deliverables, item by item (`D3200R0.md`):

- Coherence argument — present ("Coherent grading for fallible contexts"):
  nominal `error_set`, subset-only implicit widenings with the direction
  stated, canonicalization as identity-not-spelling-or-position. Faithful to
  `error-set-identity`, including the two-level may-raise/did-raise reading
  and the left-biased first-witness point tied to normative order — which is
  `accumulation-evidence`'s "that decision pays for this one" argument,
  correctly reproduced.
- Compatibility section — present, and it is the decided three sentences
  (pure paths / unmixed-by-lazy-join with the singleton-semantic-grade
  nuance from `plain-error-grade-reading` / previously-invalid mixes now
  additive) plus the detection-idiom caveat from `grading-footprint`.
- Deliberate omissions — both present with their reasons: non-idempotent
  grade monoids (shape-dependence forcing indexed types or a perpendicular
  NTTP axis, per `grade-generality`) and the std law-checker with the
  constraint-capture / pay-once-vs-pay-per-use rationale, matching
  `laws.hpp`'s own header comment. A third omission (Monad stays rationale)
  is consistent with the existing "Why not Monad" section.
- Normative traversal order — added to the proposed-operations list.
- Graded-monad language confined to rationale — holds trivially and
  substantively: the Wording section is still "[To be written.]", and all
  grade language sits in design/rationale sections. One forward note, not a
  finding: when wording is written, the plan's WG21-posture sentence ("error
  types are deduced, compose by set union, and subsumption is implicit and
  coherent") is the normative claim it should carry.

No unaccounted design-shaped choices entered the shipped headers in this
range — the only header changes are B-1's removals and A-1's constraint
fix, both slugged.

## New findings

None in classes A, B, or D. Class C: C-2 carries (hand-verified negatives
for the accumulating bind diagnostic and `recover`'s CHECKED half remain
documentation-pinned only), and one optional one-liner — the cross-model
sensors test only a top-graded foreign carrier (`may_fail`); a
`Fallible<T, never_fails>` operand is also rejected by the same constraint
arithmetic but nothing pins it.

## Verdict

The remediation is complete and faithful: all nine addressed findings from
review 1 are verified fixed or ruled, the two proposed OPEN entries were
answered into the log in the review's own draft shape, and the fix for the
one code violation is the minimal semantic tightening the log now
describes, with sensors that would catch its regression. Stage 10 delivers
exactly the plan's rationale items and nothing normative. The documents
remain sufficient — this follow-up was conducted entirely from them, the
diff, and two compile probes. Nothing blocks final delivery from this
review's perspective; the only open review items are the low-severity C-2
and the still-open design questions the log already tracks.
