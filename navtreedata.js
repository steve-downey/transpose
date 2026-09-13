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
    [ "Decision Log — grading in beman.transpose", "md_docs_decisions.html", [
      [ "empty-grade-spelling", "md_docs_decisions.html#autotoc_md35", null ],
      [ "grading-footprint", "md_docs_decisions.html#autotoc_md37", null ],
      [ "error-set-identity", "md_docs_decisions.html#autotoc_md39", null ],
      [ "grade-machinery-home", "md_docs_decisions.html#autotoc_md41", null ],
      [ "applicative-objects", "md_docs_decisions.html#autotoc_md43", null ],
      [ "recover-grade-inference", "md_docs_decisions.html#autotoc_md45", null ],
      [ "grade-generality", "md_docs_decisions.html#autotoc_md47", null ],
      [ "uniform-form-surface", "md_docs_decisions.html#autotoc_md49", null ],
      [ "datum-entry-point", "md_docs_decisions.html#autotoc_md51", null ],
      [ "traverse-policy-surface", "md_docs_decisions.html#autotoc_md53", null ],
      [ "optional-grade-model", "md_docs_decisions.html#autotoc_md55", null ],
      [ "plain-error-grade-reading", "md_docs_decisions.html#autotoc_md57", null ],
      [ "grade-model-identity", "md_docs_decisions.html#autotoc_md59", null ],
      [ "expected-instance-introduction", "md_docs_decisions.html#autotoc_md61", null ],
      [ "grade-operation-spelling", "md_docs_decisions.html#autotoc_md63", null ],
      [ "accumulation-evidence", "md_docs_decisions.html#autotoc_md65", null ],
      [ "multi-witness-elimination", "md_docs_decisions.html#autotoc_md67", null ],
      [ "golden-vs-scheduled-assertions", "md_docs_decisions.html#autotoc_md69", null ],
      [ "bottom-grade-identity", "md_docs_decisions.html#autotoc_md71", null ],
      [ "mixing-point-vocabulary", "md_docs_decisions.html#autotoc_md73", null ],
      [ "cross-model-mixing", "md_docs_decisions.html#autotoc_md75", null ],
      [ "evidence-combine-surface", "md_docs_decisions.html#autotoc_md77", null ],
      [ "graded-context-role", "md_docs_decisions.html#autotoc_md79", null ],
      [ "wording-generation", "md_docs_decisions.html#autotoc_md81", null ],
      [ "wording-visible-internals", "md_docs_decisions.html#autotoc_md83", null ]
    ] ],
    [ "Provenance: extraction of Paper A into beman.transpose", "md_docs_provenance.html", [
      [ "Source", "md_docs_provenance.html#autotoc_md85", null ],
      [ "What was copied and renamed", "md_docs_provenance.html#autotoc_md86", null ],
      [ "Intentional deviations", "md_docs_provenance.html#autotoc_md87", null ],
      [ "Synchronization policy", "md_docs_provenance.html#autotoc_md88", null ],
      [ "2026-07-14: the pure/apply forms removed; evidence labels added", "md_docs_provenance.html#autotoc_md89", null ]
    ] ],
    [ "Design-Fidelity Review — Grading Branch (interim, pre-delivery)", "md_docs_review_grading_fidelity_review.html", [
      [ "Boot check", "md_docs_review_grading_fidelity_review.html#autotoc_md91", null ],
      [ "Direction 1 — decision → embodiment → sensor", "md_docs_review_grading_fidelity_review.html#autotoc_md92", null ],
      [ "Class A — violations", "md_docs_review_grading_fidelity_review.html#autotoc_md93", null ],
      [ "Class B — undocumented decisions", "md_docs_review_grading_fidelity_review.html#autotoc_md94", null ],
      [ "Class C — sensor gaps", "md_docs_review_grading_fidelity_review.html#autotoc_md95", null ],
      [ "Class D — log drift", "md_docs_review_grading_fidelity_review.html#autotoc_md96", null ],
      [ "Verdict on the documents", "md_docs_review_grading_fidelity_review.html#autotoc_md97", null ]
    ] ],
    [ "Review of the P3200 Reference Implementation", "md_docs_review_p3200_reference_implementation_review.html", [
      [ "Executive summary", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md99", null ],
      [ "Scope and method", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md100", null ],
      [ "Findings", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md101", [
        [ "High: vector traversal violates the linear complexity guarantee", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md102", null ],
        [ "High: sender effects do not obey left-to-right order", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md103", null ],
        [ "High: <tt>transpose</tt> does not implement its stated constraint", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md104", null ],
        [ "Medium: Applicative probes require default-constructible result values", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md105", null ],
        [ "Medium: forwarding-reference front doors do not provide consuming traversal", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md106", null ],
        [ "Medium: callable detection and invocation use different value categories", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md107", null ],
        [ "Medium: array operations impose unnecessary construction requirements", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md108", null ],
        [ "Medium: the short-circuit specification is internally inconsistent", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md109", null ],
        [ "Evidence gap: the sender example is not a P2300 sender", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md110", null ],
        [ "Documentation readiness", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md111", null ]
      ] ],
      [ "Test and tooling assessment", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md112", null ],
      [ "Recommended order of work", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md113", null ],
      [ "Conclusion", "md_docs_review_p3200_reference_implementation_review.html#autotoc_md114", null ]
    ] ],
    [ "Grading in beman.transpose — Contextful Evolution Plan", "md_docs_transpose_grading_plan.html", [
      [ "0. How to use this document (divergence protocol)", "md_docs_transpose_grading_plan.html#divergence-protocol", null ],
      [ "1. Context: what this library is and what grading is doing in it", "md_docs_transpose_grading_plan.html#context", null ],
      [ "2. Standing decisions", "md_docs_transpose_grading_plan.html#standing-decisions", null ],
      [ "3. Work plan — beman.transpose", "md_docs_transpose_grading_plan.html#work-plan", [
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
      [ "Typeclass Object Pattern in This Repository", "md_docs_typeclass_object_pattern.html#autotoc_md116", [
        [ "Why this exists", "md_docs_typeclass_object_pattern.html#autotoc_md117", null ],
        [ "The surface in this repo", "md_docs_typeclass_object_pattern.html#autotoc_md118", null ],
        [ "Lookup modes (important)", "md_docs_typeclass_object_pattern.html#autotoc_md119", null ],
        [ "Core mechanics", "md_docs_typeclass_object_pattern.html#autotoc_md120", [
          [ "Concept side", "md_docs_typeclass_object_pattern.html#autotoc_md121", null ],
          [ "Type side", "md_docs_typeclass_object_pattern.html#autotoc_md122", null ],
          [ "Call side", "md_docs_typeclass_object_pattern.html#autotoc_md123", null ]
        ] ],
        [ "How to add a new instance", "md_docs_typeclass_object_pattern.html#autotoc_md124", null ],
        [ "How to add a new concept", "md_docs_typeclass_object_pattern.html#autotoc_md125", null ],
        [ "Testing and build wiring expectations", "md_docs_typeclass_object_pattern.html#autotoc_md126", null ],
        [ "Algorithm objects: Inheriting from typeclass instances", "md_docs_typeclass_object_pattern.html#autotoc_md127", [
          [ "Pattern", "md_docs_typeclass_object_pattern.html#autotoc_md128", null ],
          [ "Multi-typeclass composition", "md_docs_typeclass_object_pattern.html#autotoc_md129", null ],
          [ "Key points", "md_docs_typeclass_object_pattern.html#autotoc_md130", null ]
        ] ],
        [ "Applicative: Derived invoke via terminating partial application", "md_docs_typeclass_object_pattern.html#autotoc_md131", null ],
        [ "Traps and corrections from tree-instance implementation", "md_docs_typeclass_object_pattern.html#autotoc_md132", null ],
        [ "Notes for future cleanup", "md_docs_typeclass_object_pattern.html#autotoc_md133", null ]
      ] ]
    ] ],
    [ "The wording pipeline", "md_docs_wording_pipeline.html", [
      [ "The loop", "md_docs_wording_pipeline.html#autotoc_md135", null ],
      [ "What generates what", "md_docs_wording_pipeline.html#autotoc_md136", null ],
      [ "Rules for marking up a header", "md_docs_wording_pipeline.html#autotoc_md137", null ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"index.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';