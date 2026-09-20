/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "transpose", "index.html", [
    [ "Baseline vocabulary audit", "md_docs_baseline_vocabulary_audit.html", [
      [ "Method", "md_docs_baseline_vocabulary_audit.html#autotoc_md1", null ],
      [ "Finding: the surface carries no error vocabulary at all", "md_docs_baseline_vocabulary_audit.html#autotoc_md2", null ],
      [ "Finding: <tt>expected</tt> is absent from the entire repository", "md_docs_baseline_vocabulary_audit.html#autotoc_md3", null ],
      [ "Finding: <tt>join</tt> is already taken", "md_docs_baseline_vocabulary_audit.html#autotoc_md4", null ],
      [ "Public surface inventory", "md_docs_baseline_vocabulary_audit.html#autotoc_md5", null ],
      [ "Tripwire check", "md_docs_baseline_vocabulary_audit.html#autotoc_md6", null ]
    ] ],
    [ "CODING_RULES", "md_docs_CODING_RULES.html", [
      [ "Coding Rules", "md_docs_CODING_RULES.html#autotoc_md7", [
        [ "Semantic Defaults", "md_docs_CODING_RULES.html#autotoc_md8", null ],
        [ "Project Layout", "md_docs_CODING_RULES.html#autotoc_md9", null ],
        [ "File Prolog and Includes", "md_docs_CODING_RULES.html#autotoc_md10", null ],
        [ "CMake and Build Graph", "md_docs_CODING_RULES.html#autotoc_md11", null ],
        [ "C++ Structure", "md_docs_CODING_RULES.html#autotoc_md12", null ],
        [ "Language and Tooling", "md_docs_CODING_RULES.html#autotoc_md13", null ],
        [ "Typeclass Design", "md_docs_CODING_RULES.html#autotoc_md14", null ],
        [ "Foldable Rules", "md_docs_CODING_RULES.html#autotoc_md15", null ],
        [ "Applicative Rules", "md_docs_CODING_RULES.html#autotoc_md16", null ],
        [ "Traversable Rules", "md_docs_CODING_RULES.html#autotoc_md17", null ],
        [ "Test Rules", "md_docs_CODING_RULES.html#autotoc_md18", null ],
        [ "Slide and Transclusion Rules", "md_docs_CODING_RULES.html#autotoc_md19", null ],
        [ "Prose and Documentation Formatting", "md_docs_CODING_RULES.html#autotoc_md20", null ]
      ] ]
    ] ],
    [ "Coordination worklist — 2026-07-13", "md_docs_coordination_worklist_2026_07_13.html", [
      [ "1. Add a \"Why not Monad\" section to D3200R0", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md22", null ],
      [ "2. Label unproposed repo surface as evidence", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md23", null ],
      [ "3. Keep Traversable free of a Foldable superclass requirement", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md24", null ],
      [ "4. Relationships essay hook", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md25", null ],
      [ "5. Remove the pure/apply forms; <tt>invoke</tt> is the applicative core", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md26", null ],
      [ "6. BinaryTree example cross-check (standing item)", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md27", null ]
    ] ],
    [ "Coordination worklist — 2026-09-07", "md_docs_coordination_worklist_2026_09_07.html", [
      [ "1. Named monoids; no numeric defaults", "md_docs_coordination_worklist_2026_09_07.html#autotoc_md29", null ],
      [ "2. Ground the full Functor instance in the Monad instance", "md_docs_coordination_worklist_2026_09_07.html#autotoc_md30", null ],
      [ "3. Monad: admit the other complete bases (contingency)", "md_docs_coordination_worklist_2026_09_07.html#autotoc_md31", null ],
      [ "4. Functor combinators as evidence; higher kinds via Reflection (far horizon)", "md_docs_coordination_worklist_2026_09_07.html#autotoc_md32", null ]
    ] ],
    [ "Decision Log addendum — transpose over real senders", "md_docs_decisions_execution_addendum.html", [
      [ "execution-dependency-shape", "md_docs_decisions_execution_addendum.html#autotoc_md35", null ],
      [ "sender-instance-keying", "md_docs_decisions_execution_addendum.html#autotoc_md36", null ],
      [ "sender-value-type-reading", "md_docs_decisions_execution_addendum.html#autotoc_md37", null ],
      [ "runtime-arity-composition", "md_docs_decisions_execution_addendum.html#autotoc_md38", null ],
      [ "erasure-boundary", "md_docs_decisions_execution_addendum.html#autotoc_md39", null ],
      [ "all-of-failure-semantics", "md_docs_decisions_execution_addendum.html#autotoc_md40", null ],
      [ "demo-sender-fate", "md_docs_decisions_execution_addendum.html#autotoc_md41", null ],
      [ "sender-error-grade", "md_docs_decisions_execution_addendum.html#autotoc_md43", null ],
      [ "sender-monad-instance", "md_docs_decisions_execution_addendum.html#autotoc_md44", null ],
      [ "static-arity-array", "md_docs_decisions_execution_addendum.html#autotoc_md45", null ]
    ] ],
    [ "Decision Log — grading in beman.transpose", "md_docs_decisions.html", [
      [ "empty-grade-spelling", "md_docs_decisions.html#autotoc_md48", null ],
      [ "grading-footprint", "md_docs_decisions.html#autotoc_md50", null ],
      [ "error-set-identity", "md_docs_decisions.html#autotoc_md52", null ],
      [ "grade-machinery-home", "md_docs_decisions.html#autotoc_md54", null ],
      [ "applicative-objects", "md_docs_decisions.html#autotoc_md56", null ],
      [ "recover-grade-inference", "md_docs_decisions.html#autotoc_md58", null ],
      [ "grade-generality", "md_docs_decisions.html#autotoc_md60", null ],
      [ "uniform-form-surface", "md_docs_decisions.html#autotoc_md62", null ],
      [ "datum-entry-point", "md_docs_decisions.html#autotoc_md64", null ],
      [ "traverse-policy-surface", "md_docs_decisions.html#autotoc_md66", null ],
      [ "optional-grade-model", "md_docs_decisions.html#autotoc_md68", null ],
      [ "plain-error-grade-reading", "md_docs_decisions.html#autotoc_md70", null ],
      [ "grade-model-identity", "md_docs_decisions.html#autotoc_md72", null ],
      [ "expected-instance-introduction", "md_docs_decisions.html#autotoc_md74", null ],
      [ "grade-operation-spelling", "md_docs_decisions.html#autotoc_md76", null ],
      [ "accumulation-evidence", "md_docs_decisions.html#autotoc_md78", null ],
      [ "multi-witness-elimination", "md_docs_decisions.html#autotoc_md80", null ],
      [ "golden-vs-scheduled-assertions", "md_docs_decisions.html#autotoc_md82", null ],
      [ "bottom-grade-identity", "md_docs_decisions.html#autotoc_md84", null ],
      [ "mixing-point-vocabulary", "md_docs_decisions.html#autotoc_md86", null ],
      [ "cross-model-mixing", "md_docs_decisions.html#autotoc_md88", null ],
      [ "evidence-combine-surface", "md_docs_decisions.html#autotoc_md90", null ],
      [ "graded-context-role", "md_docs_decisions.html#autotoc_md92", null ],
      [ "wording-generation", "md_docs_decisions.html#autotoc_md94", null ],
      [ "wording-visible-internals", "md_docs_decisions.html#autotoc_md96", null ]
    ] ],
    [ "Provenance: extraction of Paper A into beman.transpose", "md_docs_provenance.html", [
      [ "Source", "md_docs_provenance.html#autotoc_md98", null ],
      [ "What was copied and renamed", "md_docs_provenance.html#autotoc_md99", null ],
      [ "Intentional deviations", "md_docs_provenance.html#autotoc_md100", null ],
      [ "Synchronization policy", "md_docs_provenance.html#autotoc_md101", null ],
      [ "2026-07-14: the pure/apply forms removed; evidence labels added", "md_docs_provenance.html#autotoc_md102", null ]
    ] ],
    [ "Stage review — all-of-algorithm", "md_docs_review_execution_all_of_algorithm.html", [
      [ "1. The result", "md_docs_review_execution_all_of_algorithm.html#autotoc_md105", null ],
      [ "2. Design, and the two places the plan was wrong", "md_docs_review_execution_all_of_algorithm.html#autotoc_md107", null ],
      [ "3. What was hard, and what a later stage should expect", "md_docs_review_execution_all_of_algorithm.html#autotoc_md109", null ],
      [ "4. Acceptance", "md_docs_review_execution_all_of_algorithm.html#autotoc_md111", null ],
      [ "5. What Stage 3's agent needs", "md_docs_review_execution_all_of_algorithm.html#autotoc_md113", null ]
    ] ],
    [ "Stage review — execution-baseline", "md_docs_review_execution_baseline.html", [
      [ "1. The thing to read first", "md_docs_review_execution_baseline.html#autotoc_md116", null ],
      [ "2. What was pinned", "md_docs_review_execution_baseline.html#autotoc_md117", null ],
      [ "3. What diverged", "md_docs_review_execution_baseline.html#autotoc_md119", null ],
      [ "4. What the next stage's agent needs that the plan does not say", "md_docs_review_execution_baseline.html#autotoc_md121", null ],
      [ "5. Deliverable-by-deliverable", "md_docs_review_execution_baseline.html#autotoc_md123", null ]
    ] ],
    [ "Stage review — collect-hook", "md_docs_review_execution_collect_hook.html", [
      [ "1. The result", "md_docs_review_execution_collect_hook.html#autotoc_md126", null ],
      [ "2. What the plan got right, and the one thing it got half wrong", "md_docs_review_execution_collect_hook.html#autotoc_md128", null ],
      [ "3. What a later stage should expect", "md_docs_review_execution_collect_hook.html#autotoc_md130", null ],
      [ "4. Acceptance", "md_docs_review_execution_collect_hook.html#autotoc_md132", null ],
      [ "5. What Stage 4's agent needs", "md_docs_review_execution_collect_hook.html#autotoc_md134", null ]
    ] ],
    [ "Stage review — sender-registration", "md_docs_review_execution_sender_registration.html", [
      [ "1. What changed", "md_docs_review_execution_sender_registration.html#autotoc_md137", null ],
      [ "2. Findings", "md_docs_review_execution_sender_registration.html#autotoc_md139", null ],
      [ "3. Acceptance", "md_docs_review_execution_sender_registration.html#autotoc_md141", null ],
      [ "4. What Stage 3's agent needs that the plan does not say", "md_docs_review_execution_sender_registration.html#autotoc_md143", null ]
    ] ],
    [ "execution-stage-prompt", "md_docs_review_execution_stage_prompt.html", null ],
    [ "Stage review — transpose-receipts", "md_docs_review_execution_transpose_receipts.html", [
      [ "1. The result", "md_docs_review_execution_transpose_receipts.html#autotoc_md146", null ],
      [ "2. The divergence, and it is not a small one", "md_docs_review_execution_transpose_receipts.html#autotoc_md148", null ],
      [ "3. The post, and what a build container cannot finish", "md_docs_review_execution_transpose_receipts.html#autotoc_md150", null ],
      [ "4. Acceptance", "md_docs_review_execution_transpose_receipts.html#autotoc_md152", null ],
      [ "5. What Stage 5's agent needs", "md_docs_review_execution_transpose_receipts.html#autotoc_md154", null ]
    ] ],
    [ "Design-Fidelity Review — Grading Branch (interim, pre-delivery)", "md_docs_review_grading_fidelity_review.html", [
      [ "Boot check", "md_docs_review_grading_fidelity_review.html#autotoc_md156", null ],
      [ "Direction 1 — decision → embodiment → sensor", "md_docs_review_grading_fidelity_review.html#autotoc_md157", null ],
      [ "Class A — violations", "md_docs_review_grading_fidelity_review.html#autotoc_md158", null ],
      [ "Class B — undocumented decisions", "md_docs_review_grading_fidelity_review.html#autotoc_md159", null ],
      [ "Class C — sensor gaps", "md_docs_review_grading_fidelity_review.html#autotoc_md160", null ],
      [ "Class D — log drift", "md_docs_review_grading_fidelity_review.html#autotoc_md161", null ],
      [ "Verdict on the documents", "md_docs_review_grading_fidelity_review.html#autotoc_md162", null ]
    ] ],
    [ "Review of the P3200 Reference Implementation", "md_docs_review_p3200_reference_implementation_review.html", [
      [ "Executive summary", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md164", null ],
      [ "Scope and method", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md165", null ],
      [ "Findings", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md166", [
        [ "High: vector traversal violates the linear complexity guarantee", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md167", null ],
        [ "High: sender effects do not obey left-to-right order", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md168", null ],
        [ "High: <tt>transpose</tt> does not implement its stated constraint", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md169", null ],
        [ "Medium: Applicative probes require default-constructible result values", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md170", null ],
        [ "Medium: forwarding-reference front doors do not provide consuming traversal", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md171", null ],
        [ "Medium: callable detection and invocation use different value categories", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md172", null ],
        [ "Medium: array operations impose unnecessary construction requirements", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md173", null ],
        [ "Medium: the short-circuit specification is internally inconsistent", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md174", null ],
        [ "Evidence gap: the sender example is not a P2300 sender", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md175", null ],
        [ "Documentation readiness", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md176", null ]
      ] ],
      [ "Test and tooling assessment", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md177", null ],
      [ "Recommended order of work", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md178", null ],
      [ "Conclusion", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md179", null ]
    ] ],
    [ "Prior art: joining a runtime-sized range of senders", "md_docs_review_prior_art_when_all_range.html", [
      [ "1. The headline: exactly one prior implementation, and it is not in a standard", "md_docs_review_prior_art_when_all_range.html#autotoc_md182", null ],
      [ "2. libunifex <tt>when_all_range</tt> — the one real precedent", "md_docs_review_prior_art_when_all_range.html#autotoc_md184", [
        [ "Surface", "md_docs_review_prior_art_when_all_range.html#autotoc_md185", null ],
        [ "Allocation", "md_docs_review_prior_art_when_all_range.html#autotoc_md186", null ],
        [ "Failure semantics — and where they differ from ours", "md_docs_review_prior_art_when_all_range.html#autotoc_md187", null ]
      ] ],
      [ "3. The standard's variadic <tt>when_all</tt>, which is what we chose to match", "md_docs_review_prior_art_when_all_range.html#autotoc_md189", null ],
      [ "4. Live WG21 work that touches the semantics we are matching", "md_docs_review_prior_art_when_all_range.html#autotoc_md191", null ],
      [ "5. Where prior art contradicts the plan — read before Stage 2", "md_docs_review_prior_art_when_all_range.html#autotoc_md193", null ],
      [ "6. Design-shape comparison (non-sender, recorded for contrast only)", "md_docs_review_prior_art_when_all_range.html#autotoc_md195", null ],
      [ "Sources", "md_docs_review_prior_art_when_all_range.html#autotoc_md197", null ]
    ] ],
    [ "Transpose over real senders — Contextful Evolution Plan", "md_docs_transpose_execution_plan.html", [
      [ "1. Context: what is being claimed and why it is not yet true", "md_docs_transpose_execution_plan.html#context", null ],
      [ "2. Standing decisions the agent must know cold", "md_docs_transpose_execution_plan.html#standing-decisions", null ],
      [ "3. Work plan", "md_docs_transpose_execution_plan.html#work-plan", [
        [ "Stage 0 — @ref execution-baseline \"execution-baseline\"", "md_docs_transpose_execution_plan.html#execution-baseline", null ],
        [ "Stage 1 — @ref sender-registration \"sender-registration\"", "md_docs_transpose_execution_plan.html#sender-registration", null ],
        [ "Stage 2 — @ref all-of-algorithm \"all-of-algorithm\"", "md_docs_transpose_execution_plan.html#all-of-algorithm", null ],
        [ "Stage 3 — @ref collect-hook \"collect-hook\"", "md_docs_transpose_execution_plan.html#collect-hook", null ],
        [ "Stage 4 — @ref transpose-receipts \"transpose-receipts\"", "md_docs_transpose_execution_plan.html#transpose-receipts", null ],
        [ "Stage 5 — @ref paper-note \"paper-note\"", "md_docs_transpose_execution_plan.html#paper-note", null ]
      ] ],
      [ "4. Follow-ons explicitly not in this plan", "md_docs_transpose_execution_plan.html#follow-ons", null ]
    ] ],
    [ "Grading in beman.transpose — Contextful Evolution Plan", "md_docs_transpose_grading_plan.html", [
      [ "0. How to use this document (divergence protocol)", "md_docs_transpose_grading_plan.html#divergence-protocol", [
        [ "Stage 0 — @ref baseline-capture \"baseline-capture\"", "md_docs_transpose_grading_plan.html#baseline-capture", null ],
        [ "Stage 1 — @ref expected-instance \"expected-instance\"", "md_docs_transpose_grading_plan.html#expected-instance", null ],
        [ "Stage 2 — @ref error-set-type \"error-set-type\"", "md_docs_transpose_grading_plan.html#error-set-type", null ],
        [ "Stage 3 — @ref grade-concept \"grade-concept\"", "md_docs_transpose_grading_plan.html#grade-concept", null ],
        [ "Stage 4 — @ref crtp-absorption \"crtp-absorption\"", "md_docs_transpose_grading_plan.html#crtp-absorption", null ],
        [ "Stage 5 — @ref graded-deduction \"graded-deduction\"", "md_docs_transpose_grading_plan.html#graded-deduction", null ],
        [ "Stage 6 — @ref accumulating-object \"accumulating-object\"", "md_docs_transpose_grading_plan.html#accumulating-object", null ],
        [ "Stage 7 — @ref recover-narrowing \"recover-narrowing\"", "md_docs_transpose_grading_plan.html#recover-narrowing", null ],
        [ "Stage 8 — @ref law-harness \"law-harness\"", "md_docs_transpose_grading_plan.html#law-harness", null ],
        [ "Stage 9 — @ref model-dispatched-mixing \"model-dispatched-mixing\"", "md_docs_transpose_grading_plan.html#model-dispatched-mixing", null ],
        [ "Stage 10 — @ref paper-revision \"paper-revision\"", "md_docs_transpose_grading_plan.html#paper-revision", null ]
      ] ],
      [ "4. Guidance — tree_algorithms", "md_docs_transpose_grading_plan.html#tree-algorithms", null ],
      [ "5. Scoping note — fingertree", "md_docs_transpose_grading_plan.html#fingertree", null ],
      [ "6. Open questions", "md_docs_transpose_grading_plan.html#open-questions", null ]
    ] ],
    [ "typeclass-object-pattern", "md_docs_typeclass_object_pattern.html", [
      [ "Typeclass Object Pattern in This Repository", "md_docs_typeclass_object_pattern.html#autotoc_md200", [
        [ "Why this exists", "md_docs_typeclass_object_pattern.html#autotoc_md201", null ],
        [ "The surface in this repo", "md_docs_typeclass_object_pattern.html#autotoc_md202", null ],
        [ "Lookup modes (important)", "md_docs_typeclass_object_pattern.html#autotoc_md203", null ],
        [ "Core mechanics", "md_docs_typeclass_object_pattern.html#autotoc_md204", [
          [ "Concept side", "md_docs_typeclass_object_pattern.html#autotoc_md205", null ],
          [ "Type side", "md_docs_typeclass_object_pattern.html#autotoc_md206", null ],
          [ "Call side", "md_docs_typeclass_object_pattern.html#autotoc_md207", null ]
        ] ],
        [ "How to add a new instance", "md_docs_typeclass_object_pattern.html#autotoc_md208", null ],
        [ "How to add a new concept", "md_docs_typeclass_object_pattern.html#autotoc_md209", null ],
        [ "Testing and build wiring expectations", "md_docs_typeclass_object_pattern.html#autotoc_md210", null ],
        [ "Algorithm objects: Inheriting from typeclass instances", "md_docs_typeclass_object_pattern.html#autotoc_md211", [
          [ "Pattern", "md_docs_typeclass_object_pattern.html#autotoc_md212", null ],
          [ "Multi-typeclass composition", "md_docs_typeclass_object_pattern.html#autotoc_md213", null ],
          [ "Key points", "md_docs_typeclass_object_pattern.html#autotoc_md214", null ]
        ] ],
        [ "Applicative: Derived invoke via terminating partial application", "md_docs_typeclass_object_pattern.html#autotoc_md215", null ],
        [ "Traps and corrections from tree-instance implementation", "md_docs_typeclass_object_pattern.html#autotoc_md216", null ],
        [ "Notes for future cleanup", "md_docs_typeclass_object_pattern.html#autotoc_md217", null ]
      ] ]
    ] ],
    [ "The wording pipeline", "md_docs_wording_pipeline.html", [
      [ "The loop", "md_docs_wording_pipeline.html#autotoc_md219", null ],
      [ "What generates what", "md_docs_wording_pipeline.html#autotoc_md220", null ],
      [ "Rules for marking up a header", "md_docs_wording_pipeline.html#autotoc_md221", null ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"index.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';