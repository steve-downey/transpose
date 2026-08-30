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
- 2026-08-30 — Sentinel generalized off the shipped model by stage [law-harness](transpose-grading-plan.md#law-harness): the harness asserts for EVERY model that re-indexing a carrier at that model's bottom yields the bare value, so "no framework path materializes an ∅-graded carrier the user did not write" is now checked for the Boolean model too, not only for `error_set`.
  Doing so surfaced that this decision's ∅ and the framework's `unit_grade` are different types that behave differently here — see [bottom-grade-identity](#bottom-grade-identity).

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
nothing else; the data-facing API is deliberately impoverished **but Regular**
(visitation and membership for `recover`, plus equality and copy/move; not a
variant competitor). May share storage/visit machinery with a structural sum
by composition or private inheritance; the public identity never slices into
the structural world.

**Value-level invariant (amended 2026-08-29, see
[accumulation-evidence](#accumulation-evidence)):** a value holds a *non-empty
witnessed subset* of its grade — for each raised type, at most one witness.
The type says *may raise*; the value says *did raise*, and did-raise is a
subset of may-raise. Storage is per-type slots bounded by |grade|, not a
one-of union: no allocation, constexpr-clean. Equality is "same present set,
and equal witnesses". "Impoverished" was always about not competing with
`variant` — no monostate conveniences, no assignment gymnastics — never about
irregularity.
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
- 2026-08-29 — Ruling (Steve): keep the `operator==`, and amend the decision
  rather than the code. "Impoverished" meant not a variant competitor — no
  monostate conveniences, no assignment gymnastics — not irregular. Regularity
  is table stakes and was already half-present via copy/move. Decision text
  above now reads "impoverished but Regular".
- 2026-08-29 — AMENDMENT at the value level, from
  [accumulation-evidence](#accumulation-evidence): the invariant moves from
  "exactly one alternative" to "at least one", storage from a one-of union to
  per-type slots, and equality to "same present set, equal witnesses". Note
  for whoever reads this slug next: this is the second amendment to it in a
  week, and both times the cause was the same — value semantics left
  underspecified relative to type semantics. Worth suspecting that pattern
  anywhere else the log states a type-level rule without saying what the
  values do.
- 2026-08-29 — `visit` is now PARTIAL, and checked. The witnessed-subset
  amendment made it possible to hold more than one witness, which left `visit`
  with a documented but unenforced precondition and a `to_variant` that
  silently returned the leftmost witness — a dropped error surfacing far from
  its cause. Violation is now a compile error in a constant expression
  (`to_variant` calls a non-constexpr function on that path) and stops the
  program at runtime. `witness_count()` is public so a caller can check rather
  than trip: an unenforceable contract callers cannot inspect is a trap, not a
  contract. Note this is a real narrowing of the surface — `visit` was total
  before the amendment, and `error-set-identity` lists visitation as one of
  the two things the API does offer — so the multi-witness case now has no
  visitation verb at all, only per-type `witness<E>()`. If
  [recover-narrowing](transpose-grading-plan.md#recover-narrowing) wants one,
  that is a question for a new slug rather than an addition to this one.

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
- 2026-08-30 — LEAK-DETECTOR RESULT, from stage [law-harness](transpose-grading-plan.md#law-harness).
  The Boolean semilattice was registered as the promised second model and the full harness run against both.
  Verdict is split, and the split is exactly along the layer boundary this decision draws.
  *Upheld at the algebra and the carrier traits:* `tests/beman/transpose/laws.hpp` includes `grade.hpp` and nothing else from the library, never names `error_set`, `recover`, or `expected`, and both models pass it — so the framework layer really can state and check the semilattice laws without naming its shipped model.
  *Not upheld at the mixing point,* which is the one place the algebra exists for: `grade_join_t` has no callers in `include/` at all, and every graded deduction joins through `detail::joined_error_t` in error vocabulary.
  Filed as [mixing-point-vocabulary](#mixing-point-vocabulary), which is a contradiction of this decision's letter and needs Steve's ruling on whether it re-opens it.
  *A third finding,* smaller but structural, is that the framework and every model each carry their own ∅ and the two are never identified — [bottom-grade-identity](#bottom-grade-identity).
  Nothing was patched: this stage's charter is to log what the second model exposes, not to smooth it.

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
**Decision:** The policy is *the existing NTTP-pinned applicative object
itself* — not a new type — passed as a **trailing defaulted value parameter**,
the shape `std::ranges` algorithms use for `comp = {}, proj = {}`. POLICY is
constrained by a concept ("is an applicative object for this carrier"). No tag
types.
The constraint is load-bearing twice over: a stray third argument — someone's
extra container — fails loudly instead of being silently swallowed as a
policy, and it is the enforcement hook at the traverse boundary for the
framework refusing bind-derivation on the accumulating object.
**Why:** An explicit template parameter (`traverse<accumulating>(f, xs)`)
would rule out ever spelling `traverse` as a CPO, since call syntax cannot
supply template arguments to `operator()` — and in a HOF-centric library
`traverse` wants to be passable to other combinators. Working around it by
making `traverse` a variable template of callables breaks the *default* call:
`traverse(f, xs)` becomes `traverse<>(f, xs)`, changing every existing call
site and failing the goldens. So the explicit-template spelling quietly
forecloses the object form; the surface decision would settle a
customization-point question nobody has asked.
The trailing defaulted parameter costs nothing to get this: the objects are
stateless, so type is identity, dispatch is type-directed and constant-folded,
and default call sites are character-identical to today. Composition comes
free — `bind_back(traverse, accumulating)` is the accumulating traverse as a
first-class object, no wrapper lambda. It is also the LEWG-friendliest
precedent to cite, being the ranges convention rather than the
`sort(par, ...)` policy-first one.
Tag types are rejected as a *shadow identity*: `accumulating_t` would be a
second name for a structure that already has a nominal one, the applicative
object itself — precisely what the NTTP-object design exists to prevent.
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
- 2026-08-30 — The model now EXISTS, test-only, in `tests/beman/transpose/laws.test.cpp`, built by stage [law-harness](transpose-grading-plan.md#law-harness).
  Still open, and deliberately so: the carrier is a purpose-built `Fallible<T>`, not `std::optional`, because registering `std::optional` as a graded carrier would answer this question by fiat — and would contradict `grade.test.cpp`'s `is_ungraded<std::optional<int>>`, which stage [grade-concept](transpose-grading-plan.md#grade-concept) pinned as part of its ∅-default tripwire.
  Two facts for whoever answers this.
  *The price is now measured:* the algebra is ten trivial specializations and slides in cleanly, but the carrier costs four more, all transliterations of `error_set.hpp`'s, and two of the four exist only because of [bottom-grade-identity](#bottom-grade-identity).
  *The benefit is currently unavailable:* per [mixing-point-vocabulary](#mixing-point-vocabulary) a registered algebra is not a usable one, so shipping this today would register `optional` as graded without letting it join with anything.
  Answering yes probably wants that resolved first.

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

## accumulation-evidence

**Question:** What carries the evidence of *multiple* failures, when the
accumulating applicative object collects every error rather than the first?
**Status:** DECIDED 2026-08-29
**Decision:** The `error_set` value itself, as a **non-empty witnessed subset
of its grade**: for each raised type, one witness value. The grade says "may
raise {A,B}"; the value says "did raise", and did-raise is a subset of
may-raise. Both applicative objects therefore share the carrier exactly as
[applicative-objects](#applicative-objects) states — short-circuit only ever
produces singletons, accumulating combines by witnessed union.
Combining is **left-biased per type**: two A-witnesses keep the first. That is
associative, allocation-free, and deterministic *because*
[applicative-objects](#applicative-objects) already made left-to-right
traversal normative — that decision pays for this one.
Multiplicity loss is real and documented: the second A witness is dropped. The
escape hatch is honest and needs no new machinery — a user who must keep every
witness chooses a collecting error type as their `E`.
**Why:** The alternatives conflated two levels. The grade join is type-level ∪
and is always available, needing no monoid at all; what accumulation needs is
a semigroup on the *evidence*, the value-level record of what did raise.
Requiring `Monoid<E>` on user error types looks for that semigroup in the
wrong place and makes graded accumulation unavailable by construction; its
kernel survives here as "`error_set` supplies the semigroup itself". Making
the accumulating object yield a different carrier was rejected because it
breaks the decided same-carrier premise and makes the result type depend on
the policy, which poisons composition.
The confirming sign that this is the right shape rather than a patch for
accumulation: it makes `recover` compositional. Under one-of semantics,
recovering type A from a value that raised {a,b} has no meaning. Under subset
semantics it is set-difference at both levels — the handler consumes the A
witness, {b} remains an error, empty means success — so the value semantics
and the grade arithmetic become the same operation at two levels.
**Cost:** amends [error-set-identity](#error-set-identity) at the value level:
invariant from "exactly one alternative" to "at least one", storage from a
one-of union to per-type slots (bounded by |grade|, still no allocation, still
constexpr-clean), equality to "same present set, equal witnesses". The
`error_set` shipped by stage
[error-set-type](transpose-grading-plan.md#error-set-type) implements the
one-of form and must be revised before the accumulating object can exist.
**Log:**
- 2026-08-29 — Raised by stage
  [accumulating-object](transpose-grading-plan.md#accumulating-object): the
  plan specified the accumulating object's existence, its grade behaviour, and
  its lawlessness for bind, but never what carries multiple witnesses at the
  value level. The protocol's "question nobody has asked yet" case.
- 2026-08-29 — Answered by Steve.

---

## multi-witness-elimination

**Question:** How is a multi-witness `error_set` value eliminated, now that
`visit` is partial and defined only for the singleton case?
**Status:** DECIDED 2026-08-29
**Decision:** `visit` stays the one-of eliminator — partial, precondition
checked — and is **not** extended to multi-witness. Multi-witness elimination
is a **static per-type fold over the handled set**, implemented as `recover`'s
internal mechanism and **not** shipped as a public verb. The impoverished-API
fence holds until a second client outside `recover` demonstrates the need —
the same YAGNI posture as [datum-entry-point](#datum-entry-point).
**Why:** No runtime visitation verb is required, because the handled set {H}
is a compile-time type-set. Elimination is a static fold over the grade with
runtime presence filtering: for each E in H, if a witness is present, feed it
to the handler and remove it. That is expressible entirely in the API that
already exists — membership plus `witness<E>()` — so a public eliminator would
be machinery with exactly one caller.
Note also that the option one reaches for instinctively — statically excluding
multi-witness visits — does not exist. *Which* witnesses are present is a
runtime property of the value even though the grade is static, so `visit` is
*inherently* partial under the witnessed-subset amendment of
[error-set-identity](#error-set-identity). The only real choice was between a
checked precondition and a silent leftmost pick, and leftmost is a lie: it
reports "the error" of a value holding two.
This retroactively settles the narrowing logged under
[error-set-identity](#error-set-identity). The absent multi-witness visitation
verb is not a gap awaiting a verb; it is the fence line.
**Log:**
- 2026-08-29 — Raised by the `visit` hardening: making the precondition
  checked left the multi-witness case with no visitation verb at all, and it
  was unclear whether that was a hole or a boundary.
- 2026-08-29 — Answered by Steve, deliberately *before*
  [recover-narrowing](transpose-grading-plan.md#recover-narrowing) starts. The
  answer is forced, and under the fresh-agent-per-stage cadence an agent would
  otherwise hit this wall in its first hour and spend a stop-and-report cycle
  on a question that was never open.

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

---

## bottom-grade-identity

**Question:** Is the framework's `unit_grade` the same ∅ as a model's
`grade_bottom_t<G>`, or a second bottom sitting below every model's?
**Status:** OPEN
**Log:**
- 2026-08-30 — Raised by stage [law-harness](transpose-grading-plan.md#law-harness).
  Registering the Boolean semilattice as a second model made visible something the shipped model alone hid: the framework and every model each carry their own ∅, and nothing identifies them.
  Each fact below was confirmed by a negative control — invert or delete it, watch the build break, restore.
  * They are DISTINCT TYPES, and STRICTLY ORDERED.
    `unit_grade` and `error_set<>` are not the same type; `grade_subsumes_v<unit_grade, error_set<>>` is true and `grade_subsumes_v<error_set<>, unit_grade>` is false.
    So the framework's ∅ sits strictly BELOW the model's ∅ in the order the framework itself computes.
    A bounded join-semilattice has one bottom; what is registered is the model's lattice with a second bottom glued underneath.
    Everything stays internally consistent — order-from-join is satisfied in both directions — which is why no earlier stage tripped on it.
  * THE ROUND TRIP FAILS AT THE BOTTOM.
    `grade_of_t<rebind_grade_t<T, error_set<>>>` is `unit_grade`, not `error_set<>`, because the ∅ carrier is bare `T` ([empty-grade-spelling](#empty-grade-spelling)) and bare `T` reports the framework's ∅.
    The harness states the round-trip law with that carve-out written into it; removing the carve-out fails the build, so the carve-out is describing reality rather than being defensive.
  * "RE-INDEX AT ∅" IS TWO DIFFERENT FUNCTIONS.
    `rebind_grade_t<expected<int, error_set<A,B>>, unit_grade>` is `expected<int, error_set<A,B>>` — the framework's default leaves a carrier alone.
    `rebind_grade_t<expected<int, error_set<A,B>>, error_set<>>` is `int` — the model's bottom strips the carrier off.
    Under [empty-grade-spelling](#empty-grade-spelling) the second is what ∅ is supposed to mean, so the framework's single carrier-facing default is the one that is wrong for a graded carrier, and a model can only correct it for its own bottom, never for `unit_grade`.
  * THE COST TO A SECOND MODEL, measured.
    The ALGEBRA is ten trivial specializations (four joins, two bottoms, four order facts) and slides in with no friction at all.
    The CARRIER costs four more — `grade_of` on the carrier, promote-bare at ⊤, do-not-nest at ⊤, and strip-to-bare at ⊥ twice, once from the bare value and once from the carrier — and all four are transliterations of `error_set.hpp`'s, with the carrier's name swapped.
    Deleting any one of them breaks the harness.
    The framework supplies exactly one carrier specialization, `rebind_grade<CONTEXT, unit_grade>`, and it is the only one of the five no model needs.
    Two of the model's four exist purely because its ⊥ is not the framework's ∅.
  * THE FAILURE MODE IS BAD.
    Omitting one of those registrations produces `invalid use of incomplete type 'rebind_grade<...>'` pointing inside `grade.hpp`, with nothing naming the registration that is missing.
  * A SENSOR THE HARNESS NOW CARRIES: "one model, one bottom" — every grade in a sample agrees on `grade_bottom_t`.
    `unit_grade` satisfies `grade_semilattice` yet fails this the moment it is admitted to a model's grade sample, which is the cleanest statement of the problem: the framework's ∅ is a grade that belongs to no model's lattice.

  *Not patched, deliberately.*
  Candidate resolutions, none obviously right.
  (a) Identify them: require `grade_bottom_t<G>` to be `unit_grade` for every model, and models stop declaring a bottom type.
  Smallest, and it lets the framework own carrier-stripping once; but it costs `error_set<>` its standing as a grade while keeping it as the explicit uniform spelling, which reaches [empty-grade-spelling](#empty-grade-spelling) and [uniform-form-surface](#uniform-form-surface).
  (b) Keep them distinct but give the framework a constrained `rebind_grade` at any model's bottom, so stripping is written once.
  (c) Accept the duplication and document it as the price of models owning their own lattice.
  Both halves of the disagreement are now pinned by the harness, so whichever is chosen cannot happen silently.

---

## mixing-point-vocabulary

**Question:** In whose vocabulary is the join at a mixing point computed —
the framework's `grade_join`, or the model's own?
**Status:** OPEN
**Log:**
- 2026-08-30 — Raised by stage [law-harness](transpose-grading-plan.md#law-harness), and it is the sharp half of what the second model was put there to detect.
  [grade-generality](#grade-generality) decides that the framework layer speaks only grade vocabulary.
  That holds for the algebra and the carrier traits — see the leak-detector entry logged there.
  It does not hold at the mixing point.
  * `grade_join_t` HAS NO CALLERS.
    Not one, anywhere in `include/`.
    The only framework verb any shipped header consumes is `grade_subsume`, in the defaulted `subsume` member of the two CRTP bases.
    Every join a graded deduction actually performs runs through `detail::joined_error_t` in `expected.hpp`, written end to end in error vocabulary (`error_elements`, `error_set_of_elements`, `error_set<...>`).
    The framework's join verb is registered by the model, asserted by tests, and used by nothing.
  * WHERE THEY AGREE, THEY AGREE BY CONSTRUCTION.
    For two operands that are already graded, the deduced result's grade equals `grade_join_t` of the operand grades.
    That is now pinned as a sensor — but it is agreement, not dispatch, and a second algebra gets no benefit from it.
  * WHERE THEY CANNOT AGREE.
    Lazy join ([grading-footprint](#grading-footprint), no spontaneous singletons) makes `grade_of_t<expected<int, errc>>` equal `unit_grade`.
    So for the bare mixing point the framework's verbs compute ∅ ∨ ∅ = ∅ and predict a bare `int`, while the instance deduces `expected<int, error_set<errc, io_errc>>`.
    The step the instance takes and the grade layer cannot express is LIFTING A BARE ERROR TYPE INTO A SINGLETON SET — which is error vocabulary by definition.
    This is not a bug in either layer; it is the observation that the newly-claimed territory of [grading-footprint](#grading-footprint) is reachable only in the model's language.
  * THE PRACTICAL CONSEQUENCE.
    A second algebra can be REGISTERED but not USED.
    The Boolean model passes every law in the harness and still cannot participate in a graded deduction, because the graded core of `ExpectedApplicativeImpl` produces an `error_set` regardless of which algebra the operands' grades belong to.
    "An abstraction with one model is renamed, not generic" is the test [grade-generality](#grade-generality) set for itself; on this reading the abstraction currently has one model at the layer that matters.

  *Steve's call, and it may re-open a DECIDED entry.*
  Logging rather than stopping is what stage [law-harness](transpose-grading-plan.md#law-harness) instructs for exactly this finding, so the stage completed; but this contradicts the letter of [grade-generality](#grade-generality), which is more than a stage-local divergence.
  *Not patched:* routing the mixing point through `grade_join` needs the grade layer to gain something the ∅ grade currently forbids — either a singleton-lifting operation, or a `grade_of` that reports a singleton for `expected<T,E>`, which [grading-footprint](#grading-footprint) rules out at the deduction level even if not necessarily at the trait level.
  Either is a design decision, not a stage deliverable.
