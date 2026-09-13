Read docs/transpose-execution-plan.md in full, then docs/decisions.md and
docs/decisions-execution-addendum.md. You are executing exactly one stage:
**execution-baseline** (§3, by slug — ignore the stage number).

Rules, from the plan's §0 divergence protocol, which you must follow:
- Deliverables, Acceptance, and Tripwires for your stage are the contract.
  Do not start work from a later stage, even if it looks easy.
- If reality contradicts a Why, stop and ask me. If it contradicts a What
  but the Why holds, propose an adaptation in a short note and wait.
- Never fix a tripwire condition to make it pass.
- Log every divergence as a dated entry under the implicated slug in
  docs/decisions.md (or a new OPEN section if no slug fits).
- Existing goldens do not change. If one does, stop.

Work on the current branch. Run the full suite under the gcc-debug and
llvm-debug presets before you claim acceptance. Finish with
docs/review/execution-<slug>.md: what was pinned, what diverged, what the
next stage's agent needs to know that the plan does not say.
