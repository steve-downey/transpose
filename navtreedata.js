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
    [ "CODING_RULES", "md_docs_CODING_RULES.html", [
      [ "Coding Rules", "md_docs_CODING_RULES.html#autotoc_md0", [
        [ "Semantic Defaults", "md_docs_CODING_RULES.html#autotoc_md1", null ],
        [ "Project Layout", "md_docs_CODING_RULES.html#autotoc_md2", null ],
        [ "File Prolog and Includes", "md_docs_CODING_RULES.html#autotoc_md3", null ],
        [ "CMake and Build Graph", "md_docs_CODING_RULES.html#autotoc_md4", null ],
        [ "C++ Structure", "md_docs_CODING_RULES.html#autotoc_md5", null ],
        [ "Language and Tooling", "md_docs_CODING_RULES.html#autotoc_md6", null ],
        [ "Typeclass Design", "md_docs_CODING_RULES.html#autotoc_md7", null ],
        [ "Foldable Rules", "md_docs_CODING_RULES.html#autotoc_md8", null ],
        [ "Applicative Rules", "md_docs_CODING_RULES.html#autotoc_md9", null ],
        [ "Traversable Rules", "md_docs_CODING_RULES.html#autotoc_md10", null ],
        [ "Test Rules", "md_docs_CODING_RULES.html#autotoc_md11", null ],
        [ "Slide and Transclusion Rules", "md_docs_CODING_RULES.html#autotoc_md12", null ],
        [ "Prose and Documentation Formatting", "md_docs_CODING_RULES.html#autotoc_md13", null ]
      ] ]
    ] ],
    [ "Coordination worklist — 2026-07-13", "md_docs_coordination_worklist_2026_07_13.html", [
      [ "1. Add a \"Why not Monad\" section to D3200R0", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md15", null ],
      [ "2. Label unproposed repo surface as evidence", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md16", null ],
      [ "3. Keep Traversable free of a Foldable superclass requirement", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md17", null ],
      [ "4. Relationships essay hook", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md18", null ],
      [ "5. Remove the pure/apply forms; <tt>invoke</tt> is the applicative core", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md19", null ],
      [ "6. BinaryTree example cross-check (standing item)", "md_docs_coordination_worklist_2026_07_13.html#autotoc_md20", null ]
    ] ],
    [ "Decision Log — grading in beman.transpose", "md_docs_decisions.html", [
      [ "empty-grade-spelling", "md_docs_decisions.html#autotoc_md23", null ],
      [ "grading-footprint", "md_docs_decisions.html#autotoc_md25", null ],
      [ "error-set-identity", "md_docs_decisions.html#autotoc_md27", null ],
      [ "grade-machinery-home", "md_docs_decisions.html#autotoc_md29", null ],
      [ "applicative-objects", "md_docs_decisions.html#autotoc_md31", null ],
      [ "recover-grade-inference", "md_docs_decisions.html#autotoc_md33", null ],
      [ "grade-generality", "md_docs_decisions.html#autotoc_md35", null ],
      [ "uniform-form-surface", "md_docs_decisions.html#autotoc_md37", null ],
      [ "datum-entry-point", "md_docs_decisions.html#autotoc_md39", null ],
      [ "traverse-policy-surface", "md_docs_decisions.html#autotoc_md41", null ],
      [ "optional-grade-model", "md_docs_decisions.html#autotoc_md43", null ]
    ] ],
    [ "Provenance: extraction of Paper A into beman.transpose", "md_docs_provenance.html", [
      [ "Source", "md_docs_provenance.html#autotoc_md45", null ],
      [ "What was copied and renamed", "md_docs_provenance.html#autotoc_md46", null ],
      [ "Intentional deviations", "md_docs_provenance.html#autotoc_md47", null ],
      [ "Synchronization policy", "md_docs_provenance.html#autotoc_md48", null ],
      [ "2026-07-14: the pure/apply forms removed; evidence labels added", "md_docs_provenance.html#autotoc_md49", null ]
    ] ],
    [ "Grading in beman.transpose — Contextful Evolution Plan", "md_docs_transpose_grading_plan.html", [
      [ "0. How to use this document (divergence protocol)", "md_docs_transpose_grading_plan.html#divergence-protocol", null ],
      [ "1. Context: what this library is and what grading is doing in it", "md_docs_transpose_grading_plan.html#context", null ],
      [ "2. Standing decisions", "md_docs_transpose_grading_plan.html#standing-decisions", null ],
      [ "3. Work plan — beman.transpose", "md_docs_transpose_grading_plan.html#work-plan", [
        [ "Stage 0 — @ref baseline-capture \"baseline-capture\"", "md_docs_transpose_grading_plan.html#baseline-capture", null ],
        [ "Stage 1 — @ref error-set-type \"error-set-type\"", "md_docs_transpose_grading_plan.html#error-set-type", null ],
        [ "Stage 2 — @ref grade-concept \"grade-concept\"", "md_docs_transpose_grading_plan.html#grade-concept", null ],
        [ "Stage 3 — @ref crtp-absorption \"crtp-absorption\"", "md_docs_transpose_grading_plan.html#crtp-absorption", null ],
        [ "Stage 4 — @ref graded-deduction \"graded-deduction\"", "md_docs_transpose_grading_plan.html#graded-deduction", null ],
        [ "Stage 5 — @ref accumulating-object \"accumulating-object\"", "md_docs_transpose_grading_plan.html#accumulating-object", null ],
        [ "Stage 6 — @ref recover-narrowing \"recover-narrowing\"", "md_docs_transpose_grading_plan.html#recover-narrowing", null ],
        [ "Stage 7 — @ref law-harness \"law-harness\"", "md_docs_transpose_grading_plan.html#law-harness", null ],
        [ "Stage 8 — @ref paper-revision \"paper-revision\"", "md_docs_transpose_grading_plan.html#paper-revision", null ]
      ] ],
      [ "4. Guidance — tree_algorithms", "md_docs_transpose_grading_plan.html#tree-algorithms", null ],
      [ "5. Scoping note — fingertree", "md_docs_transpose_grading_plan.html#fingertree", null ],
      [ "6. Open questions", "md_docs_transpose_grading_plan.html#open-questions", null ]
    ] ],
    [ "typeclass-object-pattern", "md_docs_typeclass_object_pattern.html", [
      [ "Typeclass Object Pattern in This Repository", "md_docs_typeclass_object_pattern.html#autotoc_md51", [
        [ "Why this exists", "md_docs_typeclass_object_pattern.html#autotoc_md52", null ],
        [ "The surface in this repo", "md_docs_typeclass_object_pattern.html#autotoc_md53", null ],
        [ "Lookup modes (important)", "md_docs_typeclass_object_pattern.html#autotoc_md54", null ],
        [ "Core mechanics", "md_docs_typeclass_object_pattern.html#autotoc_md55", [
          [ "Concept side", "md_docs_typeclass_object_pattern.html#autotoc_md56", null ],
          [ "Type side", "md_docs_typeclass_object_pattern.html#autotoc_md57", null ],
          [ "Call side", "md_docs_typeclass_object_pattern.html#autotoc_md58", null ]
        ] ],
        [ "How to add a new instance", "md_docs_typeclass_object_pattern.html#autotoc_md59", null ],
        [ "How to add a new concept", "md_docs_typeclass_object_pattern.html#autotoc_md60", null ],
        [ "Testing and build wiring expectations", "md_docs_typeclass_object_pattern.html#autotoc_md61", null ],
        [ "Algorithm objects: Inheriting from typeclass instances", "md_docs_typeclass_object_pattern.html#autotoc_md62", [
          [ "Pattern", "md_docs_typeclass_object_pattern.html#autotoc_md63", null ],
          [ "Multi-typeclass composition", "md_docs_typeclass_object_pattern.html#autotoc_md64", null ],
          [ "Key points", "md_docs_typeclass_object_pattern.html#autotoc_md65", null ]
        ] ],
        [ "Applicative: Derived invoke via terminating partial application", "md_docs_typeclass_object_pattern.html#autotoc_md66", null ],
        [ "Traps and corrections from tree-instance implementation", "md_docs_typeclass_object_pattern.html#autotoc_md67", null ],
        [ "Notes for future cleanup", "md_docs_typeclass_object_pattern.html#autotoc_md68", null ]
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