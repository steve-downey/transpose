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
    [ "Decision Log — grading in beman.transpose", "md_docs_decisions.html", [
      [ "empty-grade-spelling", "md_docs_decisions.html#autotoc_md30", null ],
      [ "grading-footprint", "md_docs_decisions.html#autotoc_md32", null ],
      [ "error-set-identity", "md_docs_decisions.html#autotoc_md34", null ],
      [ "grade-machinery-home", "md_docs_decisions.html#autotoc_md36", null ],
      [ "applicative-objects", "md_docs_decisions.html#autotoc_md38", null ],
      [ "recover-grade-inference", "md_docs_decisions.html#autotoc_md40", null ],
      [ "grade-generality", "md_docs_decisions.html#autotoc_md42", null ],
      [ "uniform-form-surface", "md_docs_decisions.html#autotoc_md44", null ],
      [ "datum-entry-point", "md_docs_decisions.html#autotoc_md46", null ],
      [ "traverse-policy-surface", "md_docs_decisions.html#autotoc_md48", null ],
      [ "optional-grade-model", "md_docs_decisions.html#autotoc_md50", null ],
      [ "plain-error-grade-reading", "md_docs_decisions.html#autotoc_md52", null ],
      [ "grade-model-identity", "md_docs_decisions.html#autotoc_md54", null ],
      [ "expected-instance-introduction", "md_docs_decisions.html#autotoc_md56", null ],
      [ "grade-operation-spelling", "md_docs_decisions.html#autotoc_md58", null ],
      [ "accumulation-evidence", "md_docs_decisions.html#autotoc_md60", null ],
      [ "multi-witness-elimination", "md_docs_decisions.html#autotoc_md62", null ],
      [ "golden-vs-scheduled-assertions", "md_docs_decisions.html#autotoc_md64", null ],
      [ "bottom-grade-identity", "md_docs_decisions.html#autotoc_md66", null ],
      [ "mixing-point-vocabulary", "md_docs_decisions.html#autotoc_md68", null ],
      [ "cross-model-mixing", "md_docs_decisions.html#autotoc_md70", null ],
      [ "evidence-combine-surface", "md_docs_decisions.html#autotoc_md72", null ],
      [ "graded-context-role", "md_docs_decisions.html#autotoc_md74", null ]
    ] ],
    [ "Provenance: extraction of Paper A into beman.transpose", "md_docs_provenance.html", [
      [ "Source", "md_docs_provenance.html#autotoc_md76", null ],
      [ "What was copied and renamed", "md_docs_provenance.html#autotoc_md77", null ],
      [ "Intentional deviations", "md_docs_provenance.html#autotoc_md78", null ],
      [ "Synchronization policy", "md_docs_provenance.html#autotoc_md79", null ],
      [ "2026-07-14: the pure/apply forms removed; evidence labels added", "md_docs_provenance.html#autotoc_md80", null ]
    ] ],
    [ "Design-Fidelity Review — Grading Branch (interim, pre-delivery)", "md_docs_review_grading_fidelity_review.html", [
      [ "Boot check", "md_docs_review_grading_fidelity_review.html#autotoc_md82", null ],
      [ "Direction 1 — decision → embodiment → sensor", "md_docs_review_grading_fidelity_review.html#autotoc_md83", null ],
      [ "Class A — violations", "md_docs_review_grading_fidelity_review.html#autotoc_md84", null ],
      [ "Class B — undocumented decisions", "md_docs_review_grading_fidelity_review.html#autotoc_md85", null ],
      [ "Class C — sensor gaps", "md_docs_review_grading_fidelity_review.html#autotoc_md86", null ],
      [ "Class D — log drift", "md_docs_review_grading_fidelity_review.html#autotoc_md87", null ],
      [ "Verdict on the documents", "md_docs_review_grading_fidelity_review.html#autotoc_md88", null ]
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
      [ "Typeclass Object Pattern in This Repository", "md_docs_typeclass_object_pattern.html#autotoc_md90", [
        [ "Why this exists", "md_docs_typeclass_object_pattern.html#autotoc_md91", null ],
        [ "The surface in this repo", "md_docs_typeclass_object_pattern.html#autotoc_md92", null ],
        [ "Lookup modes (important)", "md_docs_typeclass_object_pattern.html#autotoc_md93", null ],
        [ "Core mechanics", "md_docs_typeclass_object_pattern.html#autotoc_md94", [
          [ "Concept side", "md_docs_typeclass_object_pattern.html#autotoc_md95", null ],
          [ "Type side", "md_docs_typeclass_object_pattern.html#autotoc_md96", null ],
          [ "Call side", "md_docs_typeclass_object_pattern.html#autotoc_md97", null ]
        ] ],
        [ "How to add a new instance", "md_docs_typeclass_object_pattern.html#autotoc_md98", null ],
        [ "How to add a new concept", "md_docs_typeclass_object_pattern.html#autotoc_md99", null ],
        [ "Testing and build wiring expectations", "md_docs_typeclass_object_pattern.html#autotoc_md100", null ],
        [ "Algorithm objects: Inheriting from typeclass instances", "md_docs_typeclass_object_pattern.html#autotoc_md101", [
          [ "Pattern", "md_docs_typeclass_object_pattern.html#autotoc_md102", null ],
          [ "Multi-typeclass composition", "md_docs_typeclass_object_pattern.html#autotoc_md103", null ],
          [ "Key points", "md_docs_typeclass_object_pattern.html#autotoc_md104", null ]
        ] ],
        [ "Applicative: Derived invoke via terminating partial application", "md_docs_typeclass_object_pattern.html#autotoc_md105", null ],
        [ "Traps and corrections from tree-instance implementation", "md_docs_typeclass_object_pattern.html#autotoc_md106", null ],
        [ "Notes for future cleanup", "md_docs_typeclass_object_pattern.html#autotoc_md107", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"index.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';