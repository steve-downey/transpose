# Decision Log — grading in beman.transpose

Convention: each section answers one **question**, identified by a slug named
for the question, never the chosen answer (so the slug survives reversal:
`#empty-grade-spelling`, not `#bare-t`). All references anywhere in the repo
render as links to these anchors. Open questions live in the same namespace
with `Status: OPEN`; answering one graduates it in place — the slug and every
existing link stay valid. Divergences append dated entries to the implicated
question's Log; they do not get their own files.

Entry shape: Question / Status / Decision / Why / Log.

---

## empty-grade-spelling

**Question:** How is the ∅ (empty) grade spelled — bare `T` or
`expected<T, error_set<>>`?
**Status:** DECIDED 2026-08-28
**Decision:** Bare `T`. The uniform degenerate-expected form remains available
as an explicit spelling with the isomorphism documented once
(see [uniform-form-surface](#uniform-form-surface)).
**Why:** Bare `T` makes the unit fiber *identical* to the ungraded API, not
merely isomorphic — pure paths deduce character-for-character what they deduce
today, so grading has zero footprint until `error_set` is uttered. The
subsumption coercion out of ∅ is `expected`'s existing converting constructor
from `T`; the standard already implements η's coercion. Known deliberate
divergence from libfn, which spells the identity monad
`fn::expected<T, fn::copack<>>` (uniform family, unconstructible error
alternative) — a coherent choice for a closed pipeline library, the wrong
trade for an open boundary.
**Sentinel:** any framework path materializing `expected<T, error_set<>>`
without the user writing it violates this decision.
**Log:**
- 2026-08-28 — Adopted in planning discussion (graded monads thread).
- 2026-08-28 — Sentinel partially mechanized by stage
  [baseline-capture](transpose-grading-plan.md#baseline-capture): bare `int`
  is pinned as an *unregistered* context in the golden tests, so promotion at
  ∅ (stage [crtp-absorption](transpose-grading-plan.md#crtp-absorption))
  cannot arrive by quietly registering bare values as a carrier.

---

## grading-footprint

**Question:** What is grading permitted to change for existing clients at the
API level?
**Status:** DECIDED 2026-08-28
**Decision:** Grading claims only previously-ill-formed territory. For any
input combination that deduces a type today, the graded framework deduces the
same type. `error_set` appears in a deduced result only when an input already
carried one, or the combination was previously ill-formed (mixing
`expected<T,E1>` with `expected<T,E2>`, which now joins). Corollary — no
spontaneous singletons: unmixed `expected<T,E>` pipelines never become
`expected<T, error_set<E>>`; join is lazy, entering only at a genuine mixing
point. Graded and ungraded CPO overloads are mutually exclusive by constraint
(structural presence of `error_set`), never merely ranked.
**Why:** This is the entire additive-compatibility story, and it is available
only because of [empty-grade-spelling](#empty-grade-spelling). Accepted
residue: previously-ill-formed combinations becoming well-formed flips
detection idioms (`requires`, SFINAE) — additive and tolerated; changed
deductions on previously-valid code are not tolerated.
**Log:**
- 2026-08-28 — Adopted; mechanized as the golden deduction tests in stage
  [baseline-capture](transpose-grading-plan.md#baseline-capture).
- 2026-08-28 — DIVERGENCE, raised by
  [baseline-capture](transpose-grading-plan.md#baseline-capture); awaiting
  resolution. *Plan said:* the baseline matrix includes unmixed
  `expected<T,E>`, and the newly-well-formed territory is "mixing
  `expected<T,E1>` with `expected<T,E2>`". *Reality:* `std::expected` appears
  nowhere in the repository — not registered as an applicative, monad, or
  functor — so neither the unmixed nor the mixed case deduces anything today.
  Both are ill-formed, and for the same reason: no instance, rather than no
  join. *Assessment:* the Why is unharmed and arguably strengthened. For
  `expected` the graded framework claims entirely new territory, so additive
  compatibility there is trivially satisfied; the real compatibility burden
  falls on the optional / vector / sender / zip_list / array paths, which the
  goldens now pin. *Proposed resolution:* keep this decision as written;
  capture the baseline over the carriers that exist; pin `expected`'s
  non-registration as an explicit negative golden so a later stage flipping it
  is deliberate; and settle where the ungraded instance comes from under
  [expected-instance-introduction](#expected-instance-introduction).

---

## error-set-identity

**Question:** Is `error_set` a nominal type or any suitably normalized
structural sum?
**Status:** DECIDED 2026-08-28
**Decision:** Nominal. `error_set<Es...>` is its own type: canonicalization
(sorted, deduplicated via type ordering) is a class invariant enforced at
construction; converting constructors encode exactly the `⊆` widenings and
nothing else; the data-facing API is deliberately impoverished (visitation and
membership for `recover`; not a variant competitor). May share storage/visit
machinery with a structural sum by composition or private inheritance; the
public identity never slices into the structural world.
**Why:** Type as semantics. A sum of types is representation; "the set of
errors this computation may raise" is an interpretation, and interpretations
need names. Nominality makes structural grade detection sound (it conscripts
only the willing) and dissolves the datum-vs-grade ambiguity at the
declaration site: `expected<T, variant<A,B>>` is a value-level sum error,
`expected<T, error_set<A,B>>` is the graded carrier. Having *only* the subset
conversions is the coherence argument compiled into the overload set (sorted
normalization = names-not-positions; inclusions unique). Precedents:
`chrono::duration`, `std::byte` — semantics carried by what the type refuses.
**Log:**
- 2026-08-28 — Adopted.
- 2026-08-29 — ADDITION, made by stage
  [graded-deduction](transpose-grading-plan.md#graded-deduction); flag if
  unwanted. `error_set_of` gained a defaulted `operator==`, which the
  impoverished-API list (visitation, membership) did not enumerate. *Reason:*
  `std::expected<T,E>` compares equal whenever `E` does, so without it
  `expected<T, error_set<...>>` silently loses equality and grading becomes a
  usability regression at precisely the point it claims to be additive. That
  reads as against the spirit of [grading-footprint](#grading-footprint) even
  though its letter does not apply, these types being new. *Assessment:*
  equality on a sum of comparable alternatives is not a step toward competing
  with `variant` — it exposes no alternative, no index, and no storage. It is
  defaulted, so it is deleted rather than ill-formed when an alternative is
  not comparable.

---

## grade-machinery-home

**Question:** Where does grade machinery live — typeclass surface, or
framework/grade-algebra infrastructure?
**Status:** DECIDED 2026-08-28
**Decision:** Never on the typeclass surface. `grade_of<Ctx>` and
`rebind_grade<Ctx,G>` are framework traits with structural defaults
(pattern-match the `error_set` shape; everything else is ∅-graded;
customizable for exotic carriers). `join`/`bottom`/`subsume` are operations of
the grade algebra (`grade_semilattice` concept), written once, with
`error_set` as the shipped model. The CRTP base absorbs promotion of bare
values at ∅, ap-from-bind, and defaulted subsume as constrained members that
SFINAE away silently.
**Why:** Gadget-author ergonomics: registering a kind must stay "a couple of
functions," and duck typing at use sites is a feature to preserve. Invariant:
an instance that knows nothing about grades works verbatim, treated as
uniformly ∅-graded; the monad instance remains *identical to* the applicative
instance because grades ride in deduced return types — there is no separate
graded spelling to diverge into.
**Sentinel:** a typeclass instance that must declare grade participation, or
contains a bare/graded mixed-case overload, means the factoring has leaked.
**Log:**
- 2026-08-28 — Adopted.
- 2026-08-28 — Spelling snag found by
  [baseline-capture](transpose-grading-plan.md#baseline-capture): `join` in
  this library already means monadic join. Deferred to
  [grade-operation-spelling](#grade-operation-spelling); does not disturb the
  decision, which is about where the operations live, not what they are
  called.

---

## applicative-objects

**Question:** How do the two applicative structures over one carrier
(short-circuit, accumulating) relate, and which owns bind?
**Status:** DECIDED 2026-08-28
**Decision:** Two distinct NTTP-pinned typeclass objects over the same carrier
and grade algebra. Bind belongs to the short-circuit (monad-derived) object
only; the framework never auto-derives bind for the accumulating object.
Traverse takes an explicit, defaulted applicative-object policy
(see [traverse-policy-surface](#traverse-policy-surface)). Traversal order is
specified normatively: left-to-right.
**Why:** Accumulation has no graded bind — when the first computation failed
there is no value to feed the continuation, so the promised joined grade
cannot be honored; a value-flow obstruction no grade bookkeeping fixes.
Normative order is required because grades are order-blind (∪ commutes) while
short-circuit *values* are order-sensitive (which error is observed first
depends on child order) — the types cannot pin what the wording must.
**Log:**
- 2026-08-28 — Adopted.

---

## recover-grade-inference

**Question:** Are grades narrowed by `recover` inferred or annotated at fold
boundaries?
**Status:** DECIDED 2026-08-28
**Decision:** Annotated and checked. `recover` handling {H} takes grade e to
(e ∖ H) ∪ raised; inside recursive folds, inference of the resulting grade is
a least-fixed-point computation on the lattice. The user annotates the
intended grade; the framework verifies. Inference only where no fixpoint is
required.
**Why:** The fixpoint is finite (bounded lattice) but runs as template
recursion — compile-time cost and specification complexity for a rare
construct. Annotate-and-check keeps grade computation a fold of joins.
**Log:**
- 2026-08-28 — Adopted.

---

## grade-generality

**Question:** How general is the grade algebra — is `error_set` the grade, or
one model of a grade concept?
**Status:** DECIDED 2026-08-28
**Decision:** One model of a concept. The framework layer speaks only grade
vocabulary (`grade_of`, `join`, `subsume`, `bottom`); error vocabulary
(`error_set`, `recover`, ⊆-as-may-raise) is confined to the model layer. A
future algebra is adopted by supplying a `grade_semilattice` model with
decidable type-level canonical forms. Deliberately excluded: non-idempotent
grade monoids (cost, fuel) — fold grades would become runtime-shape-dependent,
forcing indexed data types; fuel is a perpendicular NTTP axis prototyped in
compile-time-scheme, entering (if ever) as a componentwise product of grades,
never through the error door.
**Why:** The three semilattice properties are load-bearing everywhere:
commutative ⇒ order-free grade arithmetic; idempotent ⇒ shape-independent
fold grades (Fix stays ungraded); order-from-join ⇒ canonical, coherent
subsumption. The concept-with-one-shipped-model posture is kept honest by a
second test-only model (see [optional-grade-model](#optional-grade-model) and
stage [law-harness](transpose-grading-plan.md#law-harness)): an abstraction
with one model is renamed, not generic.
**Log:**
- 2026-08-28 — Adopted.

---

## uniform-form-surface

**Question:** What is the surface for the explicit uniform spelling (the
documented iso from [empty-grade-spelling](#empty-grade-spelling)) —
constructor, named function, or both?
**Status:** OPEN
**Log:**
- 2026-08-28 — Raised during planning.

---

## datum-entry-point

**Question:** Is an `as_computation`-style explicit entry needed for the rare
"error_set as a datum" author, or does the
[error-set-identity](#error-set-identity) fence make it YAGNI until a
divergence proves otherwise?
**Status:** OPEN
**Log:**
- 2026-08-28 — Raised during planning; default posture is YAGNI.

---

## traverse-policy-surface

**Question:** How is the traverse applicative-object policy spelled at call
sites — NTTP object parameter or tag type?
**Status:** DECIDED 2026-08-29
**Decision:** The policy is the NTTP-pinned applicative object itself, passed
as a **trailing defaulted value parameter** — the shape `std::ranges`
algorithms use for `comp = {}, proj = {}`. The parameter is constrained. No
tag types.
**Why:** An explicit template parameter (`traverse<accumulating>(f, xs)`)
would rule out ever spelling `traverse` as a CPO, since a function object
cannot take explicit template arguments — the surface decision would silently
foreclose a customization-point decision that has not been made. A trailing
defaulted value parameter keeps both open, and it is already the idiom the
standard library uses for exactly this shape of optional policy, so it needs
no explanation in the paper. Tag types are rejected because they add a
vocabulary the library does not otherwise use, and a tag is a second thing to
keep in sync with the object it names when the object is right there and
already NTTP-pinnable.
**Log:**
- 2026-08-28 — Raised during planning.
- 2026-08-29 — Answered: trailing defaulted constrained value parameter, on
  the ranges `comp={}, proj={}` precedent; no tag types. The deciding
  consideration was that an explicit-template spelling forecloses a CPO
  `traverse`.

---

## optional-grade-model

**Question:** Where does the Boolean semilattice model ({⊥,⊤}, the "may-fail
bit") live long-term — test-only leak detector, or shipped as `optional`'s
official grade registration?
**Status:** OPEN
**Log:**
- 2026-08-28 — Raised during planning. Note: shipping it quietly unifies
  optional and expected under one framework and likely belongs in the paper's
  rationale either way.

---

## expected-instance-introduction

**Question:** Where does the *ungraded* `std::expected` applicative/monad
instance come from — its own stage before
[graded-deduction](transpose-grading-plan.md#graded-deduction), or does
grading introduce `expected` to this library already graded?
**Status:** DECIDED 2026-08-28
**Decision:** Ungraded first, as its own stage. `std::expected<T,E>` is
registered as an ordinary applicative and monad — one pinned error type per
instance object, so mixing `E1` with `E2` remains ill-formed — and folded into
the golden deduction matrix, before any grade machinery reaches it. Placed
immediately after [baseline-capture](transpose-grading-plan.md#baseline-capture)
as stage [expected-instance](transpose-grading-plan.md#expected-instance),
earlier than the "before graded-deduction" minimum the question asked for.
**Why:** Without an ungraded before-state the no-spontaneous-singletons
corollary of [grading-footprint](#grading-footprint) has nothing to be true
*of*: "unmixed pipelines never become `expected<T, error_set<E>>`" would be an
assertion about a type that never existed, unfalsifiable by the goldens that
exist to falsify exactly that. Registering it before
[grade-concept](transpose-grading-plan.md#grade-concept) rather than after
also puts `expected<T,E>` into the matrix that stage must classify as
∅-graded, which is a far sharper test of the ∅-default-for-unrecognized-types
mechanism than `optional` alone — `expected` is precisely the type a leaky
structural detector would misclassify, and its tripwire should fire on the
hard case. Keeping one error type per instance object is what holds the
previously-ill-formed territory closed until
[graded-deduction](transpose-grading-plan.md#graded-deduction) opens it
deliberately.
**Log:**
- 2026-08-28 — Raised by stage
  [baseline-capture](transpose-grading-plan.md#baseline-capture). No stage in
  the plan registers `expected` at all, graded or otherwise, yet
  [graded-deduction](transpose-grading-plan.md#graded-deduction) and
  [recover-narrowing](transpose-grading-plan.md#recover-narrowing) both assume
  the carrier is present. Two consequences worth weighing before answering.
  *First,* sequencing: only an ungraded-first introduction gives the
  no-spontaneous-singletons corollary of
  [grading-footprint](#grading-footprint) something to be true *of* — with no
  ungraded `expected<T,E>` ever in the library, "unmixed pipelines never
  become `expected<T, error_set<E>>`" has no before-state to preserve and
  degrades from a compatibility guarantee into an assertion about a type that
  never existed. *Second,* `pure`: an `expected` instance can only name an
  error type by taking it from the pinned instance object, so the ∅ grade has
  no `pure` of its own — which is exactly the bare-value promotion that stage
  [crtp-absorption](transpose-grading-plan.md#crtp-absorption) absorbs, and it
  arrives earlier than the plan's ordering suggests.
- 2026-08-28 — Answered: ungraded first, as a new stage placed directly after
  baseline-capture. Divergence closed; work resumed.

---

## grade-operation-spelling

**Question:** How are the grade-algebra operations spelled, given that `join`
in this library already means monadic join?
**Status:** DECIDED 2026-08-28
**Decision:** `grade_`-prefixed free verbs — `grade_join`, `grade_bottom`,
`grade_subsume` — dispatching through the grade model. Monadic `join` keeps
its name unqualified and unchanged.
**Why:** This is the library's own established shape one level up:
`monoid_combine` and `monoid_identity` are free verbs dispatching to
`Monoid<T>`, and the grade semilattice is the same kind of object. It keeps
the free-verb surface every other operation in the library presents, keeps
lookup static and explicit per the typeclass-object invariants in
`detail/typeclass_base.hpp`, and resolves the collision without introducing a
namespace level the library does not otherwise use. The prefix also reads
correctly at the call site, where the operand is a grade and not a monad.
**Log:**
- 2026-08-28 — Raised by the vocabulary audit of stage
  [baseline-capture](transpose-grading-plan.md#baseline-capture)
  ([baseline-vocabulary-audit.md](baseline-vocabulary-audit.md)).
  `beman::transpose::join(MMA&&)` and `Monad::join` are the monadic join,
  `join mma = mma >>= id`; [grade-machinery-home](#grade-machinery-home) gives
  the grade algebra an operation also named `join`, in the same namespace.
  Shallow — a spelling question, not a design one — but cheapest to settle
  before stage [grade-concept](transpose-grading-plan.md#grade-concept) writes
  the name down. Candidates: a nested `grades::` namespace, a `grade_` prefix,
  or members of the `grade_semilattice` model rather than free functions.
  Nothing else collides: `grade`, `semilattice`, `bottom`, `subsume`, and
  `lattice` return zero hits on the public surface.
- 2026-08-28 — Answered: `grade_`-prefixed free verbs, on the
  `monoid_combine` precedent. Divergence closed; work resumed.

---

## golden-vs-scheduled-assertions

**Question:** How does the golden file separate assertions that must never
change from assertions a named stage is *expected* to flip, and what does
[graded-deduction](transpose-grading-plan.md#graded-deduction)'s tripwire say
about the latter?
**Status:** DECIDED 2026-08-29
**Decision:** Separate translation units, and drop the word "golden" for the
scheduled kind. `baseline_deduction.test.cpp` is golden in full: nothing in it
is scheduled to change, and rule 4 applies to all of it.
`current_state.test.cpp` holds assertions that record what the library does
today at a point the plan intends to move; each block names the stage that
will move it. Editing the latter is ordinary work, editing the former is a
stop-and-ask. graded-deduction's tripwire names the distinction rather than
relying on the reader to infer it.
**Why:** A golden is defined by not changing, so an assertion with a scheduled
flip was never one — the fix is the vocabulary, not an exception to the rule.
Separate files make the distinction structural instead of a matter of reading
a comment carefully: "any golden test changes → STOP" stays literally true,
and the scheduled file is visibly droppable. Splitting first also makes the
block available for red/green: flipping the three negatives to their post-
graded-deduction form fails the build today, which is the failure that stage
is defined to fix.
**Log:**
- 2026-08-28 — Raised by stage
  [expected-instance](transpose-grading-plan.md#expected-instance). That
  stage's deliverable asks for "negative tests pinning the mixed case as still
  ill-formed"; [graded-deduction](transpose-grading-plan.md#graded-deduction)'s
  acceptance requires those same mixed cases to become well-formed and deduce
  `expected<T, error_set<E1,E2>>`. Filed as goldens, they make graded-deduction
  trip its own tripwire — "any golden test changes → STOP" — on assertions its
  charter requires it to reverse.
  *Fixed on the test side:* `baseline_deduction.test.cpp` is now split by
  lifetime rather than polarity. Sections 1–8 are goldens; section 9 is
  SCHEDULED, names graded-deduction as its expiry, and carries paired
  permanent controls so the negatives cannot pass vacuously. The split also
  surfaced a misfiling: `!applicative_registered<int>` is permanent and
  load-bearing — under [empty-grade-spelling](#empty-grade-spelling) promotion
  at ∅ must not arrive by registering bare values — yet it sat under a heading
  reading "what is NOT a context today", beside two pins that were about to
  flip.
  *Unresolved, and Steve's call:* graded-deduction's tripwire still says any
  golden change stops the stage, with no mention of a scheduled block.
  Amending a tripwire is what rule 4 forbids and rule 5 reserves, so the plan
  is deliberately left untouched. Options: amend the tripwire to name the
  licensed exception; move the scheduled block to its own file so "golden
  file" stays literally true; or drop the mixed-case negatives from stage 1
  and let graded-deduction assert only the positives.
- 2026-08-29 — Answered: combine the first two — rename away from "golden"
  AND split into `current_state.test.cpp`. Done before graded-deduction
  starts, so the block can be driven red first: the three negatives flipped
  to their post-stage form fail the build at this commit. Divergence closed.
