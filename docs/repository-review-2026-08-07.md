# Repository Design, Code, and Docs Review - 2026-08-07

## Scope

This review covers the repository design, public headers, tests, build
workflow, and every pre-existing file under `docs/`; this file is the generated
review output. It treats
`make TOOLCHAIN=gcc-16 test` as the primary matrix target, per project
priority, while also recording the GCC 13 production-compiler constraint as a
real adoption concern.

The main conclusion: the current implementation is coherent on the GCC 16 path,
but the documentation and doc-generation configuration are materially stale.
The highest-risk issue is not the core code under GCC 16; it is that the
default `make test` path is known-bad on GCC 13, while several docs still claim
an older Applicative design that the code has intentionally moved past.

## Verification Summary

| Check | Result | Notes |
| --- | --- | --- |
| `make TOOLCHAIN=gcc-16 test` | Passed | 56/56 tests under Asan with GCC 16.1.1. |
| `make test` | Failed | Local default `c++` is GCC 13.3; parse errors begin at `include/beman/transpose/apply.hpp:164` on `this auto &&`. |
| C++26 SIMD smoke compile | Passed | Direct `g++-16 -std=gnu++26 -Iinclude` compile and run against `include/beman/transpose/simd.hpp`. |
| `node --check docs/doxygen-awesome-darkmode-toggle.js` | Failed | Syntax error at line 35. |
| `mrdocs` availability | Failed | `mrdocs` is not on PATH locally. |
| Authored external URLs under `docs/` | Mostly passed | All checked authored URLs returned 200 except the MrDocs schema URL, which returned 404. |

Important nuance: `make TOOLCHAIN=gcc-16 test` configures C++23
(`CMakePresets.json:10`, `CMakeLists.txt:38`). The `simd` test executable is
present, but `tests/beman/transpose/simd.test.cpp:9` gates real tests on
`__cplusplus > 202302L`, so the normal GCC 16 test target does not exercise the
real `std::simd` code path.

## Prioritized Actions

### P0. Make the default test path honest about compiler support

Evidence:

- `Makefile:87` makes `test` the default target.
- `etc/toolchain.cmake:5-6` uses plain `cc` and `c++`.
- On this host, `c++ --version` is GCC 13.3.0.
- Public headers use explicit object parameters throughout, for example
  `include/beman/transpose/apply.hpp:164`,
  `include/beman/transpose/fold.hpp:230`,
  `include/beman/transpose/traverse.hpp:58`, and many more.
- `make test` fails immediately on GCC 13 with `expected identifier before
  'this'`.

Action:

- If GCC 16 is the supported mainline compiler, make the default workflow select
  a supported compiler when available, or fail early with a clear diagnostic
  before CMake/Ninja emits thousands of parser errors.
- If GCC 13 support is required for real-world use, add an explicit
  compatibility decision: either a GCC 13 branch/lane that rewrites deducing-
  `this` members into conventional member templates, or a documented
  "unsupported on GCC 13" policy for mainline. Do not let GCC 13 silently define
  the default behavior while GCC 16 remains the design target.

### P0. Repair docs generation configuration before publishing generated docs

Evidence:

- `docs/mrdocs.yml:9-21` is copied from `beman.optional`: it excludes
  `../include/beman/optional/detail`, includes `beman::optional::**`, and marks
  `beman::optional::detail` implementation-defined.
- The MrDocs schema URL in `docs/mrdocs.yml:1-2` returns HTTP 404.
- `Makefile:272-276` has a MrDocs target, but `mrdocs` is not installed locally.
- `docs/Doxyfile:59` says `PROJECT_BRIEF = An Example of Everything`.
- `docs/Doxyfile:66` and `docs/Doxyfile:72` reference `docs/beman-logo.png`,
  which is not present.
- `docs/Doxyfile:996` uses `INPUT = src docs`; this repository has no `src/`
  directory, and the public headers are under `include/`.
- `docs/Doxyfile:1054` excludes `docs/debug-ci.md`, which is not present.
- `docs/doxygen-awesome-darkmode-toggle.js` is syntactically invalid.

Action:

- Update MrDocs to target `beman::transpose::**`, the actual `detail` namespace,
  and the actual include tree.
- Replace or remove the stale MrDocs schema URL.
- Update Doxygen input to include the real public headers and intended docs
  sources, remove missing asset references or add the assets, and give the
  project a real brief.
- Replace the broken dark-mode JS with a clean upstream copy or remove it from
  the generated-doc assets.

### P0. Correct the Applicative narrative across docs

Current code truth:

- `include/beman/transpose/apply.hpp:111-142` states the design: an instance can
  provide `pure + invoke` or `pure + ap`; `invoke` is the user-facing interface,
  and `ap` remains a secondary operation.
- `include/beman/transpose/apply.hpp:24-75` still contains
  `detail::terminating_partial`.
- `include/beman/transpose/apply.hpp:99-107` still contains `detail::ap`.
- `include/beman/transpose/apply.hpp:208-236` exposes `Applicative::ap`.
- `tests/beman/transpose/apply.test.cpp:25-47` explicitly tests `ap`.

Stale docs:

- `docs/provenance.md:77-87` says `apply`, `ap`, and
  `terminating_partial` were removed and that tests pin their absence.
- `docs/coordination-worklist-2026-07-13.md:89-123` records the same removal as
  executed work.
- `docs/typeclass-object-pattern.md:193-214` teaches `pure + apply` as the
  primitive.
- `docs/typeclass-object-pattern.md:232-235` says the Applicative contract is
  `pure` plus `apply`.
- `docs/CODING_RULES.md:92-96` says implementor-facing primitives may be
  `pure` and `apply`.
- `docs/typeclass-object-pattern-in-cpp.org:495-500` lists Applicative
  primitives as `pure + apply`.
- `docs/box-traits-and-typeclass-objects.org:568-571` says Applicative is
  `pure + apply`.

Action:

- Make `invoke` the lead term everywhere.
- Replace "removed" claims with "reintroduced as secondary operation" or mark
  the old worklist/provenance sections as superseded by the current code.
- Use one spelling consistently: `ap` for one-step contextual application, not
  `apply`, unless the doc is explicitly talking about the historical name.

### P1. Add real C++26 SIMD test coverage

Evidence:

- `include/beman/transpose/simd.hpp:19` self-gates on `<simd>` and
  `__cplusplus > 202302L`.
- `tests/beman/transpose/simd.test.cpp:9` uses the same gate.
- `CMakePresets.json:10` sets `CMAKE_CXX_STANDARD` to 23.
- The normal GCC 16 test run reports the fallback case:
  `simd: std::simd (P1928, C++26) unavailable in this configuration`.
- A direct GCC 16 C++26 smoke compile and run passed.

Action:

- Add a C++26 configure/test target or CI job that exercises
  `tests/beman/transpose/simd.test.cpp` on GCC 16.
- Keep the existing C++23 lane, but do not count it as coverage for the real
  `std::simd` applicative.

### P1. Fix stale fold-right evidence in the typeclass essay

Evidence:

- `docs/typeclass-object-pattern-in-cpp.org:324-360` claims that
  `tests/beman/transpose/fold.test.cpp` contains `RightFoldSequenceImpl`,
  `RightFoldSequenceMap`, and comparison tests for a `fold_right` primitive.
- `tests/beman/transpose/fold.test.cpp` has no such alternate-core test.
- `include/beman/transpose/fold.hpp:208-220` says the design supports
  `fold_map` or `fold_right + element_type`.

Action:

- Either restore the missing fold-right primitive test, or rewrite the essay to
  say the snippet is illustrative and not current test evidence.

### P1. Repair citation machinery and formatting in `docs/codestyle.org`

Evidence:

- `docs/codestyle.org` contains many opaque citation artifacts such as
  `turn9search0` and `turn12search10`; these are not auditable sources in the
  repository.
- The BibTeX URL entries at `docs/codestyle.org:820-921` all returned HTTP 200.
- `docs/codestyle.org:47-96` flattens tables into prose, making the precedence
  and filename mapping hard to audit.
- `docs/codestyle.org:556-563` runs "CMake House Rules" into the preceding
  paragraph instead of opening a heading.

Action:

- Replace opaque generated citation artifacts with stable citation keys or Org
  citations backed by a checked bibliography.
- Rebuild the flattened tables as actual Org tables.
- Restore missing headings so the document exports coherently.

### P1. Fix Org bibliography configuration

Evidence:

- `docs/typeclass-object-pattern-in-cpp.org:30` declares
  `#+BIBLIOGRAPHY: ./references.bib`.
- `docs/references.bib` does not exist.
- `docs/typeclass-object-pattern-in-cpp.org:531` calls `#+print_bibliography:`.

Action:

- Add the bibliography file, remove the bibliography directives, or migrate the
  live links in the document into a real citation file.

### P1. Clarify `simd_lanes` availability

Evidence:

- `docs/typeclass-object-a-new-extension-point.org:259-264` and
  `docs/typeclass-object-pattern-in-cpp.org:511-516` say
  `simd_lanes<T, N>` is conditional on `<simd>`.
- `include/beman/transpose/simd_lanes.hpp:18-24` includes only local headers and
  standard C++23 headers; it is standalone.
- `include/beman/transpose/transpose.hpp:30-33` includes `simd_lanes.hpp` from
  the umbrella header only when `<simd>` is present.

Action:

- State the nuance: the standalone `simd_lanes.hpp` header is unconditional;
  umbrella exposure through `transpose.hpp` is currently conditional because it
  is grouped with `simd.hpp`.

### P1. Fix the README first-page formatting bug

Evidence:

- `README.md:25-33` inserts a `Build` section before finishing the problem
  statement.
- The shape equation `structure<context<T>> -> context<structure<T>>` appears
  inside the shell code fence opened for `cmake --workflow --preset gcc-release`.

Action:

- Move the Build section down to the existing `## Building` area.
- Put the shape equation in a text or C++-neutral code fence in the problem
  section.

### P2. Remove or test unused fold-program machinery

Evidence:

- `include/beman/transpose/fold.hpp:44-83` defines `LeftFoldProgram`,
  `LeftFoldProgramT`, `RightFoldProgram`, and `RightFoldProgramT`.
- `include/beman/transpose/fold.hpp:116-164` specializes monoids for the
  template variants, but their `identity()` returns an
  `IdentityFoldFunc<int>`-based type regardless of the state type.
- Repository search found no uses outside the definitions.

Action:

- Remove the unused template fold-program path, or add focused tests that prove
  the generic identity/composition behavior for non-`int` state.

### P2. Avoid default-construction constraints in fixed-width Applicatives

Evidence:

- `include/beman/transpose/array.hpp:48-51` default-constructs
  `std::array<U, N>` and assigns each element.
- `include/beman/transpose/simd_lanes.hpp:61-65` default-constructs
  `simd_lanes<U, N>` and assigns each lane.

Action:

- If these contexts should support non-default-constructible result types, build
  results with index-sequence aggregate construction instead of default
  construction plus assignment.

### P2. Document or relax `sender<T>` value/callable constraints

Evidence:

- `include/beman/transpose/sender.hpp:29` stores work in `std::function<T()>`.
- `include/beman/transpose/sender.hpp:35-37` captures a ready value by move but
  returns it by copy from a const lambda body.
- `include/beman/transpose/sender.hpp:73-77` captures callables and operands in
  a copyable closure for `std::function`.

Action:

- If `sender` remains only a non-normative demonstration type, document the
  copyability limitation.
- If it is expected to model realistic deferred work, use a move-capable storage
  strategy and add move-only payload tests.

## Doc-by-Doc Fact Check

| File | Result |
| --- | --- |
| `docs/CODING_RULES.md` | Mostly matches repo layout, but Applicative rule at lines 92-96 is stale. |
| `docs/Doxyfile` | Active configuration is stale: wrong brief, missing logo, wrong input tree, missing excluded file. |
| `docs/box-traits-and-typeclass-objects.org` | External links checked OK; Applicative line 570 is stale. |
| `docs/bringing-typeclass-operations-into-scope.org` | Internal file links exist; no blocking fact drift found in this pass. |
| `docs/codestyle.org` | Bibliography URLs checked OK, but opaque citation artifacts and malformed tables/headings need cleanup. |
| `docs/coordination-worklist-2026-07-13.md` | Historically useful, but item 5 conflicts with current code and should be marked superseded or updated. |
| `docs/doxygen-awesome-darkmode-toggle.js` | Invalid JavaScript; `node --check` fails at line 35. |
| `docs/doxygen-awesome.css` | Vendored-style asset; source URL checked OK. No functional CSS audit performed. |
| `docs/mrdocs.yml` | Stale from optional; schema URL returns 404; config points at the wrong namespace. |
| `docs/provenance.md` | Extraction history is useful, but lines 77-87 contradict current `ap` support. |
| `docs/typeclass-object-a-new-extension-point.org` | Internal examples are broadly consistent; `simd_lanes` availability claim is wrong. |
| `docs/typeclass-object-pattern-in-cpp.org` | Multiple stale claims: missing bibliography file, stale fold-right test evidence, stale Applicative primitive row, wrong `simd_lanes` condition. |
| `docs/typeclass-object-pattern.md` | Good high-level local guide, but Applicative section still teaches the old `pure + apply` story. |

## Citation Audit

- Authored external links outside `docs/Doxyfile` were checked with `curl -L -I`.
  All returned HTTP 200 except
  `https://mrdocs.com/docs/mrdocs/develop/_attachments/mrdocs.schema.json`,
  which returned 404.
- Normal redirects were observed and are not problems: cppreference rewrites
  `/w/cpp/...` to `/cpp/...`, `wg21.link/P1255` resolves to the current PDF,
  and the GitHub `.git` URL resolves to the repository page.
- `http://www.w3.org/2000/svg` appears only as an XML namespace inside an inline
  SVG string and should not be treated as an external citation.
- The many URLs embedded in the generated Doxygen template comments were not
  treated as authored citations. The active Doxygen settings were reviewed
  instead.
- Internal Org `[[file:...]]` links under `docs/` resolve to existing files, but
  existence is not enough: the fold-right excerpt in
  `docs/typeclass-object-pattern-in-cpp.org` no longer matches the cited test
  file.

## Suggested Immediate Order

1. Decide and encode the compiler-support policy for default `make test`
   versus `TOOLCHAIN=gcc-16`.
2. Fix `docs/mrdocs.yml`, `docs/Doxyfile`, and the broken dark-mode JS.
3. Update all Applicative prose to the current `pure + invoke` or `pure + ap`
   dual-basis model.
4. Add a C++26 SIMD test lane for GCC 16.
5. Clean the citation system and stale examples in the Org docs.
