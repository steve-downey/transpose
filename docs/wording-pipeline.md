# The wording pipeline

<!-- SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception -->

P3200's normative wording is generated from the headers, not written by hand.
`beman.specgen` lowers the specgen markup on each in-scope header to a semantic
IR and renders mpark/wg21 markdown; `papers/D3200R0.md` splices the
fragments in at build time. See [`decisions.md#wording-generation`](decisions.md).

## The loop

```sh
make wording        # regenerate papers/wording from the headers
make wording-check  # fail if the checked-in fragments have drifted
make papers         # regenerate, then build the PDF
```

`make wording` runs [`scripts/gen-wording.sh`](../scripts/gen-wording.sh),
which needs `specgen` on `PATH` (or `SPECGEN=` pointing at it) and a Clang 22
development install. The generated fragments are checked in, so the paper
builds without either.

## What generates what

`specgen` reads only the main file's declaration/comment interleave, so each
header is rendered on its own and contributes its own synopsis subclause.
`--root` names that synopsis, which keeps the root fragments distinct in the
shared output directory.

| header | `--root` | subclauses |
| ------ | -------- | ---------- |
| `apply.hpp` | `transpose.applicative.syn` | `.basis` `.derived` `.grade` `.delegate` `.optional` |
| `traverse.hpp` | `transpose.traversable.syn` | `.ops` `.delegate`, plus `transpose.alg.traverse` |
| `transpose.hpp` | `transpose.alg.syn` | `transpose.alg.transpose` |
| `sequence.hpp` | `transpose.range.syn` | `.traverse` |
| `array.hpp` | `transpose.array.syn` | `.applicative` `.tuple` |
| `grade.hpp` | `transpose.grade.syn` | `.subsume` |
| `error_set.hpp` | `transpose.errset.syn` | `.cons` `.obs` `.ops` `.empty` `.lattice` `.recover` |
| `expected.hpp` | `transpose.expected.syn` | `.applicative` `.accumulating` |

`--split` never deletes files an earlier run left behind, so each header's
ordered manifest is written beside the fragments; it is the record the paper's
include order is reconciled against.

Headers outside the proposal carry no markup and are not generated from:
`fold.hpp`, `monad.hpp`, `monoid.hpp`, `dual_monoid.hpp`, `functor.hpp`,
`sender.hpp`, `zip_list.hpp`, `simd*.hpp`, `config.hpp`. `sender` and
`zip_list` are non-normative demonstration types (see
[`provenance.md`](provenance.md)); Monad and Foldable are stated non-goals, so
their instances inside generated headers are `\omit`ted.

## Rules for marking up a header

Comment spelling is significant. `//!` and `/*! */` are specgen docblocks and
become wording; `///` and `/** */` are Doxygen and are stripped; plain `//`
survives as a draft comment where the corresponding source is rendered. Design
commentary belongs in `///` so that it does not land in a rendered synopsis.

**Declare members in-class, define them out of line.** This is not optional.
specgen keeps an in-class body in the class synopsis when that body names an
entity from another file -- which every non-trivial body here does -- and the
synopsis then shows implementation rather than declarations. Out-of-line
definitions of members with an explicit object parameter are accepted by
GCC 14/15/16 and Clang 21/22, so the pattern costs nothing.

A constraint on an out-of-line definition must be spelled with the *same
tokens* as the in-class declaration's. Class scope is not in effect where the
out-of-line requires-clause appears, so a constraint that names a member
(`contains<E>()`) or the injected class name (`error_set_of`) cannot be
repeated there. Give such a constraint a namespace-scope spelling that works
in both positions -- `error_set_has_v` in `error_set.hpp` is the worked
example.

**Route members to sections** with a `// \ref{stable.name}, group` comment in
the class body, and declare the section with a `// \rSec3[stable.name]{Title}`
marker at namespace scope. Keep both on one source line: `clang-format` will
wrap a long one and specgen then reports a malformed marker.

**Keep `detail` out of declarations.** See
[`decisions.md#wording-visible-internals`](decisions.md) for the rule and the
list of names it moved. Bodies may name `detail` freely.

**Neither verbatim marker works.** `\verbatim-itemdecl` and
`\verbatim-synopsis` both emit the authored text *and* the parsed declaration,
so the item or the class synopsis appears twice and whatever was being masked
survives in the second copy (steve-downey/specgen#4). Until that is fixed there
is no way to keep a name out of a rendered synopsis except to move the entity.

## Transclusion

[`papers/filters/transclude.py`](../papers/filters/transclude.py)
replaces a fenced div carrying the `include` class with the parsed blocks of
the file it names:

```markdown
::: {.include file="wording/transpose.applicative.basis.md"}
:::
```

It is selected by [`papers/defaults.yaml`](../papers/defaults.yaml), which
MPark.WG21's `base.mk` already layers after its own `doc` and `formatting`
defaults.
A defaults file *replaces* a list-valued key rather than merging into it, so
that file restates the whole filter chain; transclusion runs first, so the
`[x]{.pnum}` and `[stable.name]{- .sref}` spans inside a fragment reach
`wg21.py` exactly as if they had been typed into the paper.

The fragments must not become prerequisites of the paper: `base.mk` computes
its pandoc inputs as `$(filter %.md, $^)` and would hand each fragment to
pandoc as a further input file. [`papers/Makefile`](../papers/Makefile) depends
on `wording/.stamp` instead, which `make wording` writes.

Building the paper prints `mpark/wg21: stable name transpose.* not found` for
every new subclause. That is expected: the names are not in the C++ draft's
stable-name database, so they render as plain bracketed text rather than as a
link.

## The vendored framework

MPark.WG21 is a squashed subtree at `papers/wg21`, so it updates with

```sh
git subtree pull --prefix=papers/wg21 wg21-upstream master --squash
```

given a `wg21-upstream` remote pointing at <https://github.com/mpark/wg21>.
The subtree is unpatched, and should stay that way: everything this project
adds lives beside it in `papers/`, which is the layout `wg21/flat.mk`
documents.

[`papers/filters/bibliography-header.py`](../papers/filters/bibliography-header.py)
is the one shim. `wg21.py` removes an `unnumbered` class from the generated
references header unconditionally, and pandoc 3.9's citeproc does not set it,
so every paper carrying a citation aborts the filter with `ValueError`. A
twelve-line paper with one `[@Pnnnn]` reproduces it. The shim runs between
citeproc and `wg21.py` and puts the class back; delete it once upstream guards
the removal.
