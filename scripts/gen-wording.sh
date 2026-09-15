#!/usr/bin/env bash
# scripts/gen-wording.sh                                              -*-sh-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Generates the P3200 wording fragments from the marked-up headers.
#
# Two phases. Each header is PARSED on its own -- beman.specgen reads only the
# main file's declaration/comment interleave -- and emits its document as IR;
# then one `render` takes all eight IR documents together. `--root` names each
# document's synopsis subclause, which is what keeps the root fragments
# distinct in the shared output directory.
#
# Rendering them together is what makes `--validate` see the paper rather than
# a header. The unit that has to be internally consistent is the paper: the
# applicative clause specifies `subsume` in terms of `grade_subsume`, which is
# specified over in `grade.hpp`, and a validator scoped to one header calls
# that a foreign name (steve-downey/specgen#109). Rendered together, each
# document validates against the union of all their documented names.
#
# `--split` never deletes files left by an earlier run, so the ordered
# manifest is written beside the fragments and is the record the paper's
# include order is reconciled against. One render writes one manifest, for
# the whole paper, in paper order.
#
# Usage: scripts/gen-wording.sh [output-directory]

set -o nounset
set -o errexit
set -o pipefail
IFS=$'\n\t'

readonly REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

SPECGEN="${SPECGEN:-specgen}"
GCC_TOOLCHAIN="${GCC_TOOLCHAIN:-}"
OUT_DIR="${1:-$REPO_ROOT/papers/wording}"

# One render, one manifest. Each header had its own while each was rendered
# on its own; the paper's include order is one order.
readonly MANIFEST="transpose.manifest"

# Headers to generate wording from, paired with the stable-name stem their
# subclauses live under. Add a header here once it carries specgen markup and
# validates clean.
readonly HEADERS=(
    "apply.hpp:transpose.applicative"
    "traverse.hpp:transpose.traversable"
    "transpose.hpp:transpose.alg"
    "sequence.hpp:transpose.range"
    "array.hpp:transpose.array"
    "grade.hpp:transpose.grade"
    "error_set.hpp:transpose.errset"
    "expected.hpp:transpose.expected"
)

# The specgen-supported front end is Clang 22 against a C++26 standard
# library. Flags are passed explicitly rather than through a compilation
# database, because headers are not translation units in one.
CLANG_ARGS=(
    -std=c++2c
    "-I$REPO_ROOT/include"
)
if [[ -n "$GCC_TOOLCHAIN" ]]; then
    CLANG_ARGS+=("--gcc-toolchain=$GCC_TOOLCHAIN")
fi
readonly -a CLANG_ARGS

if ! command -v "$SPECGEN" > /dev/null 2>&1; then
    echo "gen-wording: error: '$SPECGEN' not found; set SPECGEN to its path" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# The IR is an intermediate, not an artifact: it exists only to carry each
# header's document from its own parse into the one render that validates
# them against each other.
IR_DIR="$(mktemp -d)"
trap 'rm -rf "$IR_DIR"' EXIT

render_args=()
for entry in "${HEADERS[@]}"; do
    header="${entry%%:*}"
    stem="${entry#*:}"

    echo "gen-wording: $header -> $stem.*"
    "$SPECGEN" generate "$REPO_ROOT/include/beman/transpose/$header" \
        --emit-ir \
        --no-compile-commands \
        --output "$IR_DIR/$stem.ir.json" \
        -- "${CLANG_ARGS[@]}"

    render_args+=(--from-ir "$IR_DIR/$stem.ir.json" --root "$stem.syn")
done

echo "gen-wording: rendering ${#HEADERS[@]} documents as one paper"
# Run from the output directory with a relative `--split` so the manifest
# records `<stable name>.md` rather than a path particular to this machine.
(
    cd "$OUT_DIR"
    "$SPECGEN" render "${render_args[@]}" \
        --backend mpark \
        --validate \
        --paper \
        --split . \
        > "$MANIFEST"
)

# A stamp the paper build can depend on. The fragments themselves must not
# become prerequisites of the paper: the vendored MPark.WG21 Makefile passes
# every `%.md` prerequisite to pandoc as an input file.
date -u +%Y-%m-%dT%H:%M:%SZ > "$OUT_DIR/.stamp"

echo "gen-wording: wrote $OUT_DIR"
