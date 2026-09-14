# Decision Log — grading in beman.transpose

Convention: each section answers one **question**, identified by a slug named
for the question, never the chosen answer (so the slug survives reversal:
`#empty-grade-spelling`, not `#bare-t`). All references anywhere in the repo
render as links to these anchors. Open questions live in the same namespace
with `Status: OPEN`; answering one graduates it in place — the slug and every
existing link stay valid. Divergences append dated entries to the implicated
question's Log; they do not get their own files.

Entry shape: Question / Status / Decision / Why / Log.
New or amended DECIDED entries also carry `Decided by` so later agents can distinguish Steve rulings from stage-local observations.

---

## empty-grade-spelling

**Question:** How is the ∅ (empty) grade spelled — bare `T` or
`expected<T, error_set<>>`?
**Status:** DECIDED 2026-08-28
**Decided by:** Planning discussion; Stage 8 generalization affirmed by Steve Downey 2026-08-30.
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
  Steve's ruling accepts the generalization but changes the implementation target: framework-∅ is a pure sentinel, not a model grade or carrier.

---

## grading-footprint

**Question:** What is grading permitted to change for existing clients at the
API level?
**Status:** DECIDED 2026-08-28
**Decided by:** Planning discussion; amended by Steve Downey 2026-08-30.
**Decision:** Grading claims only previously-ill-formed territory. For any
input combination that deduces a type today, the graded framework deduces the
same type. `error_set` appears in a deduced result only when an input already
carried one, or the combination was previously ill-formed (mixing
`expected<T,E1>` with `expected<T,E2>`, which now joins). Corollary — no
spontaneous singletons: unmixed `expected<T,E>` pipelines never become
`expected<T, error_set<E>>`; join is lazy, entering only at a genuine mixing
point. Graded and ungraded paths are mutually exclusive by semantic
`grade_of` / `graded_context` constraints; expected's lazy-spelling path also
requires declared-error matching, so foreign graded carriers are not treated
as bare operands.
**Why:** This is the entire additive-compatibility story, and it is available
only because of [empty-grade-spelling](#empty-grade-spelling). Accepted
residue: previously-ill-formed combinations becoming well-formed flips
detection idioms (`requires`, SFINAE) — additive and tolerated; changed
deductions on previously-valid code are not tolerated.
**Log:**
- 2026-08-28 — Adopted; mechanized as the golden deduction tests in stage
  [baseline-capture](transpose-grading-plan.md#baseline-capture).
- 2026-08-28 — DIVERGENCE, raised by
  [baseline-capture](transpose-grading-plan.md#baseline-capture). *Plan said:*
  the baseline matrix includes unmixed
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
- 2026-08-28 — Closed by
  [expected-instance-introduction](#expected-instance-introduction):
  `expected` was registered ungraded first, then the later grade stages
  preserved its existing same-error carrier spelling while adding only the
  previously ill-formed mixed-error territory.

---

## error-set-identity

**Question:** Is `error_set` a nominal type or any suitably normalized
structural sum?
**Status:** DECIDED 2026-08-28
**Decided by:** Planning discussion; amended by Steve Downey 2026-08-29.
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
**Decided by:** Planning discussion; Stage 9 amendments by Steve Downey 2026-08-30.
**Decision:** Never on the typeclass surface. `grade_of<Ctx>` and
`rebind_grade<Ctx,G>` are framework traits: `grade_of` defaults unregistered
carriers to the ungraded sentinel, and models specialize registered carriers
to model grades. `join`/`bottom`/`subsume` are operations of the grade algebra
(`grade_semilattice` concept), written once, with `error_set` as the shipped
model. Plain-error `std::expected<T, E>` is semantically graded at
`error_set<E>` per [plain-error-grade-reading](#plain-error-grade-reading);
lazy join remains a carrier-spelling rule, not a second grade trait. The CRTP
base carries ap-from-bind and the defaulted `grade_subsume` coercion as
constrained members that SFINAE away silently. Mixing-point bare-operand
lifting lives in the model-dispatched deduction helpers, where an ungraded
operand is lifted to the current model's bottom.
**Why:** Gadget-author ergonomics: registering a kind must stay "a couple of
functions," and duck typing at use sites is a feature to preserve. Invariant:
an instance that knows nothing about grades works verbatim, treated as
uniformly ∅-graded; the monad instance remains *identical to* the applicative
instance because grades ride in deduced return types — there is no separate
graded spelling to diverge into.
**Sentinel:** a typeclass instance that must declare grade participation, or a
second grade-reading trait beside `grade_of`, means the factoring has leaked.
**Log:**
- 2026-08-28 — Adopted.
- 2026-08-28 — Spelling snag found by
  [baseline-capture](transpose-grading-plan.md#baseline-capture): `join` in
  this library already means monadic join. Deferred to
  [grade-operation-spelling](#grade-operation-spelling); does not disturb the
  decision, which is about where the operations live, not what they are
  called.
- 2026-08-30 — Amended by [plain-error-grade-reading](#plain-error-grade-reading) and [grade-model-identity](#grade-model-identity).
  The framework still owns the traits, but the ungraded sentinel is no longer a grade and registered carriers report model grades through the single `grade_of` question.

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
- 2026-08-30 — REPAIR, by stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing).
  `grade_join_t` now drives expected mixing points and the Boolean model drives an actual mixed deduction, so the leak detector's positive result has been repaired rather than documented around.

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
- 2026-08-30 — Superseded by
  [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing):
  the Boolean model now drives a real mixed deduction, so the "registered but
  not usable" objection above is closed. The question remains open because
  shipping the Boolean model as `optional`'s grade registration is a public
  surface decision, not an implementation-blocker.

---

## plain-error-grade-reading

**Question:** What grade does `grade_of<std::expected<T, E>>` report when `E` is a plain error type rather than an `error_set`?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** It reports the singleton model grade `error_set<E>`.
This is type-level bookkeeping only: an unmixed pipeline still deduces `std::expected<T, E>`, not `std::expected<T, error_set<E>>`.
Lazy join is a discipline on deduced carrier spellings, not on semantic grade reading.
There is no parallel grade-reading trait; consumers ask `grade_of` once.
**Why:** `std::expected<T, E>` is a registered carrier in the `error_set` family, so reporting the framework's ungraded sentinel would violate [bottom-grade-identity](#bottom-grade-identity), which says `grade_of` yields either ungraded or a model grade.
The singleton reading materializes no new runtime or deduced carrier form; it only lets the framework compute joins in grade vocabulary.
**Log:**
- 2026-08-30 — Raised by the Opus sanity check before [grade-concept](transpose-grading-plan.md#grade-concept), but not given its own slug at the time.
  The grade-concept stage pinned the opposite answer in tests by treating plain-error `expected` as ungraded.
- 2026-08-30 — Stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing) initially tried to route around the tension with a second per-model `mixing_grade` hook.
  Steve stopped that direction: two grade-reading traits would make every consumer know which one lies.
  The repair instead changes `grade_of<std::expected<T, E>>` to the singleton semantic grade and preserves lazy spelling at deduction sites.

---

## grade-model-identity

**Question:** How does the framework know whether two grades belong to the same model?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** A public `grade_model<G>` trait maps every model grade to a nominal model tag.
Same-model constraints compare those tags.
The tag carries identity only: joins, bottoms, and subsumption remain operations on grades through `grade_join`, `grade_bottom`, and `grade_subsumes`.
Join-fold and sentinel-lift helpers used by implementation code stay in `detail`.
**Why:** [cross-model-mixing](#cross-model-mixing) needs a way to reject operands from different grade models without inventing a product model.
Nominal model tags are the smallest public vocabulary that expresses that constraint while keeping model operations on the existing grade algebra hooks.
**Log:**
- 2026-08-30 — Added by stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing) under Steve's ruling.
  The aborted `mixing_grade` direction was not kept; the only new public framework hook is model identity.

---

## expected-instance-introduction

**Question:** Where does the *ungraded* `std::expected` applicative/monad
instance come from — its own stage before
[graded-deduction](transpose-grading-plan.md#graded-deduction), or does
grading introduce `expected` to this library already graded?
**Status:** DECIDED 2026-08-28
**Decided by:** Planning discussion; amended by Steve Downey 2026-08-30.
**Decision:** Ungraded first, as its own stage. `std::expected<T,E>` is
registered as an ordinary applicative and monad — one pinned error type per
instance object, so mixing `E1` with `E2` remains ill-formed until
[graded-deduction](transpose-grading-plan.md#graded-deduction) — and folded
into the golden deduction matrix before grade machinery reaches its
deductions. Placed immediately after
[baseline-capture](transpose-grading-plan.md#baseline-capture) as stage
[expected-instance](transpose-grading-plan.md#expected-instance), earlier than
the "before graded-deduction" minimum the question asked for. Amended by
[plain-error-grade-reading](#plain-error-grade-reading): once grade machinery
exists, `grade_of<std::expected<T, E>>` reports the singleton semantic grade;
the ungraded-first before-state is about carrier spelling and typeclass
registration, not the final grade trait answer.
**Why:** Without an ungraded before-state the no-spontaneous-singletons
corollary of [grading-footprint](#grading-footprint) has nothing to be true
*of*: "unmixed pipelines never become `expected<T, error_set<E>>`" would be an
assertion about a type that never existed, unfalsifiable by the goldens that
exist to falsify exactly that. Registering it before
[grade-concept](transpose-grading-plan.md#grade-concept) rather than after
also puts `expected<T,E>` into the matrix before graded deduction can change
its carrier spelling, which is the hard case the no-spontaneous-singletons
corollary needs. Keeping one error type per instance object is what holds the
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
- 2026-08-30 — Amended by [plain-error-grade-reading](#plain-error-grade-reading).
  The original "ungraded first" answer remains true for the staged
  introduction and the deduced carrier spellings, but not for the final
  semantic `grade_of` reading after grade machinery is installed.

---

## grade-operation-spelling

**Question:** How are the grade-algebra operations spelled, given that `join`
in this library already means monadic join?
**Status:** DECIDED 2026-08-28
**Decided by:** Planning discussion; amended by Steve Downey 2026-08-30.
**Decision:** `grade_`-prefixed operations — `grade_join`, `grade_bottom`,
the order predicate `grade_subsumes`, and the carrier coercion
`grade_subsume`. Monadic `join` keeps its name unqualified and unchanged.
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
- 2026-08-30 — Amended by stage
  [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing):
  the order question and the value coercion have distinct names.
  `grade_subsumes` is the type-level predicate used in constraints;
  `grade_subsume` is the coercion that re-indexes a carrier at a wider grade.

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
**Decided by:** Steve Downey.
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
- 2026-08-30 — Narrow ruling by Steve, logged during grading-fidelity
  remediation: after a scheduled block has expired, a stage may append that
  block's positive post-stage form to the golden file only when no existing
  golden assertion is weakened or edited. This retroactively licenses the
  Stage 5 shape; it is not permission to change established golden coverage.

---

## bottom-grade-identity

**Question:** Is the framework's `unit_grade` the same ∅ as a model's
`grade_bottom_t<G>`, or a second bottom sitting below every model's?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** The framework ∅ is a pure sentinel, not a grade in any model lattice and not a carrier-bearing bottom.
`grade_of` yields either that ungraded sentinel or a model grade.
Bottoms exist only per model, such as `error_set<>` and `never_fails`.
Promotion is where the sentinel and a model meet: an ungraded operand entering a mixing point lifts to that mixing point's model bottom.
One sentinel, N bottoms, zero duplication.
**Why:** Bare `T` is the unit fiber of every graded family simultaneously, so materializing a framework bottom as if it were a grade with carrier machinery creates a shadow model.
The Boolean model exposed that duplication by forcing the framework bottom and model bottoms to be compared outside the shipped `error_set` vocabulary.
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
- 2026-08-30 — Ruling by Steve: choose the sentinel interpretation, not identification and not documented duplication.
  Framework-∅ is the model-less state "not yet in a family"; model bottoms remain model-owned.
  The repair belongs in stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing), where promotion at a mixing point lifts ungraded operands to that point's model bottom.
- 2026-08-30 — Implemented by stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing).
  `unit_grade` no longer models `grade_semilattice`, `rebind_grade<CONTEXT, unit_grade>` no longer exists, and implementation helpers lift the sentinel to the current model's bottom only inside model-dispatched joins.

---

## mixing-point-vocabulary

**Question:** In whose vocabulary is the join at a mixing point computed —
the framework's `grade_join`, or the model's own?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** Fix the mixing point rather than documenting around it.
Mixing-point deduction is `rebind_grade<Carrier, grade_join<g1, g2>>`, dispatched through the operands' grade model.
No shipped mixing-point code utters `error_set` outside the model layer.
If an ungraded operand participates in a typed mixing point, it is first lifted to that point's model bottom as decided by [bottom-grade-identity](#bottom-grade-identity).
**Why:** [grade-generality](#grade-generality) says `error_set` is one model of a grade concept, and the Boolean model proved that the current deduction path registers a second algebra without letting it drive the machinery.
The repair must happen while Steve is still the sole client, because after a second client this change would alter meanings rather than only repair an abstraction boundary.
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
- 2026-08-30 — Ruling by Steve: this is a design bug with the same root cause as [bottom-grade-identity](#bottom-grade-identity), not an acceptable documented limitation.
  The next stage must make the Boolean model drive an actual mixed deduction before [paper-revision](transpose-grading-plan.md#paper-revision) starts.
- 2026-08-30 — Implemented by stage [model-dispatched-mixing](transpose-grading-plan.md#model-dispatched-mixing).
  The aborted `mixing_grade` direction was removed; mixing deductions use `grade_of`, same-model checks via `grade_model`, and `grade_join_t` folded over operands after lifting only ungraded operands to the current model bottom.

---

## cross-model-mixing

**Question:** May a mixing point combine graded operands from different grade models?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** No.
Both graded operands at a mixing point must belong to the same grade model; cross-model mixing is ill-formed by constraint.
Do not build product-of-models machinery.
**Why:** Model-dispatched mixing needs one model to supply the join and bottom-lift vocabulary for the deduction.
A product of grade models is a new semantic feature with no current client, the same YAGNI boundary recorded for fuel and other perpendicular axes in [grade-generality](#grade-generality).
**Log:**
- 2026-08-30 — Raised and answered by Steve's ruling on the Stage 8 leak-detector findings.
  The absence of cross-model products is intentional and belongs in the log so the next stage treats it as a constraint, not an oversight.
- 2026-08-30 — Grading-fidelity review found a constraint divergence in
  expected's same-error-plus-bare cores: "bare" meant "not `std::expected`,"
  so a foreign graded carrier could enter the lazy expected path and bypass
  the same-model check. Fixed by making "bare" mean semantic ungraded
  (`grade_of_t<T> == unit_grade`, spelled through `graded_context`) while
  preserving declared-error matching for the unmixed expected spelling.

---

## evidence-combine-surface

**Question:** What public surface combines value-level failure evidence for
the accumulating expected applicative object?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** There is no generic public `combine_errors`.
The shipped error-set model exposes its native `error_set_combine` operation.
Framework-facing evidence dispatch stays private as
`detail::combine_grade_evidence`, with a left-biased same-type default and
model-specific overloads where needed.
**Why:** Evidence combination is value-level model vocabulary, not a generic
grade-algebra operation. A public generic name would either encode the
shipped model's assumptions as framework API or promise a cross-model
semigroup the grade concept does not require.
**Log:**
- 2026-08-30 — Raised by the grading-fidelity review.
  Stage accumulating-object introduced public `combine_errors`, but the later
  model-dispatched repair moved the only framework fold to
  `detail::combine_grade_evidence`, leaving `combine_errors` and its support
  trait `is_error_set_of_v` as caller-less residue. They were removed; the
  intentional public model-native operation remains `error_set_combine`.

---

## graded-context-role

**Question:** Is `graded_context` the public spelling of
[grading-footprint](#grading-footprint)'s graded/ungraded exclusivity
constraint?
**Status:** DECIDED 2026-08-30
**Decided by:** Steve Downey.
**Decision:** Yes.
`graded_context<C>` means `grade_of_t<C>` is a model grade rather than the
framework's ungraded sentinel. It is public semantic vocabulary, and shipped
mixing-boundary constraints use it to distinguish true ungraded operands from
foreign-model carriers.
**Why:** Structural checks such as "not `std::expected`" or "contains
`error_set`" do not survive multiple grade models. `graded_context` is the
single framework question that matches the sentinel/model split decided by
[bottom-grade-identity](#bottom-grade-identity).
**Log:**
- 2026-08-30 — Raised by the grading-fidelity review after `graded_context`
  was public and tested but unused in shipped headers. The expected
  same-error-plus-bare constraints now make it load-bearing: plain `int`
  remains bare, `expected<T, E>` keeps the declared-error lazy spelling path,
  and `Fallible<T, may_fail>` is rejected at the expected cross-model
  boundary.

---

## wording-generation

**Question:** Where does P3200's normative wording come from, and how does it
reach the paper?
**Status:** DECIDED 2026-09-03
**Decision:** The shipping headers are the wording source.
`beman.specgen` reads each in-scope header on its own and renders mpark/wg21
markdown fragments into `papers/wording/`; `D3200R0.md` splices them with a
per-paper pandoc filter, `papers/filters/transclude.py`, selected by
`papers/defaults.yaml`.
`make wording` regenerates, `make wording-check` is the drift gate, and the
fragments are checked in so the paper builds without specgen installed.
**Why:** Hand-written wording drifts from the reference implementation it
claims to specify, and the drift is invisible until review. Generating it
means the paper cannot describe an operation the code does not have.
Splicing through a per-paper filter keeps the MPark.WG21 subtree unpatched, so
`git subtree pull` costs nothing.
**Consequences:** specgen reads only the main file's declaration/comment
interleave, so each header contributes its own synopsis subclause rather than
there being one `[transpose.syn]`. Marked-up headers declare members in-class
and define them out of line, so that a member's wording is placed by the
definition's lexical position.
**Log:**
- 2026-09-03 — Decided, and the pipeline built for the eight in-scope headers.
- 2026-09-04 — Regenerated against a specgen carrying eight fixes
  (steve-downey/specgen#1 and #3--#8). The generated wording had been wrong in
  ways the old tool could not see, and the checked-in fragments carried every
  one of them: a dozen documented namespace-scope aliases, variable templates
  and concepts rendered as *empty* code blocks with their descriptions
  discarded (#1); `\expos` on a namespace-scope alias was inert, so
  `traverse_context_t` published its underscore spelling rather than
  `traverse-context-t` (#1 follow-up); `\seebelow` on `recover` was inert
  against an explicit trailing return type (#6); and `grade_lifted_into_model`
  published a stray `public:` above a `type` naming an elided private alias,
  which would not compile if copied (#7). Four error-severity findings and
  four synopsis defects were fixed in the markup: the `detail` machinery each
  header declares is now `\omit`ted rather than published as specification
  (#3 made the leak visible by reporting a qualifier wherever its namespace is
  declared, not only in the main file), `error_set`'s canonicalizing
  right-hand side is masked with `\seebelow`, `applicative_eval` gained a
  namespace-scope spelling under
  [wording-visible-internals](#wording-visible-internals), and two
  `// NOLINT` comments stopped rendering into the published `error_set_of`
  synopsis. Two rules this decision imposed are relaxed: an in-class body is
  now spliced out of the synopsis whichever entities it names (#5), and a
  `\rSec` title may wrap across `//` continuation lines (#8).
- 2026-09-04 — Raised while regenerating and filed as
  steve-downey/specgen#18: a documented class or class-template *definition*'s
  own description elements were silently discarded. The `\remarks` on
  `unit_grade` and on `grade_of`, and the `\mandates` and `\remarks` on
  `error_set_of`, appeared in no generated fragment, and the authored
  `\mandates` did not replace the derived one. The silent-drop family #1
  closed for namespace-scope entities, which the #1 backstop missed because a
  definition does produce a synopsis.
- 2026-09-05 — Regenerated against a specgen carrying seven further fixes
  (steve-downey/specgen#18, #20--#24 and #31). #18 is the one that moved this
  paper: all four dropped descriptions now render, and `error_set_of`'s
  authored *Mandates* replaces the derived paragraph, so the canonical-pack
  requirement the class exists to enforce is finally stated in the terms it was
  written in. Nothing else in the wording moved, and no new finding appeared.
  Four of the remaining six fixes address shapes this markup does not have --
  transpose spells every constraint as a trailing requires-clause rather than
  in the template head (#20), defines no member in class (#21), declares no
  deduction guide (#22), and marks no class template `\expos` (#23) -- and #31
  concerns gathered regions. #24 is the one with leverage: bare `\seebelow`
  now masks a namespace-scope variable's type, so `applicative_eval` is
  described as `inline constexpr unspecified applicative_eval;` rather than
  `\omit`ted, and a name the synopsis puts in front of a reader is one the
  reader can now look up. Masking a *reference*-typed variable renders a stray
  `const` and loses a space (steve-downey/specgen#33), so it is declared as a
  forwarding object rather than a forwarding reference.
- 2026-09-06 — Regenerated against a specgen carrying seven more fixes
  (steve-downey/specgen#33--#41 and #47's qualifier normalization). The one
  that matters is #36: `\expos` now reaches a declaration in an included
  header, which reverses
  [wording-visible-internals](#wording-visible-internals) and removes ten
  namespace-scope spellings the pipeline had added to the library. #33 fixed
  the reference mask filed from here, though `applicative_eval` stays a
  forwarding object: it reads as the same thing and there is no reason to churn
  it back. #47 moved nothing, these headers being west-const throughout. Two
  new gaps went the other way and are filed rather than worked around:
  steve-downey/specgen#49, a partial specialization of an `\expos` primary
  rendering its raw name -- visible in `transpose.errset.recover.md`, where
  `is_expected_v`'s specialization sits unrenamed under its own
  `$is-expected-v$` primary -- and steve-downey/specgen#50, `\expos` and
  `\seebelow` not composing on an alias or a concept, which is what would end
  the three exposition-only chains that still name an undeclared helper.
- 2026-09-06 — Rechecked against the next beta (fba1271). Its one functional
  change is steve-downey/specgen#45, which concerns gathered regions this
  pipeline does not use, and the generated wording is byte-identical. Two
  defects found by reading the output rather than by the validator, both filed
  and neither worked around: steve-downey/specgen#56, a class template's
  parameter list keeping `std::` in the synopsis while the next line of the
  same code block drops it -- visible in `transpose.array.syn.md` against
  `transpose.array.tuple.md` -- and steve-downey/specgen#57, the `--paper`
  paragraph-number placeholders restarting at `x` in every wording block, which
  gives `[transpose.grade.syn]` nine paragraphs all numbered `x`. Spelling
  `size_t` unqualified in `array.hpp` would silence the first, and that is the
  edit [wording-visible-internals](#wording-visible-internals) now refuses to
  make: the specification does not get to change the library to suit itself.

- 2026-09-06 — Regenerated against the beta that closes every issue filed from
  here (steve-downey/specgen#48--#50, #56, #57). All four defects this paper
  carried are gone: the `[x]` placeholders now run ascending across a whole
  subclause, so `[transpose.grade.syn]`'s nine paragraphs are `x` through
  `x+8` rather than nine `x`s; `ArrayApplicativeImpl`'s head reads
  `template<class T, size_t N>`; and `is_expected_v`'s specialization renders
  under its primary's exposition name.
  The markup adapts to two of them rather than only benefiting. #49 lets a
  specialization follow its `\expos` primary, so the two-case helpers
  `carrier_value` and `mixes_with_model_impl` are exposed whole instead of
  `\omit`ted whole -- an exposition-only primary and the specialization
  carrying the real case, which is what such a helper looks like in the draft.
  #50 lets `\seebelow` mask an alias's definition under `\expos`, which ends
  the one chain that does not terminate usefully: `mixed_result_t` now renders
  `= see below` rather than opening four levels of grade-algebra plumbing to
  say one thing about a return type. **No raw implementation name reaches the
  wording any more**, and every exposition-only name in the document is
  declared in it -- the residue this log recorded on 2026-09-05 is closed.
  One cosmetic defect remains, filed as steve-downey/specgen#67: an
  exposition-only rename does not re-flow a continuation line, so the second
  line of `mixes-with-model`'s definition sits nine columns short of its
  arguments.

---

## wording-visible-internals

**Question:** specgen refuses to render a declaration that carries a
`beman::transpose::detail` qualifier. What gives way -- the header layout, or
the wording?
**Status:** DECIDED 2026-09-03; reversed 2026-09-05
**Decision:** The wording. A `detail` entity that appears in a *declaration*
the specification shows -- a constraint, a template-parameter default, or a
trailing return type -- is marked `\expos` where it is defined, and its uses
render as the exposition-only spelling. The header layout is unchanged and the
namespace is not widened. Machinery below an exposition-only entity is
`\omit`ted. Bodies keep their `detail` spellings, because a body specgen does
not render cannot leak.
The original answer was the header layout, and stood while specgen could not
read a marker on an entity it was not specifying; the log records the reversal.
**Why:** A generated specification should not be able to require an edit to
the thing it specifies. Every forwarding spelling was a public name the library
did not want, added to satisfy a tool, and the names were real: they changed
lookup and ADL, and nothing outside the headers ever used them. Marking the
entity where it lives says the same thing to the reader -- the standard has no
`detail` namespace, so a declaration that names one is not specification text
-- and says it without moving anything.
**Consequences:** the following are `\expos` where they are defined, in
`beman::transpose::detail` unless noted. Their uses render as the
exposition-only spelling and no public name is added:
`is_expected_with_error_v`, `bind_result_t`, `all_declare_v`,
`declares_or_bare_v`, `all_declare_or_bare_v`, `carrier_value_t`
(`expected.hpp`); `mixes_with_model`, `mixed_result_t` (`grade.hpp`);
`error_set_is_canonical_v`, `error_set_names_distinct_v`, `recover_return_t`
(`error_set.hpp`); and `applicative_eval`, which stays in `beman::transpose`
under `\seebelow` because the wording describes it rather than only naming it
(`apply.hpp`). `traverse_context_t`, `is_expected_v` and `error_set_has_v` stay
where they are: the first two because the wording shows their declarations, and
`error_set_has_v` because it also answers a language problem -- one spelling of
the membership constraint usable both in class and on an out-of-line
definition, where class scope is not in effect.
Three exposition-only definitions name an entity the document does not declare:
`carrier_value_t` names `carrier_value`, `mixes_with_model` names
`mixes_with_model_impl`, and `mixed_result_t` names `mixed_grade_t`. Each is a
helper whose own definition would drag in more machinery than it settles, and
the marker that would end the chain -- `\seebelow` on an exposition-only alias
-- does not compose yet (steve-downey/specgen#50).
**Log:**
- 2026-09-03 — Raised while marking up `traverse.hpp`. `\verbatim-itemdecl`
  was tried first and emits *both* the authored declaration and the parsed
  one, which is a specgen defect (steve-downey/specgen#4, reproducible in ~15
  lines). `\verbatim-synopsis` has the same defect, and that is the one that
  would have mattered: most of the leakage errors here were on the class
  synopsis, which `\verbatim-itemdecl` does not reach. `\expos` does not
  remove a `detail` qualifier, since the entity is still in `detail`, and
  `\seebelow` masks a return type rather than a constraint, so the entity
  itself has to move or be forwarded. A working `\verbatim-synopsis` plus an
  authored `\constraints` would have avoided every promotion listed above.
- 2026-09-04 — The premise has been superseded and the decision is retained
  anyway. Both verbatim markers now replace the extracted declaration instead
  of duplicating it (steve-downey/specgen#4), and bare `\seebelow` now reaches
  an explicit trailing return type (#6), which alone would cover
  `recover_return_t`, `carrier_value_t`, `bind_result_t` and `mixed_result_t`.
  So the trigger this entry names has fired: masking a constraint in a
  synopsis is now possible. It is not being taken, because the way to take it
  is `\verbatim-synopsis`, and an authored synopsis is a hand-maintained one —
  reintroducing, for exactly the classes most worth generating, the drift
  [wording-generation](#wording-generation) exists to prevent. `\seebelow` is
  being taken where it masks rather than replaces: `error_set`'s
  canonicalizing right-hand side, and `recover`'s trailing return type, whose
  marker had been inert since it was written. Unwinding the remaining
  promotions is a live option and a separate change; the count grew by one
  first, because `applicative_eval` was leaking through a hole in the leakage
  checker (#3) rather than being caught and forwarded like its siblings.
- 2026-09-05 — REVERSED. steve-downey/specgen#36 makes `\expos` reach a
  declaration in an included header, so the marker can be written on the
  `detail` entity itself and its uses render as the exposition-only spelling
  with no qualifier and no finding. That removes the whole reason the header
  had to give way. Ten forwarding spellings are gone -- six deleted from
  `expected.hpp`, three moved back into `detail` in `error_set.hpp`, and
  `carrier_value_t` -- with the uses re-qualified to `detail::`, which is where
  they were before the pipeline existed. Nothing outside the headers referenced
  any of them: the one test that touches this machinery
  (`laws.test.cpp:274-286`) already spelled it `detail::mixes_with_model` and
  `detail::mixed_result_t`. The wording is better for it, not merely no worse:
  a constraint that used to name a bare `all_declare_v` now names
  `$all-declare-v$`, which the same document declares as exposition-only. The
  issue's own commit message names this project's #3 workaround as the thing
  that put the entities out of reach, so the reversal is the report closing its
  own loop.
- 2026-09-05 — One promotion stops being a bare `\omit`. Bare `\seebelow` now
  masks a namespace-scope variable's declared type (steve-downey/specgen#24),
  so `applicative_eval` is described as `inline constexpr unspecified
  applicative_eval;` instead of being omitted. That is a better answer than
  either horn of this question: the synopsis shows the name in `ap`'s
  constraint, and the reader can now find out what it is without the wording
  claiming a type for it. The rest of the list is unchanged -- masking a
  variable does not help a trait whose *value* is the point, and
  `error_set_is_canonical_v` rendered as `unspecified` would be worse than
  omitted.
- 2026-09-08 — `probe_witness` and `probe_witness2` were placed in the main
  namespace by `typeclass-object-concepts` and reached
  `transpose.applicative.syn.md` and `transpose.traversable.syn.md` undeclared:
  they are implementation machinery -- witness callables that let a concept
  check a derived operation's existence for one representative callable,
  never a proof it holds for every callable -- and nothing outside the
  concept definitions ever named them. They now live in `detail` and are
  marked `\expos` where they are defined, and their uses -- inside the
  `applicative_impl`/`applicative_object`/`traversable_impl`/
  `traversable_object` concept bodies themselves, not merely a declaration
  that names them -- render as `$probe-witness$`/`$probe-witness2$` with no
  qualifier and no leakage finding. That the `\expos` marker composes on a
  concept's own requires-expression, not only on a trailing return type or a
  constraint, was the open question this step existed to answer; it does.
  `applicative_value_t` was deliberately left alone as public vocabulary the
  baseline audit lists and `examples/binary_tree.hpp` uses -- its wording
  problem is the pipeline's, not the library's, and the specification does
  not get to change the library to suit itself. Filed upstream as
  steve-downey/specgen#84 (`--validate` should catch a name that appears in
  generated wording but is declared nowhere in it).

---

## wording-spec-names

**Question:** Are the reference implementation's spellings the ones the
wording should use?
**Status:** OPEN
**Decision:** Deferred for R0.
`Applicative`, `Traversable`, `applicative_typeclass`,
`OptionalApplicativeMap` and their siblings are implementation names; WG21
wording is lowercase and would not name a CRTP base this way. The explicit
object parameter (`this auto&& self`) likewise appears in every generated item
declaration and would not appear in a specification.
**Why:** The naming discussion is a paper-level argument, not a markup
question, and settling it before the mechanism has been reviewed would spend
the discussion twice. D3200R0 says so in the wording preamble rather than
shipping the spellings as if they were settled.
**Log:**
- 2026-09-03 — Recorded when the wording pipeline landed.

---

## monoid-carrier-canonicity

**Question:** Which carriers get a default `Monoid<T>` registration, and
which are named instead?
**Status:** DECIDED 2026-09-07
**Decided by:** Steve Downey, in the typeclass-relationships design
discussion of 2026-09-07, recorded in
`docs/coordination-worklist-2026-09-07.md`.
**Decision:** A raw carrier gets a default `Monoid<T>` registration only where
one instance is canonical: string and vector concatenation, and `Count`.
Where no instance is so right that any one of them can be the default —
numbers, booleans — the choice is spelled by a named carrier type instead of
a registration on the bare type.
**Why:** Addition and multiplication are both monoids on a number; maximum
and minimum are both monoids on any ordered type; a `bool` carries both
conjunction and disjunction. Registering one of these as `Monoid<int>` or
`Monoid<bool>` makes the library pick for the caller, silently, at every
`fold_map` whose function happens to return that type. Naming the carrier —
`Sum<int>`, `Max<int>`, `Any` — makes the choice something the caller writes
down instead.
**Log:**
- 2026-09-07 — [named-monoid-carriers](../tmp/plan/step-named-monoid-carriers.md)
  added the six named carriers (`Sum`, `Product`, `Max`, `Min`, `Any`, `All`)
  to `monoid.hpp`, and moved `fold.hpp`'s `detail::Any`/`detail::All` onto the
  new public `Any`/`All` so `any_of`/`all_of` exercise them. The carrier set
  is provisional: six because these are what the fold family and the
  worklist named; a consumer needing a seventh (`First`, `Last`, `Endo`)
  extends the set rather than working around it. The bare-numeric
  `Monoid<int|long|size_t>` registrations are still present; removing them is
  the following step,
  [numeric-monoid-defaults](../tmp/plan/step-numeric-monoid-defaults.md).
- 2026-09-07 — [numeric-monoid-defaults](../tmp/plan/step-numeric-monoid-defaults.md)
  removed the three bare-numeric `Monoid<int>`, `Monoid<long>`, and
  `Monoid<std::size_t>` registrations from `monoid.hpp`. The only in-tree
  consumer was `monoid.test.cpp`'s additive-int `TEST_CASE`, migrated to
  `Sum<int>`. The sentinel is a `has_monoid` concept with
  `static_assert(!has_monoid<int>)` (and `long`, `std::size_t`), so a future
  re-registration fails a test instead of silently restoring the old default.
- 2026-09-07 —
  [monad-induced-monoids](../tmp/plan/step-monad-induced-monoids.md) added
  two more induced, named, unregistered carriers, in their own header
  (`induced_monoid.hpp`, not `monoid.hpp`, which knows about no typeclass
  instances): `KleisliEndo<MONAD_OBJECT, A>`, the Kleisli endomorphism
  monoid on `A -> M<A>` arrows (identity `pure`, combine `>=>`), and
  `LiftedMonoid<APPLICATIVE_OBJECT, CONTEXT>`, `F<A>` lifted from a
  `Monoid<A>` through an applicative object (identity `pure(identity_A)`,
  combine `invoke(combine_A, ·, ·)`). Both are registered only on their own
  carrier type; `std::optional<int>` and `std::optional<Sum<int>>` gain no
  `Monoid`. Worth not rediscovering: the categorical statement "a monad is a
  monoid in the category of endofunctors" is inexpressible at `Monoid<T>` —
  its tensor is composition, not product, and its carrier is a type
  constructor, not a type — and `KleisliEndo` is the value-level statement
  that survives that gap, not the categorical statement itself. The Kleisli
  carrier erases its arrow through `std::function`, following
  `detail::LeftFoldProgram` (`fold.hpp`): a monoid's `combine` must return
  what it takes, and a bare lambda type does not close under composition.
  `Monad<Impl>::kleisli`'s derived branch returns a closure that captures
  `self` by reference, so `combine` cannot store the result of
  `MONAD_OBJECT{}.kleisli(...)` directly; it instead stores a lambda that
  constructs its own `MONAD_OBJECT{}` and calls `kleisli` on it in the same
  full expression that invokes the result, keeping the temporary alive for
  exactly as long as it is used.

## impl-access-through-bases

**Question:** How does a derived member of a typeclass base (`Functor`,
`Applicative`, `Monad`, `Foldable`, `Traversable`) address its `Impl`, given
that `self` is a deducing-this parameter typed as the most-derived class —
which may be a `Map`, a user class, or another typeclass object wrapping this
one?
**Status:** DECIDED 2026-09-07
**Decided by:** Steve, for the half that keeps `protected`: the published
wording is not changed as a side effect of an implementation convenience.
`struct Applicative : protected Impl` and `struct Traversable : protected
Impl` are published wording, appearing verbatim in
`papers/wording/transpose.applicative.syn.md` and
`transpose.traversable.syn.md`, generated from these headers, so the
inheritance keyword itself is out of scope for any refactor. The mechanism
that reaches `Impl` under that constraint is decided by this step, from
compile evidence gathered in the amendment consult's throwaway worktree.
**Decision:** Three parts, all needed by later steps:
- The five bases keep `protected Impl`. Not renegotiable by a later step:
  `Applicative` and `Traversable`'s class heads are published, and changing
  the inheritance changes what D3200R0 proposes.
- Addressing goes through **`impl_of(self)`, a private member of each
  base**, with the const rule shared as `impl_ref_t` in
  `include/beman/transpose/detail/typeclass_base.hpp`. A namespace-scope
  helper cannot perform the conversion, because it is a member or friend of
  nothing, and under protected inheritance only a member of the base that
  owns the `Impl` sub-object may cast to it.
- **A base exposes an operation as its own member, never as a `using
  Impl::op;` re-export, wherever that base may itself be wrapped.** This is
  the half that keeps a two-deep composition (`Functor<SomeMonadMap>`, where
  `SomeMonadMap` is itself built from `Monad<Impl>`) reachable: an external
  call enters the outer base's own member, which hands the inner base's
  member a `self` typed as the `Map` — one level deep, where addressing
  already works. If a later step turns a probing member back into a
  re-export, the deep chain re-forms and the inner base's `impl_of` sees a
  `self` typed as the *outer* wrapper instead of the `Map`, and the cast
  fails: `'X' is an inaccessible base of 'Functor<SomeMap>'`, raised from
  inside the inner base's member.
**Why:** Measured, with the inheritance untouched:
- A namespace-scope helper performing `static_cast<Impl &>(self)` fails
  access control even one level deep — it is a member or friend of nothing,
  and protected base-class accessibility has no clause that walks upward
  into it.
- A member of an *inner* base cannot touch an *outer* object's inherited
  surface at all: `Monad<Impl>` is a base of `Functor<Map>`, not a friend or
  member of it, and base-class accessibility does not walk upward.
- A `using Impl::op;` re-export at the wrapping base fails the two-deep case
  specifically, with `'Probe' is an inaccessible base of 'Outer<InnerMap>'`
  (reproduced directly in
  `tests/beman/transpose/detail/typeclass_base.test.cpp`'s two-deep
  `TEST_CASE`s, which use the own-member shape and pass; the re-export
  failure itself is documented here rather than compiled, since it is a hard
  error rather than a SFINAE-friendly one).
- The full suite is green and unchanged after converting
  `Applicative<Impl>::invoke`, `Applicative<Impl>::ap`, and
  `Monad<Impl>::invoke` from the open-coded `SELF`/`IMPL_BASE` cast to
  `impl_of(self)` — same probes, same derivations, same passing count
  (119, then 124 with this step's five added `TEST_CASE`s exercising the
  mechanism directly).
- The generated wording is byte-identical: `scripts/gen-wording.sh` produced
  a diff against only the four fragments already known to drift for
  unrelated reasons (`transpose.errset.recover.md`,
  `transpose.errset.syn.md`, `transpose.expected.syn.md`,
  `transpose.grade.syn.md`); neither `transpose.applicative.syn.md` nor
  `transpose.traversable.syn.md` appears. `impl_of` is `//! \omit`-marked and
  in a trailing private section in `apply.hpp` and `traverse.hpp`, which
  renders as nothing.

A trailing *requires-clause* is not a complete-class context, so `impl_of`
cannot be named there: `Applicative<Impl>::ap`'s declaration-level
disjunctive `requires`-clause keeps its existing `requires(const Impl &impl)
{ impl.op(...); }` form rather than being rewritten to name `impl_of`.
Member *bodies* are complete-class contexts and use `impl_of(self)` freely;
this split — declaration clauses one way, bodies another — is the shape
every later probing member copies.

**Provisional:** the cast is repeated in five classes rather than shared.
What would justify sharing it is a language change that lets a non-member
perform the conversion, or a base gaining genuinely private state, which no
typeclass object has today — every one of them is stateless and empty. What
is not repeated is the const propagation, which lives once in `impl_ref_t`.
**Log:**
- 2026-09-07 — [impl-probe-helper](../tmp/plan/step-impl-probe-helper.md)
  added `impl_ref_t` to `detail/typeclass_base.hpp` and a private `impl_of`
  to each of `Functor`, `Applicative`, `Monad`, `Foldable`, `Traversable`,
  then converted the three existing native-`Impl` probes
  (`Applicative<Impl>::invoke`, `Applicative<Impl>::ap`,
  `Monad<Impl>::invoke`) to call through it. Pure refactor: no other member
  gained a probe, no `: protected Impl` changed, no `using Impl::op;`
  re-export changed. The own-member rule (this decision's third part) is
  applied to a real member for the first time by
  [monad-fmap-basis](../tmp/plan/step-monad-fmap-basis.md), the next step.

## functor-monad-grounding

**Question:** How does a type with a `Monad` instance and no separate
`Functor` registration obtain its `fmap`, given that `fmap f x == bind(x,
pure . f)` is a theorem rather than a coincidence?
**Status:** DECIDED 2026-09-07
**Decided by:** the design, applied by
[monad-fmap-basis](../tmp/plan/step-monad-fmap-basis.md).
**Decision:** Structurally, not by a superclass constraint. `Monad<Impl>`
provides the Functor **basis** operation, `fmap`, derived as `bind(ma, pure .
f)` with native preference for an `Impl::fmap` when the instance supplies
one. The full Functor instance is then spelled at the registration or call
site as `Functor<SomeMonadMap>{}` — no adapter type, no nominal wrapper
requirement. The CRTP base is the adapter.

**Monad grows only the basis operations of the classes it can ground, never
their derived operations.** `replace` stays on `Functor<>` and is reached
through the layered instance, not duplicated onto `Monad`. This is what
keeps a monad-grounded instance tracking Functor's derived surface instead
of forking from it as that surface gains operations.

**Why:** Conformance in this library is structural throughout — provide the
operations, claim the laws at the registration site — so a superclass
constraint would be foreign to the rest of the design where a theorem
suffices. Growing only the basis, and never the derived surface, is the rule
that keeps the two typeclasses from drifting: if `Monad` also grew `replace`
directly, a change to `Functor::replace`'s derivation would have to be
mirrored onto `Monad` by hand, and nothing would catch a missed mirror. The
agreement test in `laws.test.cpp` is the sentinel: where a hand-written
`Functor` and a `Monad` coexist on the same carrier (`std::optional`), the
hand-written `fmap` and the Monad-derived one are checked equal, so the
redundancy cannot drift silently.

`fmap`'s `requires`-clause is a disjunction — native `Impl::fmap`, or
`Impl::bind` — spelled in the `Applicative<Impl>::ap` shape from
`apply.hpp`, for the same reason: an unconstrained probing member is
satisfied by substitution alone, so a later deep object concept checking for
`fmap` would find it vacuously present even when neither `Impl` operation
exists. Both branches address `Impl` directly, never `self` — `fmap` and
`bind` are one of the mutually-derivable pairs (`bind` is recoverable from
`join` + `fmap`) that `apply.hpp`'s cycle discipline exists to keep from
recursing into each other, and generalizing Impl-direction to other members
is out of scope for this decision.
**Log:**
- 2026-09-07 — [monad-fmap-basis](../tmp/plan/step-monad-fmap-basis.md) added
  `fmap` to `Monad<Impl>` and converted `Functor<Impl>::fmap` from a `using
  Impl::fmap;` re-export to its own forwarding member — the own-member rule
  from [impl-access-through-bases](#impl-access-through-bases), applied to a
  real member for the first time. That conversion is what makes
  `Functor<OptionalMonadMap<int>>` compile at all: with the re-export, an
  external call landed directly in `Monad<Impl>::fmap` with `self` typed as
  the outer `Functor<...>`, where the protected-inherited `Impl` is
  unreachable (`'OptionalMonadImpl<int>' is an inaccessible base of
  'Functor<OptionalMonadMap<int>>'`); with `Functor` owning the member, the
  call enters `Functor::fmap` first, where the conversion is accessible, and
  hands `Monad<Impl>::fmap` a `self` typed as the `Map` — one level deep,
  where addressing already works. Surfaced along the way: `OptionalMonadImpl
  ::bind` computed its result type only in the body (bare `auto`, `using
  Result = ...std::invoke_result_t<F, const A &>...`), which is exactly the
  non-SFINAE-friendly shape `apply.hpp`'s basis invariant warns against —
  probing it from `fmap`'s disjunctive `requires`-clause with an
  incompatible `F` produced a hard error rather than a clean constraint
  failure. Given a trailing return type (`-> remove_cvref_t<
  std::invoke_result_t<F, const A &>>`), the probe fails cleanly, matching
  the invariant `apply.hpp` already states for `Applicative` impls. Suite
  went from 124 to 131.
- 2026-09-07 — [as-functor-presentation](../tmp/plan/step-as-functor-presentation.md)
  added `Monad<Impl>::as_functor()`, the free presentation member this
  section's decision text forward-pointed to: `Functor<remove_cvref_t<
  decltype(self)>>{}`, constructed fresh at the call site rather than looked
  up. It deliberately does not consult `functor_typeclass<T>` — that
  registered object may be an unrelated optimized instance, and the coherent
  functor here is the one derived from the monad object in hand,
  law-compatible with its `bind` by construction. It is free because every
  typeclass object is stateless and empty (asserted with
  `std::is_empty_v`), and it is optimization-preserving because the derived
  members it exposes already probe `Impl` first: the sentinel test presents
  a monad object whose own `Impl`-equivalent (the Map itself, since
  `Functor<ThisMap>`'s `Impl` is the Map) supplies a native `replace`
  returning a doubled-replacement marker, and asserts `as_functor().replace`
  returns the marker, not the `fmap`-derived answer. Suite went from 154 to
  159.

## derived-op-native-preference

**Question:** How does a derived operation on a typeclass base find a
better native implementation, and what may it address?
**Status:** DECIDED 2026-09-07
**Decided by:** the design, applied by
[functor-monad-derived-probing](../tmp/plan/step-functor-monad-derived-probing.md).
**Decision:** Two parts.
1. Every derived member probes `Impl` for a native version and forwards to
   it when present, deriving otherwise, and carries a disjunctive
   `requires`-clause: native present, or the derivation's own requirement on
   the basis holds.
2. **A derived member's second alternative must name the same expression
   its body's derivation branch evaluates — same receiver, same spelling.**
   An alternative may address `Impl` only if the operation it names is one
   `Impl` is required to supply directly: a hard requirement, or the other
   half of a mutually-derivable pair — `invoke`/`ap`, `fold_map`/
   `fold_right`, `bind`/`join`+`fmap`. Anything the base can *synthesize* is
   addressed through `self`.

`Functor::replace`, `Monad::join`, `Monad::kleisli` and `Monad::ap` all
adopted the shape. `join`/`bind` is one of the mutually-derivable pairs, so
its derivation addresses `impl_of(self).bind(...)` directly, matching
`fmap`'s bind-basis branch; `replace`, `kleisli` and `ap`'s bind + pure
derivation are one-way and keep routing through `self`. `subsume` and
`bind_with` were deliberately left unprobed: `subsume` is grade machinery
defaulted from the algebra, not a derivation over `Impl`, and `bind_with` is
explicit delegation to an object the caller passed — neither has an `Impl`
operation to prefer.

**Not every member has an expressible second alternative.** `kleisli`
returns a closure whose body calls `bind` on an argument type ("a") that is
not known until the closure is invoked, so there is no concrete expression
to probe at `kleisli`'s own instantiation. `bind`'s existence is already
guaranteed unconditionally by `Monad<Impl>`'s class invariant (the
`static_assert` plus `using Impl::bind;` at the top of the class fail the
whole class before `kleisli`'s clause is ever reached if `Impl` lacks
`bind`), so `kleisli`'s second alternative is spelled `true` rather than a
tautological re-check. `invoke`'s second alternative could not name
`FUNCTION` directly either — its arity does not match `bind`'s
single-argument callback shape, since the derivation applies it only after
unwinding nested `bind` calls — so it probes `impl.bind(first, <a
single-argument passthrough lambda>)` instead, checking that `FIRST` is
bind-compatible without asserting anything about `FUNCTION`.

**Why:** The constraint keeps the deep object concept a later step
introduces from being satisfied vacuously by substitution alone, and
restores "missing basis" as a clean constraint failure. The pairs-only rule
keeps `Map`-level shadows at exactly their current reach — broad
Impl-direction would let external calls hit a shadow while internal
derivations skipped it. Hand-rolled objects never touch the bases and are
unaffected.

**Record as provisional:** which pairs are mutually derivable is a fact
about the current basis sets, and admitting an alternate Monad basis would
add one.
**Log:**
- 2026-09-07 —
  [functor-monad-derived-probing](../tmp/plan/step-functor-monad-derived-probing.md)
  converted `Functor::replace` and `Monad::join`/`kleisli`/`ap`/`invoke` to
  the probe-then-derive shape and added preference tests, including one
  per-instantiation test (`Monad::join` native for
  `std::optional<std::optional<int>>` only, falling back for
  `std::optional<std::optional<std::string>>` on the same Map) exercising
  the property a Map-level `using`-declaration could never express. No
  `Map`'s `using`-declarations were removed. Suite went from 131 to 135.
- 2026-09-07 —
  [applicative-derived-probing](../tmp/plan/step-applicative-derived-probing.md)
  converted `Applicative`'s five derived members — `map`, `lift`,
  `zip_with`, `discard_first`, `discard_second` — to the probe-then-derive
  shape, all one-way (`Applicative` has no mutually-derivable pair besides
  `invoke`/`ap`, already converted). This is the first conversion to reach a
  wording-generating header: the `\effects-equiv` markup on all five, which
  renders the body, had to become explicit `\constraints`/`\effects`/
  `\returns` prose in the shape `ap` already uses, since a probing body is
  not specification text. `papers/wording/transpose.applicative.derived.md`
  and `transpose.applicative.syn.md` were regenerated and copied over; the
  five pre-existing drifted fragments (`transpose.errset.obs.md`,
  `transpose.errset.recover.md`, `transpose.errset.syn.md`,
  `transpose.expected.syn.md`, `transpose.grade.syn.md` — one more than this
  step's own file expected, `errset.obs.md` having drifted since) were left
  untouched. `discard_first` and `discard_second` needed a second alternative
  that names a callable, and a lambda-expression cannot be spelled
  identically at both an in-class declaration and its out-of-line
  definition — each occurrence is a distinct, unrelated closure type, so the
  two declarations stop matching. `apply.hpp` gained two small named
  evaluator objects, `discard_first_eval` and `discard_second_eval`, in a
  `\omit`ted nested `detail` namespace, the same role `applicative_eval`
  already plays for `ap`. Traversable's derived members (`for_each`,
  `transpose`, `traverse_with`, `transpose_with`) were deliberately left
  unconverted per this step's file: two are delegation to a caller-supplied
  object rather than derivations over `Impl`, and `traverse.hpp` carries a
  DELIBERATE CONSTRAINT comment that makes widening it a decision rather
  than a chore. Suite went from 135 to 146 (11 added, all in
  `apply.test.cpp`: preference and fallback tests for `map`, `zip_with`,
  `lift`, `discard_first`, `discard_second`, plus one per-instantiation test
  for `map`).
- 2026-09-07 —
  [foldable-derived-probing](../tmp/plan/step-foldable-derived-probing.md)
  converted `Foldable`'s whole surface — `fold_map`/`fold_right` (the third
  mutually-derivable pair) plus the nine one-way derived members (`length`,
  `fold_left`, `combine_all`, `fold`, `any_of`, `all_of`, `empty`,
  `to_vector`, `find_first`) — to the probe-then-derive shape, closing the
  latent `fold_map`/`fold_right` mutual-recursion hazard structurally:
  before this step, a `Map` that omitted a `using`-declaration for one side
  of the pair sent the two derivations into unbounded mutual template
  recursion (each round nesting another `RightFoldProgram`); both
  derivations now address `Impl` directly (`impl_of(self)`), so neither can
  re-enter the other through `self`, and an `Impl` with neither basis fails
  as a clean constraint (its `fold_map`/`fold_right` members do not exist,
  verified with a `has_fold_map` concept the way the library's other
  "does not exist" cases are tested), with a `static_assert` naming both
  acceptable bases as a last-resort message that is unreachable given the
  declaration-level constraint already did the work. `VectorFoldableImpl`
  gained a native `length` (`values.size()`) with **no** edit to
  `VectorFoldableMap` — the ergonomics demonstration this step exists to
  make. One correction to the one-way members' shape, found empirically: the
  seven members deriving directly from `fold_map` (`length`, `fold_left`,
  `combine_all`, `any_of`, `all_of`, `to_vector`, `find_first`) must probe
  `self.fold_map(...)` in their second alternative, not `impl.fold_map(...)`
  — since `fold_map` is now itself conditionally available via either basis,
  the Impl-level shallow check the rest of the codebase uses for one-way
  derivations (`map`, `zip_with`, ... in `apply.hpp`) wrongly excludes an
  `Impl` that only provides `fold_right` + `element_type`; a hazard-Impl test
  calling `to_vector` caught this directly (compiled-away member, not a
  runtime failure). `fold`/`empty`, which derive from `combine_all`/`any_of`
  rather than `fold_map` directly, already needed this `self`-routed shape
  per this step's own file. Two lambda-related GCC/Clang portability notes
  worth carrying forward: a lambda that captures a declaration-level
  parameter (`function`, `predicate`) inside a trailing requires-clause
  fails under GCC (`-Wtemplate-body`, "use of parameter outside function
  body"), and a lambda that captures a local variable inside a `requires{}`
  used in a `static_assert` body fails under the Clang front end the wording
  generator uses ("variable cannot be implicitly captured" / "reference to
  local variable declared in enclosing function") even though GCC accepts
  it; both are fixed the same way, with a non-capturing marker lambda whose
  return-type shape is all the probe needs. `fold.hpp` is not
  wording-generating and stayed that way; `sequence.hpp`'s new native
  `length` is `\omit`ted, confirmed by regenerating and diffing wording —
  only the five pre-existing drifted fragments appear, nothing under
  `transpose.range.*`. Suite went from 146 to 151 (5 added: a native-`length`
  preference test, a per-instantiation `to_vector` test, the hazard test
  exercising a `fold_right` + `element_type` `Impl` through a `Map` with no
  `using`-declaration at all, the `has_fold_map` non-existence test, plus one
  more assertion added to the existing `sequence.hpp` `length` test for
  empty/one-element vectors).
- 2026-09-07 —
  [applicative-ap-only-probing](../tmp/plan/step-applicative-ap-only-probing.md)
  found bullet 2 above stated too broadly for a dual-basis class: `map`,
  `zip_with`, `discard_first` and `discard_second`'s second alternative,
  written by
  [applicative-derived-probing](../tmp/plan/step-applicative-derived-probing.md),
  probed `impl.invoke(...)` while their bodies derive through
  `self.invoke(...)`. For an ap-only `Impl` (`pure` + `ap`, no native
  `invoke`) the body still compiles, through the base's own synthesized
  `invoke`, but the Impl-directed clause is false, so the member silently
  vanished from overload resolution — a regression against the commit
  before that step, invisible because no in-tree `Impl` is ap-only. `lift`
  was unaffected: its second alternative names `pure`, a hard `Impl`
  requirement, so `impl.pure` and `self.pure` cannot disagree. The four
  members' second alternatives were readdressed to `self.invoke(...)`;
  `lift` converted to `self.pure(...)` too, not because it was broken, but
  so clause and body agree everywhere rather than carrying one documented
  exception. `ap` itself is untouched — `invoke`/`ap` is the
  mutually-derivable pair, and both of its alternatives correctly address
  `Impl` directly, which is what closes the derivation cycle.
  `Monad::fmap`'s Impl-directed derivation is a licensed exception distinct
  from both patterns: it is reached only through a wrapping
  `Functor<SomeMonadMap>`, where `self` is the outer `Functor` and the
  inner base's own surface is `protected` and unreachable, so
  `self`-routing is not an option there, and `bind`/`pure` are hard
  requirements, so nothing can vanish. The rider that matters downstream: a
  self-routed probe is only as strong as the constraint on the member it
  names — `self.fold_map(...)` is strong because `Foldable::fold_map` is
  itself constrained, while `self.invoke(...)` is weak, because
  `Applicative<Impl>::invoke` carries no `requires`-clause at all and
  reports a missing basis only through its own body's `static_assert`, by
  design, the GHC-`MINIMAL` message. The four converted members' second
  alternatives are therefore vacuously satisfied for a `Map` with no
  `using Impl::invoke;` re-export — correct, since the class invariant
  already guarantees a complete basis, but no later step may treat those
  clauses as proof that `Impl` itself has one; the strong check belongs on
  `applicative_impl`, over `Impl` directly. Left open: constraining
  `Applicative::invoke` itself would make the probe strong, and was
  deliberately not done here — its ap-derivation runs through
  `detail::terminating_partial` currying and has no spellable
  single-expression probe, which is a design change to a published basis
  member, not a defect fix. `papers/wording/transpose.applicative.derived.md`
  and `transpose.applicative.syn.md` were regenerated and copied over,
  alongside the five pre-existing drifted fragments
  (`transpose.errset.obs.md`, `transpose.errset.recover.md`,
  `transpose.errset.syn.md`, `transpose.expected.syn.md`,
  `transpose.grade.syn.md`), left untouched. Suite went from 151 to 153 (2
  added: an ap-only `Impl`/`Map` exercising the whole derived surface, and
  a `has_map`/`has_zip_with` concept pair proving an inapplicable callable
  still makes `map`/`zip_with` disappear rather than hard-error against the
  shipped invoke-basis object).

## typeclass-conformance-depth

**Question:** What does a typeclass concept check — the basis, or the full
operation surface?
**Status:** DECIDED 2026-09-07
**Decided by:** the design, applied by
[typeclass-object-concepts](../tmp/plan/step-typeclass-object-concepts.md).
**Decision:** Two concepts, at two depths. The concept for a typeclass
**object** (`functor_object`, `applicative_object`, `monad_object`,
`foldable_object`, `traversable_object`, one per class, carrier-indexed)
checks all operations, basis and derived, with conditionally-available
operations required conditionally and operations templated over an
arbitrary callable probed with one representative witness
(`probe_witness`/`probe_witness2` in `detail/typeclass_base.hpp`). There are
no superclass edges: `monad_object` does not require `functor_object`, and
`traversable_object` does not require a Foldable object — the latter is the
DELIBERATE CONSTRAINT `traverse.hpp` already carried and the former is the
whole point of `as-functor-presentation`, the next step. The restricted
`Impl` concept — the other half, naming only the minimal complete bases — is
`typeclass-impl-concepts`, the step after that.

`traverse.hpp`'s existing `applicative_object_for<POLICY, CONTEXT>` becomes
sugar over `applicative_object`, strengthened with the
`pure`-returns-exactly-`CONTEXT` requirement `traverse`'s policy parameter
needs and `applicative_object` itself does not make (that concept only asks
that `pure` exist, not what it returns).

**Why:** Conformance in this library is structural throughout — a program
may hand-implement a typeclass object without ever touching the CRTP base —
so the previous single object concept, `applicative_object_for`, probed
`pure` alone. It accepted an object with `pure` and nothing else and
deferred the failure to whenever some code written much later first reached
a derived operation, three frames deep inside a template, with a diagnostic
about the wrong thing. The deep concept moves the surprise to the gate and
doubles as the specification's statement of the class's full surface, which
is what a specification wants to say anyway.

Record the two cautions from the step file as part of the decision, not as
footnotes: an unconditional `ap` requirement wrongly rejects the simd
object, and a witness is a witness, not a proof that an operation holds for
every callable.

**Record as provisional:** which operations are conditional, and what
licenses each, is a fact about the current basis sets and the current
shipped instances; a new instance's shape could add one. The witness
instantiation is one representative callable because that suffices for "the
operation is missing entirely"; what would justify revisiting is a failure
mode a single witness cannot see.

**Log:**
- 2026-09-07 — [typeclass-object-concepts](../tmp/plan/step-typeclass-object-concepts.md)
  added the five concepts and found two conditional operations the step
  file's own text did not name, on top of the two (`ap`, `subsume`) it did:
  `Foldable::combine_all`/`fold` fold over the *elements themselves* via the
  identity function, so they need the element type — not a wrapped
  accumulator type — to have a registered `Monoid`; `std::vector<int>` is
  Foldable but `int` carries no `Monoid` since
  [monoid-carrier-canonicity](#monoid-carrier-canonicity), so probing them
  unconditionally on `VectorFoldableMap<int>` is a hard, non-SFINAE error
  (`monoid_v<int>`'s definition needs `Monoid<int>` complete), not a graceful
  constraint failure — confirmed empirically, not assumed. The fix is a
  `detail::has_registered_monoid<VALUE_TYPE>` guard in `fold.hpp`, gating
  `combine_all`/`fold` the same way `ap` and `subsume` are gated elsewhere.
  Symmetrically, `Traversable::transpose`/`transpose_with` are hard-wired to
  `applicative_typeclass<element_type>`, which names no applicative object
  for a structure like `std::vector<int>`; probing them unconditionally is
  the same class of hard error, guarded the same way.
  Also measured directly (see `applicative-ap-only-probing`'s existing
  rider, now confirmed rather than merely asserted): probing an
  unconstrained member like `Applicative<Impl>::invoke` or
  `Monad<Impl>::kleisli` with real, type-compatible arguments against an
  `Impl` that has *no* basis at all is not reliably vacuous — it can be a
  hard, non-SFINAE compile error (the `ap`-derivation's own
  `static_assert` fires during the body instantiation a `requires`-expression
  forces to resolve the deduced return type). The concepts here never rely
  on such a member as basis evidence, and the one negative test that needs a
  "pure and nothing else" object (`applicative_object`'s shallow-gate
  regression) uses a hand-rolled, non-CRTP struct for exactly that reason:
  such an object has no `invoke`/`map`/etc. member at all, so probing it is
  a plain, always-safe name-lookup failure rather than a body instantiation.
  `papers/wording/transpose.applicative.syn.md` and
  `transpose.traversable.syn.md` were regenerated and copied over, alongside
  the five pre-existing drifted fragments left untouched. Suite went from
  153 to 154: one new test executable, `objects`, added to
  `tests/beman/transpose/CMakeLists.txt`.
- 2026-09-07 —
  [typeclass-impl-concepts](../tmp/plan/step-typeclass-impl-concepts.md)
  added the other half: `functor_impl`, `applicative_impl`, `monad_impl`,
  `foldable_impl` and `traversable_impl`, each naming only the minimal
  complete basis its class's CRTP base needs, beside that class's object
  concept. `applicative_impl` admits `pure` with either `invoke` or `ap`;
  `foldable_impl` admits `fold_map` alone or `fold_right` with
  `element_type`; both disjunctions are now sayable in one place instead of
  spread across a `static_assert` message and a comment. `monad_impl`
  admits only `pure` + `bind` — Monad's other complete bases (`pure` +
  `fmap` + `join`, and `pure` + `kleisli`) are deliberately not admitted, a
  recorded contingency and not scheduled work. The sentinel for the whole
  discipline is a paired assertion, present for all five classes in
  `tests/beman/transpose/objects.test.cpp`: an `Impl` supplying only its
  basis satisfies the `Impl` concept and fails the object concept
  (`OptionalFunctorImpl`, `OptionalApplicativeImpl`, `OptionalMonadImpl`,
  `VectorFoldableImpl` and `VectorTraversableImpl` against their object
  concepts). The dual-basis assertion from `applicative-ap-only-probing`
  carries over unchanged in spirit: `ApOnlyImpl` (`pure`+`ap`, no `invoke`)
  and `OptionalApplicativeImpl` (`pure`+`invoke`, no `ap`) both satisfy
  `applicative_impl`, and a local `FoldRightOnlyImpl`
  (`fold_right`+`element_type`, no `fold_map`) alongside the shipped
  `VectorFoldableImpl` (`fold_map`, no `fold_right`) both satisfy
  `foldable_impl`. A basis-less struct with no operations at all fails all
  five.
  `applicative_impl` and `foldable_impl` had to be declared *before* their
  CRTP base (`Applicative`, `Foldable`), not merely beside their object
  concept: each base's `invoke`/`fold_map`/`fold_right` body now names the
  concept in a `static_assert`, so two-phase lookup needs the concept
  visible at the class template's definition point. `functor_impl`,
  `monad_impl` and `traversable_impl` are not referenced from inside their
  bases — `Functor::fmap` has no derivation branch to assert from, and
  `Monad`/`Traversable` already enforce their one basis unconditionally via
  `using`-declarations at the top of the class — so those three stayed
  beside their object concept, after the base.
  `Applicative<Impl>::invoke`'s existing basis `static_assert` (message:
  "Applicative Impl must provide pure and at least one basis…") and
  `Foldable<Impl>::fold_map`/`fold_right`'s existing basis `static_assert`s
  (both already documented as provably redundant given the declaration's
  own constraint, per the comments this step left in place) were replaced
  with the concept-based condition rather than duplicated, per this step's
  file. No new diagnostic site was added for `Functor`, `Monad` or
  `Traversable`, since none of the three has a class-body location where a
  concrete `CONTEXT`/`STRUCTURE` is both known and a derivation-branch body
  already exists to hold the assert.
  `papers/wording/transpose.applicative.syn.md` and
  `transpose.traversable.syn.md` were regenerated and copied over again
  (this step's two new concepts), alongside the five pre-existing drifted
  fragments left untouched. Suite stayed at 154: the additions are
  `static_assert`s in the existing `objects` translation unit.

---

## lean-model-sync

**Question:** How does a law proved in the Lean model of this design
(steve-downey/lean-graded) become a check on this implementation, and what
did the first such check find?
**Status:** DECIDED 2026-09-10
**Decided by:** Steve, by vendoring; the findings are stage-local
observations from the first run.
**Decision:** The Lean repository exports every proved law with a C++
consequence as one equation in its generated `docs/probe-harness.md`. This
repository discharges that list in ONE translation unit,
`tests/beman/transpose/probe_harness.test.cpp`, one `TEST_CASE` per
equation, named `probe-harness: <Module>.<theorem>` after the Lean theorem
it checks, with a second translation unit (`probe_harness_cross_tu.cpp`)
for the one claim a `static_assert` cannot make — that two TUs spelling an
`error_set` pack in different orders agree on the type. The file is a test
deliverable in the sense of `tests/beman/transpose/laws.hpp`: checked by
example at std types, never a registration gate, never included by a
shipped header. A harness verb with no library operation behind it is
recorded in the file as a finding, not approximated.
The Lean repository carries this one as a `git subtree` under
`cpp/transpose/`, unsquashed, so a change made there against the model is
sent back here as ordinary commits. This decision is the first such
send-back.
**Why:** The harness had 58 obligations and nothing ran any of them; the
Lean side had stated, correctly, that "probes green" is this repository's
definition of done and not its own. Discharging them here rather than in a
parallel C++ tree keeps one implementation under test.
**Findings from the first run** (all 56 cases green; three obligations
pinned as negatives instead):
- **`first_error` is not an operation of this carrier.** The model's
  accumulating carrier is a list in source order and its projection to the
  short-circuiting carrier takes the head; `toGraded_traverseK` and
  `toGraded_apK` are equations about that head. Here the accumulating
  object keeps one witness PER TYPE, left-biased, in canonical type order
  ([accumulation-evidence](#accumulation-evidence)), so which kind failed
  first is not recorded and no projection from the accumulated value alone
  recovers the short-circuit result. What holds instead, and is what the
  probes check, is the per-kind consequence: the witness kept for the
  short-circuit error's kind IS that error, and with exactly one failing
  operand the two carriers are equal outright. The Lean side owes the
  per-kind statement over a per-kind carrier; nothing here changes.
- **A composed applicative cannot be a `traverse` policy.** `traverse`
  reads the element type of the context it builds from the context TYPE
  (`applicative_value_t`, the carrier's `value_type`); for a nested
  `expected` that is the inner carrier, not the value the composed
  applicative holds, so a composed policy's `pure` fails
  `applicative_object_for` and the accumulator would be
  `vector<expected<B, H>>`. `traverseComp_eq` is checked against a hand
  fold over the library's own two objects instead. Whether the policy
  surface should let an applicative object declare its value type is an
  open question this does not decide.
- **At the empty grade there is nothing to traverse.** A function
  returning bare `T` is not a context, and the explicit uniform form
  `expected<T, error_set<>>` is a graded context whose re-indexing at its
  own grade is bare `T`, so it fails `applicative_object`'s subsumption
  requirement and `traverse` refuses it. Both refusals follow from
  [empty-grade-spelling](#empty-grade-spelling) and are pinned as negative
  `static_assert`s with a positive control. `traverse_fromEmpty` and
  `traverse_fromEmpty_map` — a no-fail traversal is a transform — are
  therefore not equations this code can get wrong: the only spelling for
  the left side is `std::ranges::transform`.
- Two side conditions the model states (`ap_flip` and `flatten_ap`) were
  confirmed necessary by exhibiting the excluded case as an inequality,
  not merely by checking the included ones.
- `rename_cast` has no residue: a same-set grade cast is type identity.
**Log:**
- 2026-09-10 — First run, 56 cases / 217 assertions green on GCC 15.2,
  C++23. The harness's `first_error`, `traverse` with a composed policy,
  and traversal at the empty grade recorded as above.
- 2026-09-11 — The Lean side answered the `first_error` finding with a
  carrier shaped like this one: `Accum.Kinds`, a value or a nonempty
  sub-grade of kinds, with a one-way projection from its list carrier and
  a theorem (`Kinds.noFirstError`) that no projection back exists. The
  four accumulation cases here are renamed for the theorems they now
  discharge — `Kinds.mem_kindsOf_traverseK`, `Kinds.kindsOf_widen`,
  `Kinds.kindsOf_apK`, `Kinds.toGraded_mem` — and the list-form rows
  carry no C++ equation. Assertions unchanged; the left-bias clause (the
  witness kept for a kind is the leftmost) is still checked here and is
  not yet stated there, pending payload-bearing accumulation.

---

## operand-value-category

**Question:** In what value category does an Applicative object's `invoke`
take its operands, and what may a traversal therefore promise about
complexity and about move-only values?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve, answering the P3200 reference-implementation review.
**Decision:** Every Applicative object this library registers deduces its
operands through forwarding references and passes each held value on with
that operand's own value category. `detail::forward_contained` and
`detail::forward_at` are the two spellings of that. The vector Traversable
hands its accumulated result to each composition as an rvalue, and carries a
second, consuming `traverse` overload so that `transpose(std::move(v))`
expresses that intent through to `function`.

`transpose`'s Complexity clause is stated in *composition operations* per
element, not in element operations. The element bound is added as a Remark,
conditioned on the applicative object composing an rvalue operand without
duplicating what it holds.
**Why:** With `const&` operands the only thing an accumulation can do is copy
the whole prefix built so far, once per element, so a successful
`transpose(vector<optional<T>>)` over 100 elements performed 5,150 element
copies while the wording promised linear. The same `const&` path made
`transpose(std::move(v))` a copy, so a move-only element type was rejected by
a call that looked like it consumed its argument.

The Complexity clause is split the way it is because the unconditional
element bound is not the algorithm's to promise. A user-supplied applicative
object that composes only `const` lvalues is still a perfectly good one, and
against it the traversal is quadratic in element operations however the
traversal is written. Promising linear regardless would be a guarantee the
generic front door cannot keep; promising nothing would understate what the
registered objects do.
**Consequences:** An applicative object registered by a program need not opt
in -- the test suite's `Identity` instance is deliberately left composing
`const&` operands so that this stays covered -- but only an object that does
opt in gets the linear element bound.

`zip_list` is the one registered object that cannot simply move: a repeating
list's single stored value logically occupies every position, so it is read
once per lane and `forward_zip_list_value_at` copies for exactly that case.
This is not a corner: the seed of any traversal into that context is
`pure(...)`, which *is* a repeating list.
**Log:**
- 2026-09-11 — Decided. Converted `optional`, `expected` (both the
  short-circuiting and the accumulating object), `array`, `simd_lanes`,
  `zip_list` and the demonstration `sender`. The two `expected` homogeneous
  cores moved from matching `const expected<T, ERROR_TYPE>&` by pattern to
  constraining on `all_declare_v`, which keeps them exactly complementary to
  the ungraded-mixed and graded-mixed cores they sit beside. Measured: 5,150
  element copies to 100 for a 100-element `optional` traversal, and the same
  for `expected`; the two-size ratio is what the regression test reads, since
  an absolute bound alone cannot separate a linear implementation with a
  large constant from a quadratic one.
- 2026-09-11 — Extended to the conformance concepts, which had the same
  defect one level up and hid it behind a diagnostic. `applicative_impl` and
  `applicative_object` probed `pure` and `lift` with a `const` lvalue
  element, and `applicative_object` probed `discard_first` and
  `discard_second` with `const` lvalue operands. Those are the two directions
  a value crosses the context boundary -- in through `pure`/`lift`, out
  through the `discard_*` pair -- and neither operation's specification asks
  for a copy. For an element type that can be moved but not copied the probes
  were not merely stricter, they were ill-formed: the derived operations have
  deduced return types, so probing instantiates the body, and a body that
  cannot copy is a hard error rather than an unsatisfied constraint. A
  concept that diagnoses cannot detect. Every probe now reads in the category
  the operation is used in. `applicative_object_for` already probed `pure`
  with an rvalue for its exact-return refinement, so the concept file was
  already inconsistent with itself.

  Two consequences. `applicative_context` had been introduced the same day as
  a narrower stand-in, because `applicative_object` would have rejected the
  consuming traversal added in this same entry; with the probes corrected it
  is exactly `applicative_object` over the looked-up object again, and the
  stand-in is gone. And `traverse_context_t` inferred the context from a
  `const` lvalue element whatever the category of the structure, so the free
  `traverse` could not reach the consuming path that `transpose` could:
  `traverse(identity, std::move(v))` over a move-only element type did not
  compile. It now names `traverse_element_t`, which is the element category
  matching the structure's own. That makes explicit a requirement the vector
  instance already kept and the wording did not state: a traversable object
  passes elements on in the category it received the structure in. Without
  it the context cannot be inferred before a traversable object is selected.

  Verified on GCC 16 and Clang 23: 241 and 239 tests, the conformance
  concepts satisfied over `optional<unique_ptr<int>>`, and the negative
  detection cases still evaluating false rather than diagnosing.

---

## p2300-front-door-shape

**Question:** Does the proposed Applicative and Traversable surface actually
accommodate `std::execution` (P2300) senders, and what evidence says so?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve, answering the P3200 reference-implementation review.
**Decision:** An Applicative object over genuine senders lives at
`examples/p2300_adapter.hpp`, written against `bemanproject/execution` and
exercised by `tests/beman/transpose/p2300.test.cpp`. It is opt-in behind
`BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, OFF by default, and lives under
`examples/` rather than `include/` so that nothing an installation of this
library pulls in depends on it.

The demonstration `sender<T>` in `include/beman/transpose/sender.hpp` stays,
and its header comment now says what it does and does not show.
**Why:** The paper's second motivating domain is deferred computation, and the
only evidence for it was a `std::function<T()>` wrapper. That is enough to
show the front door preserves laziness and exercises none of what makes a
P2300 sender a sender. The review was right that the claim could not rest on
it.

The adapter supplies only the `pure` + `invoke` basis, so `map`, `zip_with`
and `lift` in the test are the library's own CRTP derivations running over a
context they were not designed against. Values, laziness, the error channel
and move-only values all compose.
**Consequences:** Writing it established two limits, which are the more
valuable half of the result.

*The vector front door cannot take real senders.* `VectorTraversableImpl::
traverse` accumulates with `accumulated = applicative.invoke(...)`, an
assignment, so the context type must be invariant under composition. It is
for `optional<vector<T>>`. It is not for a real sender: `then(when_all(acc,
elem), f)` names a new type at every step, so a runtime-sized traversal
cannot be written as a loop at all. Transposing a runtime-sized structure of
senders needs a type-erased sender, which `bemanproject/execution` does not
ship. The toy `sender<T>` passes only because `std::function<T()>` is already
type-erased -- the very thing that made it a convenient demonstration is what
hid the limit. Compile-time-sized transposition, the shape `transpose_tuple`
already uses, has no such problem, and that is what the adapter demonstrates.

*An operand must have exactly one value completion.* `when_all` requires it,
so `just_error(...)` and `just_stopped()` cannot be operands. Error
propagation is therefore shown with an operand that can complete with a value
and fails at run time. A claim that "error and stopped completions compose"
would be broader than what holds.

*`when_all` does not sequence its children.* It completes when all of them
have. The Remarks that say contexts "are composed in that same order" can
therefore only be about the order results are assembled into the structure,
not the order in which effects run. A synchronous context makes the two
coincide; a concurrent one cannot, and the wording should not be read as
promising it does.
**Log:**
- 2026-09-11 — Decided and built. Pinned at
  `d24898d7264e74fb723b50d6275a5d05f65ddb20`; the project has no tags.
  Verified with GCC 16 against a local checkout via
  `-DFETCHCONTENT_SOURCE_DIR_EXECUTION=...`; five cases green.
  Two follow-ups are open rather than done. The evidence is not in CI, so it
  can rot: enabling it in one workflow job is the obvious next step. And the
  vcpkg dependency path does not resolve `beman.execution` -- when vcpkg is
  present the Makefile makes the vcpkg toolchain the top-level include, so
  the lockfile provider is not active and the evidence build needs the
  FetchContent path.
- 2026-09-13 — CONTRADICTED IN PART, by stage
  [execution-baseline](transpose-execution-plan.md#execution-baseline), which
  is the first stage of a plan drafted without this entry in view.
  [transpose-execution-plan.md §1](transpose-execution-plan.md#context) states
  that "a survey of every steve-downey repository on 2026-09-11 found no
  implementation over real `std::execution` senders anywhere". That is false
  of this repository on that date: `examples/p2300_adapter.hpp` and
  `tests/beman/transpose/p2300.test.cpp` are this entry, and they were built
  and ruled on the same day. The survey sentence is a Context claim, not a
  Why, and the plan's load-bearing argument survives it — but three of the
  plan's stage premises do not, and they are recorded here rather than in the
  plan, which is not self-updating (divergence protocol rule 5).
  * STAGE 1'S BASIS ALREADY EXISTS. `P2300ApplicativeImpl` supplies
    `pure(x) = ex::just(x)` and
    `invoke(f, s...) = when_all(s...) | then(f)`, with operands forwarded
    rather than taken by `const&` — which is
    [transpose-execution-plan.md §3](transpose-execution-plan.md#sender-registration)
    stage sender-registration's deliverable 1 second bullet, written and
    green. What that stage genuinely adds is the part this entry deliberately
    did NOT do: the `applicative_typeclass` registration and the
    `applicative_value` reading. The adapter's own header comment says so in
    as many words.
  * THIS ENTRY'S CONSEQUENCES ARE NOW CONTESTED, DELIBERATELY. The paragraph
    above reading "Transposing a runtime-sized structure of senders needs a
    type-erased sender, which `bemanproject/execution` does not ship" is
    exactly what
    [runtime-arity-composition](#runtime-arity-composition) and
    [erasure-boundary](#erasure-boundary) propose to refute, by giving the
    applicative object a native n-ary `collect` whose operation state owns
    *n* children in one allocation. Both readings cannot stand. The
    disagreement is not settled here and must not be settled by argument: it
    is Stage 2's allocation-count tripwire that settles it, and if that
    tripwire fires, this paragraph was right and the plan was wrong. Until
    then the log carries both, flagged, rather than one silently.
  * THE VCPKG TRIPWIRE HAS ALREADY FIRED, AND WAS ALREADY ANSWERED. Stage 0's
    tripwire says "package not found under vcpkg → propose FetchContent,
    wait". Confirmed independently: `vcpkg.json` lists only `catch2`, and the
    lockfile FetchContent path pinned at
    `d24898d7264e74fb723b50d6275a5d05f65ddb20` is what resolves the
    dependency. That is this entry's existing answer, so there is nothing to
    propose and nothing to wait for.
  * THE EVIDENCE STILL IS NOT IN CI. The follow-up this entry left open on
    2026-09-11 is still open on 2026-09-13; nothing in Stage 0 closed it.
- 2026-09-13 — THIS ENTRY STANDS, ruled by Steve the same day, resolving the
  divergence above. **Nothing execution-dependent enters `include/`.** The
  execution plan's stages 1–3 build on `examples/p2300_adapter.hpp` where it
  lives; `all_of` lands as `examples/all_of.hpp` under this entry's existing
  `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, pinned at this entry's existing
  `d24898d`; and the Stage 3 `collect` hook in `sequence.hpp` is the only
  `include/` change the plan makes, staying sender-free. Two addendum entries
  drafted without this one in view are withdrawn as superseded —
  [execution-dependency-shape](#execution-dependency-shape) and
  [demo-sender-fate](#demo-sender-fate) — and the plan's Stage 0
  dependency-wiring deliverable is struck rather than performed, the
  dependency having been wired since 2026-09-11.
  **What remains genuinely contested, and how it gets settled.** This entry's
  Consequences paragraph reading "transposing a runtime-sized structure of
  senders needs a type-erased sender" is NOT ratified by this ruling;
  [runtime-arity-composition](#runtime-arity-composition) and
  [erasure-boundary](#erasure-boundary) are kept, and they say the opposite.
  The standing instruction, from the same ruling: **when Stage 2's
  allocation-count test passes, add a dated Log entry here recording that the
  "needs a type-erased sender" consequence is refuted by a non-erased n-ary
  operation state.** Until that entry exists the paragraph stands as written,
  and a later reader should treat it as the live claim rather than assume the
  plan won by being newer.
- 2026-09-14 — **REFUTED, BY MEASUREMENT.** Stage
  [all-of-algorithm](transpose-execution-plan.md#all-of-algorithm) discharged
  the standing instruction above. This entry's Consequences paragraph reads
  "Transposing a runtime-sized structure of senders needs a type-erased
  sender, which `bemanproject/execution` does not ship." **It does not.** It
  needs a native n-ary composition, and `examples/all_of.hpp` is one:
  `all_of(std::vector<S>) -> all_of_sender<S>`, an operation state owning
  *n* child operation states constructed in place and never moved.
  The numbers, from `tests/beman/transpose/all_of_allocation.test.cpp` at
  n = 1000: **one** allocation during `connect`, of **32,000 bytes =
  n x sizeof(holder)** with `sizeof(holder) == 32`; **one** further
  allocation during `start`, the result `vector<T>`; **two** in total, the
  second being exactly the one the boundary decision permits.
  `holder` is `{optional<T> value; connect_result_t<S, element_receiver> op;}`
  -- the child's result slot colocated with the child's operation state,
  which is what keeps the budget at one block rather than two. Also
  asserted: `decltype(all_of(v))` is `all_of_sender<S>`, so the type is
  spelled from `S` and nothing is erased; and neither the sender, the
  operation state nor the holder is polymorphic. The header stores no
  callable and includes no `<functional>`.
  Behaviour, green on gcc-debug and llvm-debug and **TSan clean** on the
  concurrent case: results in input order while sixteen children complete on
  their own threads in deliberately reversed order; laziness; the empty and
  one-child cases; a child's error propagating with siblings observing the
  stop request; an external stop reaching the children; a move-only payload.

  **What this does NOT yet show, stated so the refutation is not read wider
  than it is.** `transpose(std::vector<S>)` over a real sender still does not
  work -- the vector Traversable still folds, and teaching it to prefer a
  native `collect` is stage
  [collect-hook](transpose-execution-plan.md#collect-hook)'s deliverable. So
  the second half of this entry's finding, that the vector front door cannot
  take real senders, remains true TODAY, and is now true for a reason that
  has a fix rather than for the reason this entry gave. The first half --
  that the fix must be type erasure -- is what the measurement refutes.
  [runtime-arity-composition](#runtime-arity-composition) and
  [erasure-boundary](#erasure-boundary) are no longer contested; this
  paragraph is.

---

## execution-dependency-shape

**Question:** How does `beman.execution` enter the build, and is it required?
**Status:** WITHDRAWN 2026-09-13. Superseded by
[p2300-front-door-shape](#p2300-front-door-shape), which answered this
question on 2026-09-11 and stands.
**Decided by:** Steve Downey, 2026-09-13, ruling on the Stage 0 divergence.
**Decision:** There is no separate answer here. The dependency enters exactly
as [p2300-front-door-shape](#p2300-front-door-shape) says it does: optional,
behind `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, **OFF by default
unconditionally**, resolved through `lockfile.json` FetchContent pinned at
`d24898d7264e74fb723b50d6275a5d05f65ddb20`, with everything
execution-dependent under `examples/`. **Nothing execution-dependent enters
`include/`.** The slug is kept rather than deleted so that the links the
execution plan and its addendum already carry resolve to this redirection
instead of to nothing.
**Why the drafted answer was not taken:** it was drafted without the deciding
entry in view, and was weaker on the axis that matters. Default-ON-when-found
means that on any machine where `beman.execution` happens to be installed,
`#include <beman/transpose/transpose.hpp>` acquires an execution dependency
the installer never asked for. That is the outcome
[p2300-front-door-shape](#p2300-front-door-shape) names and forbids, and the
two entries' Whys never disagreed about it — both want the front door light.
Only the What differed, and the decided one is stricter.
**Consequence for the execution plan:** stages 1–3 build on
`examples/p2300_adapter.hpp` where it lives. `all_of` lands as
`examples/all_of.hpp` under the same option. The Stage 3 `collect` hook in
`sequence.hpp` is the only `include/` change the plan makes, and it stays
sender-free — which is what
[runtime-arity-composition](#runtime-arity-composition) already required of
it for independent reasons.
**Log:**
- 2026-09-11 — Drafted PROPOSED in the execution plan's addendum.
- 2026-09-13 — Merged at Stage 0
  [execution-baseline](transpose-execution-plan.md#execution-baseline) as
  OPEN rather than default-as-drafted, because taking the default would have
  reversed a DECIDED entry by inaction (divergence protocol rule 2).
  Facts recorded for the ruling, all measured in that stage rather than
  assumed: the dependency resolves through `lockfile.json` FetchContent at
  `d24898d7264e74fb723b50d6275a5d05f65ddb20`, not through vcpkg
  (`vcpkg.json` lists only `catch2`); the exported namespace is
  `beman::execution`, with a legacy `beman::execution26` tree still shipped
  alongside in `include/beman/execution26/` (two headers), which is what
  `compile-time-scheme` was using; and the dependency requires C++23 or
  greater, declaring `cxx_std_${CMAKE_CXX_STANDARD}` rather than a floor of
  its own, so it inherits this repository's C++23.
- 2026-09-13 — WITHDRAWN by Steve's ruling, same day. The existing entry
  stands; no new option, no new pin, no `include/` dependency. Stage 0's
  dependency-wiring deliverable is struck from the plan rather than
  performed, the dependency having been wired since 2026-09-11.

---

## sender-instance-keying

**Question:** How is the Applicative object found for a P2300 sender, given
there is no single context template `M` — each adaptor is its own type?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve Downey, 2026-09-11 (planning discussion).
**Decision:** A constrained partial specialization of the
`applicative_typeclass` variable template:
`template <class S> requires single_value_sender<S> inline constexpr auto
applicative_typeclass<S> = ExecutionApplicativeMap{};` where
`single_value_sender` is `ex::sender<S>` plus "exactly one value completion,
exactly one argument" under the empty environment. The object itself is not
templated on `S`; `pure` and `invoke` are member templates.
**Why:** Every other instance is keyed on a concrete carrier
(`optional<T>`, `vector<T>`) because there *is* a template to key on. Senders
have none, and the demo's `sender<T>` hid that by being one. Keying by
concept is the honest spelling: the object is chosen because the type
*behaves* as a sender, which is the duck-typing posture the library already
takes at use sites. The single-value restriction is the arity `when_all`
imposes on its children and the arity an Applicative element needs; senders
outside it are simply not registered, so the framework's existing
"no applicative_typeclass<T>" diagnostic fires. One object for all sender
types (rather than one per `S`) is what lets `pure(x)` return `just(x)` — a
different sender type — without the object having to know it.
**Sentinel:** any `applicative_typeclass<some_specific_adaptor_type>`
specialization is a regression to per-type keying.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-11 — Ratified by Steve: one concept-keyed object for all sender
  types. Tripwire for Stage 1: ambiguity between this and
  any future concept-keyed registration must be resolved by subsumption,
  never by adding a tie-breaker tag.
- 2026-09-13 — Merged into this log at Stage 0
  [execution-baseline](transpose-execution-plan.md#execution-baseline).
  Unchanged from the ratified text except that "under `empty_env`" now reads
  "under the empty environment": the pinned dependency spells that type
  `beman::execution::env<>` and exports no `empty_env` at all. See
  [sender-value-type-reading](#sender-value-type-reading).
- 2026-09-13 — THIS SLUG IS NOW STAGE 1'S LOGBOOK, by Steve's ruling of the
  same day. Stage 1 is no longer "write a registration"; it is **audit
  `examples/p2300_adapter.hpp` against this entry and
  [sender-value-type-reading](#sender-value-type-reading), and bring it up to
  them** — concept-keyed rather than per-type, one object for all sender
  types, element type read from completion signatures — plus the law harness
  and move-only coverage the adapter does not yet have. The adapter today
  supplies the `pure`/`invoke` basis and deliberately performs NO
  `applicative_typeclass` registration and NO `applicative_value` reading, so
  the audit has real work to do and a known starting gap.
  **Every gap the audit finds is logged here**, under this slug, rather than
  under a new one — that is what the ruling directs, and it keeps the keying
  question and the evidence for it in one place. Two constraints inherited
  from the same ruling: the work stays in `examples/`, nothing
  execution-dependent enters `include/`, and the concept-keyed registration
  must therefore be reachable from the adapter's own header rather than from
  `transpose.hpp`.
- 2026-09-13 — STAGE 1 AUDIT COMPLETE. The adapter is now concept-keyed and
  reads its element type from completion signatures; `p2300.test.cpp` pins
  each claim below. Five findings, in the order they cost time.
  * THE KEYING WORKS AS DECIDED, AND THE SENTINEL HOLDS. One object
    (`P2300ApplicativeMap`) is reached from every registered sender type —
    `just(1)`, a `then`-adapted sender and `just(std::string{})` all resolve
    to the same object type, asserted directly rather than assumed. `pure(1)`
    returns `decltype(just(1))`, which is NOT the `S` the object was found
    under, and that is pinned too: it is the concrete reason the object
    cannot be templated on `S`, since a per-`S` object would have to promise
    `pure` returns `S` and no sender applicative can keep that.
  * THE DEMONSTRATION SENDER IS NOT AN `ex::sender`, checked rather than
    assumed, so the feared ambiguity between this concept-keyed registration
    and `sender.hpp`'s per-type one does not arise. Pinned as a
    `static_assert` in `p2300.test.cpp`: if it ever fails, the keying
    decision's tripwire applies — resolve by subsumption, never by adding a
    tie-breaker tag.
  * A RAW `when_all` IS NOT AN APPLICATIVE ELEMENT, which surprised this
    stage and is correct. `when_all(just(1), just(2))` completes with TWO
    value arguments, so it fails `single_value_sender` and is unregistered;
    the adapter's own `invoke` is `when_all(...) | then(f)`, and the `then`
    is what collapses the pack back to one value. So the RESULT of `invoke`
    is an element even though its middle term is not. Both directions pinned,
    because an agent "fixing" the negative would break the arity contract
    that [all-of-failure-semantics](#all-of-failure-semantics) and `when_all`
    both depend on.
  * THE DISJOINTNESS ASSUMPTION HOLDS, VACUOUSLY, AND IS NOW MEASURED. No
    sender in the pinned beman.execution has a nested `value_type`, so this
    adapter's `applicative_value` specialization and the framework's
    `void_t<typename T::value_type>` path never both match. The exclusion the
    decision requires is therefore currently doing no work, which is exactly
    why it is worth keeping: it is the tripwire, not the mechanism.
  * **THE REGISTRATION MAKES `transpose(vector<S>)` FAIL WORSE, and this is
    the finding worth carrying to Stage 3.** Before registration, a vector of
    real senders reached the framework's clean "no matching function for call
    to `transpose`" — a constraint failure at the call site, measured against
    an unregistered sender to confirm. After registration the call is viable
    far enough to instantiate `VectorTraversableImpl::traverse`, and it dies
    inside `sequence.hpp` at `accumulated = applicative.invoke(...)` with
    *"no viable overloaded `=`"* and a three-frame template backtrace. The
    diagnostic is honest — that assignment IS the invariance the fold needs
    and a real sender does not have — but it is a regression in message
    quality that registration bought.
    Not fixed here, deliberately: the fix is the `collect` hook, which is
    stage [collect-hook](transpose-execution-plan.md#collect-hook)'s
    deliverable, and Stage 1's acceptance forbids making the vector front
    door work early. The stage's acceptance is still met — `transpose` over a
    vector of senders does not work — and no negative `static_assert` could
    be written for it, since probing it is itself the hard error. Stage 3
    should treat "this message becomes clean, or becomes a success" as one of
    its own acceptance signals.
  * THE POLICY CONCEPT ADMITS EXACTLY ONE SENDER SHAPE, which is finding one
    above arriving as a bill. `applicative_object` does not constrain what
    `pure` returns; `applicative_object_for`, the `traverse` policy concept,
    adds `pure(element) -> same_as<CONTEXT>` per
    [typeclass-conformance-depth](#typeclass-conformance-depth). Measured
    across three sender shapes: the deep object concept holds for all three,
    and the policy concept holds only for `decltype(just(1))` — failing for a
    `then`-adapted sender and for `when_all(...) | then(...)`. `pure(x)` is
    always `just(x)`, so the refinement is met only when `CONTEXT` happens to
    BE the type `pure` produces, and a caller's sender almost never is.
    One object for all sender types is exactly what makes `pure` free to
    return a type of its own choosing, and this concept asks for the
    opposite; the decision is not thereby wrong, but its cost has a name now.
    Consequence for stage [collect-hook](transpose-execution-plan.md#collect-hook):
    the vector front door cannot be reached by satisfying the existing policy
    concept, so `collect` must be probed before that refinement applies, or
    the refinement must relax for objects supplying `collect`. All three rows
    are pinned in `p2300.test.cpp` so that choice is made against
    measurements. Not decided here — it is Stage 3's to raise, under its own
    slug, with this as the evidence.

---

## sender-value-type-reading

**Question:** How is `applicative_value_t<S>` read for a sender?
**Status:** PROPOSED 2026-09-11, still PROPOSED after the 2026-09-13 ruling,
which kept it as written rather than graduating it. Stage 1 audits the
adapter against it; a gap found there is logged under
[sender-instance-keying](#sender-instance-keying).
**Proposed decision:** A specialization of `applicative_value` for
`single_value_sender` types reading
`value_types_of_t<S, env<>, type_identity_t, type_identity_t>`, decayed.
The `void_t<typename T::value_type>` path is not used for senders even if
an adaptor happens to expose a `value_type`; the sender specialization is
constrained to types *without* a `value_type` member so the two partial
specializations stay disjoint (tripwire if this exclusion is ever the
reason a real sender fails to register).
**Why:** A sender's element type is what it *sends*, and the only
authoritative statement of that is its completion signatures. Reading a
`value_type` member would be reading a coincidence.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-13 — CORRECTION to the Stage 0 entry below, made by Stage 1
  [sender-registration](transpose-execution-plan.md#sender-registration) on
  the same day. That entry inferred from "the alias is ill-formed for a
  two-argument sender" that "a `requires` clause naming the alias is a hard
  error, not a graceful constraint failure", and called it the same
  non-SFINAE-friendly hazard
  [functor-monad-grounding](#functor-monad-grounding) records for
  `OptionalMonadImpl::bind`. **That inference was wrong**, and the test
  written to support it already disproved it: ill-formed INSIDE a
  requires-expression is a constraint failure, which is the whole point of a
  requires-expression. `execution_probe.test.cpp`'s own
  `static_assert(!reads_as_single_value<decltype(just(1, 2))>)` is a
  `requires` clause naming the alias, evaluating to false, compiling.
  The measurement was right and the conclusion drawn from it was not; the
  sentence is struck rather than amended, and the corrected fact is that
  `single_value_sender` can be spelled directly in terms of the alias with
  no pre-check. Stage 1 pins `just(1, 2)`, `just()`, `just_error(...)`,
  `just_stopped()` and a raw `when_all` as constraint failures in
  `p2300.test.cpp`. Worth the space because the wrong version would have
  cost Stage 1 an afternoon building a pre-check that nothing needs.
- 2026-09-13 — Merged at Stage 0 as PROPOSED, with ONE What
  corrected against the pinned dependency and verified by compiling it.
  *Plan and draft said:* `value_types_of_t<S, empty_env, type_identity_t,
  type_identity_t>`. *Reality:* `beman::execution` at
  `d24898d` exports no `empty_env`; the empty environment is the
  zero-argument case of the `env<...>` template, `beman::execution::env<>`,
  which is also the default for that alias's second parameter. This is the
  P2300 spelling change LWG made when it replaced the dedicated `empty_env`
  type; the draft was written against the older name. A What, not a Why —
  the reading is the same reading — so per the Stage 0 tripwire for the
  namespace case, logged and proceeded with the real one.
  *Confirmed by measurement, not by reading:* with `Tuple` and `Variant`
  both `std::type_identity_t`, `value_types_of_t<decltype(just(1)), env<>>`
  is exactly `int`, and the same alias over `decltype(just(1, 2))` is
  ILL-FORMED rather than merely different, because `type_identity_t` is not
  variadic. Both are pinned in
  `tests/beman/transpose/execution_probe.test.cpp`.

---

## runtime-arity-composition

**Question:** How does a runtime-sized Traversable compose contexts whose
n-ary combination is a new type at every step?
**Status:** DECIDED 2026-09-13
**Decided by:** Default-as-drafted at Stage 0
[execution-baseline](transpose-execution-plan.md#execution-baseline), per
transpose-execution-plan.md §3 deliverable 1. Not individually ruled by Steve.
**Was contested, now settled (2026-09-14):** this entry contradicted a
Consequences paragraph of
[p2300-front-door-shape](#p2300-front-door-shape). The contradiction was
resolved by measurement at stage all-of-algorithm, in this entry's favour;
the refutation is logged under that entry. See the Log there and below.
**Decision:** The Applicative object may offer a native range composition
`collect(std::vector<S>) -> S'` with `applicative_value_t<S'> =
vector<applicative_value_t<S>>`. The vector Traversable's `traverse` prefers
`collect` when the object offers it and otherwise takes the existing
`pure`/`invoke` left fold. For P2300 senders `collect` is `all_of`, a
sender algorithm whose operation state owns *n* child operation states and
joins them. `collect` is optional in `applicative_object`.
**Why:** The fold is correct for every instance whose combination is
type-stable. It is not *wrong* for senders; it is unspellable, because a
loop cannot hold a value whose type changes. The alternatives are
sequencing (loses the independence the structure states) or erasing (pays
allocation and indirection per element, and hides the type the caller
wants to keep composing). `collect` moves the n-ary knowledge to the one
place that has it — the applicative object — and leaves the Traversable
generic. This is
[derived-op-native-preference](#derived-op-native-preference)
applied on the Traversable side. The name is deliberately not `when_all_range`
or `sequence`: it is the range twin of `invoke`, and other applicatives
could offer it for performance (a single-pass `expected` collect avoids
`n` vector moves) without any sender vocabulary.
**Sentinel:** the vector Traversable naming any sender type, or any
`if constexpr` on "is a sender", violates this decision.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-13 — Merged at Stage 0, default-as-drafted, and flagged
  CONTESTED. [p2300-front-door-shape](#p2300-front-door-shape) records, from
  having written the adapter, that "transposing a runtime-sized structure of
  senders needs a type-erased sender". This entry says it needs a native
  n-ary composition instead. Adopting this one by default does not overrule
  that one — nothing at Stage 0 has tested either — and the two are left
  standing together on purpose, each pointing at the other. What decides
  between them is Stage 2's allocation-count tripwire, which is a
  measurement; an argument either way before that measurement exists should
  be treated as unsupported by both entries.
- 2026-09-14 — **SETTLED IN THIS ENTRY'S FAVOUR; the CONTESTED flag is
  removed.** Stage [all-of-algorithm](transpose-execution-plan.md#all-of-algorithm)
  built the native n-ary composition and measured it, and the refutation is
  written where the contested claim lives, under
  [p2300-front-door-shape](#p2300-front-door-shape), per the standing
  instruction. `collect` exists on the adapter's object as
  `collect(std::vector<S>) = all_of(std::move(v))`, so the n-ary knowledge
  sits in the applicative object exactly as this entry said it should. The
  Traversable side -- preferring `collect` over the fold -- is untouched and
  remains stage [collect-hook](transpose-execution-plan.md#collect-hook)'s.
  The Sentinel still holds: nothing in `sequence.hpp` names a sender, and
  nothing execution-dependent has entered `include/`.
- 2026-09-13 — Location and division of labour fixed by Steve's ruling.
  `all_of` lands as **`examples/all_of.hpp`** under
  `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`. The Stage 3 `collect` hook in
  `sequence.hpp` is **the only `include/` change this plan makes, and it
  stays sender-free** — which this entry's own Sentinel already demanded for
  independent reasons ("the vector Traversable naming any sender type, or any
  `if constexpr` on 'is a sender', violates this decision"). The ruling and
  the sentinel agree from different directions: the hook probes for a
  `collect` member and knows nothing about what supplies it, so the generic
  half can live in `include/` precisely because it is generic, and the sender
  half must live in `examples/` because it is not.
  **Standing instruction for Stage 2:** when the allocation-count test
  passes, add a dated Log entry under
  [p2300-front-door-shape](#p2300-front-door-shape) recording that its
  "needs a type-erased sender" consequence is refuted by a non-erased n-ary
  operation state. That entry is the one this decision contests, and the
  refutation is not complete until it is written where the contested claim
  lives.

---

## erasure-boundary

**Question:** What counts as the erasure the design forbids?
**Status:** DECIDED 2026-09-11
**Decided by:** Steve Downey, 2026-09-11 (planning discussion).
**Decision:** Forbidden in the execution-dependent headers: `std::function`/
`move_only_function`, `any_sender`/`task`-style wrappers, virtual dispatch,
and per-element heap allocation of operation states. Permitted: the single
allocation holding the *n* child operation states and result slots (its size
is a function of `n` and `sizeof(connect_result_t<S, R>)`, known at
`connect`), and the result `vector<T>` allocation. The composed sender's type
is spelled from `S`.
**Why:** The claim under test is "no wrapper materialized". The
measurable form of that claim is *one* allocation whose size is
`n × (a compile-time constant)`, and a result type that carries `S` so
the caller keeps composing on the real thing. Anything else is the
demo sender in a different coat.
**Sentinel:** the Stage-2 allocation-count test.
**Log:**
- 2026-09-11 — Drafted.
- 2026-09-11 — Ratified by Steve: the boundary is measured (allocation
  count + result type spelled from `S`), not described.
- 2026-09-13 — Merged at Stage 0. The decision text said "in `execution.hpp`
  and `all_of.hpp`"; those paths are the drafted ones and are exactly what
  [execution-dependency-shape](#execution-dependency-shape) leaves open, so
  the text now names the headers by role rather than by path. No change of
  substance — the boundary is about what the code may contain, not where it
  lives.
- 2026-09-13 — Location fixed by Steve's ruling: the header this boundary
  governs is **`examples/all_of.hpp`**, under
  `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`, not `include/beman/transpose/`.
  Nothing about the boundary itself changes — it is a rule about what the
  code may contain, and one allocation is one allocation wherever the file
  sits. Worth stating because the measurement gets easier: a header under
  `examples/` that no installation pulls in can carry the allocation-counting
  test's hooks without any of it becoming library surface.
  The complementary pin landed this stage:
  `tests/beman/transpose/demo_sender_golden.test.cpp` asserts that the
  DEMONSTRATION sender is invariant under composition and copyable, which is
  the erasure this boundary permits there and forbids in the real instance.
  Pinning both directions is what keeps "no wrapper materialized" a claim
  about the real instance rather than about the library as a whole.
- 2026-09-13 — THE BUDGET IS ACHIEVABLE, AND THE PLAN'S LAYOUT DOES NOT MEET
  IT. Found by the Stage 0 prior-art survey
  ([prior-art-when-all-range.md](review/prior-art-when-all-range.md)), and
  recorded here because this entry's sentinel is the allocation count.
  *Achievable:* libunifex's `when_all_range` lands on exactly the budget this
  entry permits — one array allocation for all children, plus the result
  vector — so the Stage 2 tripwire asks for something that has been done
  rather than something hoped for.
  *Not met as drafted:*
  [transpose-execution-plan.md §3](transpose-execution-plan.md#all-of-algorithm)
  deliverable 2 asks for the *n* child operation states in one allocation AND
  the result slots as a separate `vector<optional<T>>`. That is two blocks
  before the result vector, and the same stage's tripwire stops on "more than
  one allocation and the extra is not the `vector<T>` result". The plan's
  deliverable and the plan's tripwire are therefore in tension with each
  other, independently of anything this log says. libunifex resolves it by
  colocating each child's value slot WITH its operation state in a single
  holder element:
  `struct holder { optional<value_type> value; connect_result_t<S, R> connection; };`
  over one `allocator.allocate(n)`. Recorded as the shape that satisfies the
  boundary, not as the shape Stage 2 must adopt — per the plan's own "record
  only; do not copy". What Stage 2 may not do is relax the tripwire to fit
  the drafted layout (divergence protocol rule 4).
- 2026-09-14 — SENTINEL FIRED, AND PASSED. Stage
  [all-of-algorithm](transpose-execution-plan.md#all-of-algorithm) built
  `examples/all_of.hpp` and measured it at n = 1000: one allocation at
  `connect` of n x sizeof(holder) = 32,000 bytes, plus the result vector at
  completion, and nothing else. This entry's own words -- "its size is a
  function of `n` and `sizeof(connect_result_t<S, R>)`, known at `connect`"
  -- are now a passing assertion rather than a design intention. The
  refutation that follows from it is logged under
  [p2300-front-door-shape](#p2300-front-door-shape).
  Two things worth carrying forward.
  *The layout that meets the budget is the colocated one*, as the 2026-09-13
  entry above predicted from libunifex. The drafted layout -- child
  operation states in one block, result slots in a separate
  `vector<optional<T>>` -- is two blocks before the result vector and would
  have tripped this sentinel. Each child's slot now sits INSIDE its holder,
  beside its operation state.
  *The measurement cannot share a translation unit with ThreadSanitizer.*
  Counting allocations means replacing global `operator new`/`operator
  delete`, and the TSan runtime defines those symbols itself, so the TU does
  not link. The measurement is therefore its own executable
  (`all_of_allocation.test.cpp`), which leaves the behaviour tests --
  including the concurrent one this algorithm most needs checked -- buildable
  under TSan. Not a workaround for a defect in either tool: it is the shape
  the two requirements force, and a later stage adding allocation
  measurement should expect to pay it.

---

## all-of-failure-semantics

**Question:** When a child of `all_of` errors or stops, what does the whole
do, and what does result order mean?
**Status:** DECIDED 2026-09-13
**Decided by:** Default-as-drafted at Stage 0
[execution-baseline](transpose-execution-plan.md#execution-baseline), per
transpose-execution-plan.md §3 deliverable 1. Not individually ruled by Steve.
**Decision:** Follow `when_all`: the first error or stop to arrive requests
stop on all siblings and is reported once all children have completed;
error wins over stopped; among errors the first to arrive wins. Result
*order* is input order, never completion order. Traversal order
([applicative-objects](#applicative-objects), left-to-right)
governs `connect`/`start` order and result order; completion order is the
scheduler's business.
**Why:** `transpose` promises shape preservation; for a vector that is
positional. Matching `when_all` on failure means `transpose` over a
vector behaves exactly as the hand-written variadic form would for the
same children, which is the property the paper wants to state: `all_of`
*is* `when_all` at runtime arity, not a different algorithm.
**Log:**
- 2026-09-11 — Drafted. Stage 0 prior-art note to record where libunifex's
  `when_all_range` differs, if it does.
- 2026-09-13 — Merged at Stage 0, default-as-drafted. The prior-art survey
  this entry asked for is at
  [docs/review/prior-art-when-all-range.md](review/prior-art-when-all-range.md);
  where it bears on the choices above, it is cited there rather than
  restated here. Two results belong in the log rather than only the note.
  * THIS ENTRY PICKED THE STANDARD OVER THE ONLY IMPLEMENTATION, CORRECTLY.
    libunifex's `when_all_range` — the sole prior range-of-senders join that
    exists anywhere — shares ONE atomic flag between `set_error` and
    `set_done`, so a stopped child arriving first masks a later error and the
    whole completes stopped. P2300's `when_all` does the opposite on purpose:
    error uses `exchange` and out-ranks a recorded `stopped`, while stopped
    uses a compare-exchange and only wins from `started`. This entry's
    "error wins over stopped" is the standard's rule, not libunifex's. The
    Why ("`all_of` *is* `when_all` at runtime arity") is what made the
    difference, and it made it the right way round.
  * THE PLAN'S COMPLETION SIGNATURES ARE ONE REVISION BEHIND THE STANDARD.
    [transpose-execution-plan.md §3](transpose-execution-plan.md#all-of-algorithm)
    deliverable 1 lists `set_stopped_t()` unconditionally. P3887R1 — "Make
    `when_all` a Ronseal Algorithm", LWG-approved 2025-11 — says `when_all`
    advertises `set_stopped` only if a child does; P4269R0 pursues the same
    at the implementation level. Because this entry defines `all_of` by
    reference to `when_all`, following the `when_all` that was actually
    approved means making that signature CONDITIONAL on the children. A What,
    and one whose Why argues for the change: an `all_of` that hallucinates a
    stopped completion is not `when_all` at runtime arity. Stage 2 should
    treat the plan's unconditional list as superseded and say so in its own
    review note; this is not a licence to change anything else in the
    signature set.
- 2026-09-13 — Kept by Steve's ruling, unchanged. The algorithm these
  semantics govern lands as `examples/all_of.hpp` under
  `BEMAN_TRANSPOSE_BUILD_P2300_EVIDENCE`; the failure and ordering rules are
  unaffected by where the header sits.
- 2026-09-14 — IMPLEMENTED, with the P3887R1 correction applied and checked.
  `examples/all_of.hpp` follows `when_all` as this entry directs: the first
  error or stop to ARRIVE requests stop on all siblings and is reported once
  every child has completed; error outranks stopped, spelled the way P2300's
  own `when_all` spells it -- `set_error` uses `exchange` and so claims the
  slot even from a recorded `stopped`, while `set_stopped` uses a
  compare-exchange and only claims it from `started`. That asymmetry is the
  reason a stopped child cannot mask a later error, and it is exactly where
  libunifex's `when_all_range` differs, sharing one flag between the two;
  the prior-art note records the difference and this entry's Why is what
  chose against it.
  `set_stopped_t()` is advertised ONLY if a child sends it, per P3887R1
  (LWG-approved 2025-11), pinned both ways in `all_of.test.cpp`: a vector of
  `just` senders does not advertise it, a vector of stoppable senders does.
  Result order is input order, checked against sixteen children completing on
  their own threads in deliberately reversed order, TSan clean. `n == 0`
  completes immediately with an empty vector; `n == 1` is not special-cased.

---

## demo-sender-fate

**Question:** Does the `std::function` demonstration `sender<T>` stay?
**Status:** WITHDRAWN 2026-09-13. Superseded by
[p2300-front-door-shape](#p2300-front-door-shape), which ruled on this on
2026-09-11.
**Decided by:** Steve Downey, 2026-09-11, under the superseding entry;
withdrawal 2026-09-13.
**Decision:** There is no separate answer here.
[p2300-front-door-shape](#p2300-front-door-shape) already says it: "the
demonstration `sender<T>` in `include/beman/transpose/sender.hpp` stays, and
its header comment now says what it does and does not show." Both halves were
already done when this was drafted — the type is untouched and the header
comment carries its WHAT IT SHOWS and AND WHAT IT HIDES paragraphs. The slug
is kept so existing links redirect rather than dangle.
**What the draft would have added, and where it went instead:** one word,
"unconditional" — that the demonstration sender is reachable with no
dependency on an execution implementation at all. That claim is now pinned,
by `tests/beman/transpose/demo_sender_golden.test.cpp`, a translation unit
whose only library include is `sender.hpp` and which is built in every
configuration. Under the 2026-09-13 ruling the claim is not at risk from this
plan anyway — nothing execution-dependent enters `include/`, so
`transpose.hpp` cannot acquire the dependency — but the golden is the sensor
that keeps it true rather than merely intended.
**Log:**
- 2026-09-11 — Drafted PROPOSED in the execution plan's addendum, without the
  same-day ruling in view.
- 2026-09-13 — Pinned at Stage 0
  [execution-baseline](transpose-execution-plan.md#execution-baseline) by
  `demo_sender_golden.test.cpp`. The front-door DEDUCTIONS were already
  golden in `baseline_deduction.test.cpp`, which is likewise built in every
  configuration, so they are deliberately not repeated there.
- 2026-09-13 — WITHDRAWN by Steve's ruling: already ruled, nothing left to
  decide. The golden stays.

---

## execution-toolchain-floor

**Question:** Which compilers must the execution-dependent build work under,
given that the dependency's supported set and this repository's verified set
barely overlap?
**Status:** OPEN 2026-09-13
**Note:** Raised by stage
[execution-baseline](transpose-execution-plan.md#execution-baseline) as a
question no slug covered. This repository's log records verification on GCC
15.2, GCC 16 and Clang 23 ([lean-model-sync](#lean-model-sync),
[operand-value-category](#operand-value-category)). `beman.execution` at the
pinned commit documents GCC 15–14 and Clang 22–19, C++23 or greater. The
intersection is therefore GCC 14–15 and Clang 19–22 — Clang 23 and GCC 16,
the two this repository most recently verified on, are both OUTSIDE the
dependency's stated support, and the evidence build's real floor is
narrower than either project's alone.
Measured at Stage 0, and the reason this is a question rather than a note:
GCC 13.3 cannot build this library at all (no deducing-this, which every
typeclass base uses), and Clang 18.1.3 crashes the frontend on
`error_set.hpp` — so the floor is not merely a documentation preference, it
is a hard cliff one version below what the dependency's own matrix starts
at. A CI job proving the real instance (the follow-up
[p2300-front-door-shape](#p2300-front-door-shape) left open on 2026-09-11,
still open) has to pick from that narrow intersection, and the choice will
date faster than the rest of the build.
What would answer this: a decision on whether the execution-dependent build
tracks the dependency's matrix, this repository's, or the intersection, and
whether a compiler outside it is a build failure or a silently skipped
target.

---

## sender-error-grade

**Question:** Are a sender's error completion signatures a grade, and is
`when_all`'s error union the join?
**Status:** OPEN 2026-09-11
**Note:** `completion_signatures` records the set of `set_error_t(E)` a
sender may raise; `when_all` unions them; `let_value` unions them; a sender
that cannot error is at ∅. That is `error_set` with the join, computed by
the execution framework itself. If so, the grading machinery could read a
sender's grade from its signatures with no error_set materialized, and
subsumption would be the signature-widening senders already do. Recording
because it is a paper-worthy observation and because it is a temptation:
not in scope for the execution plan; [grading-footprint](#grading-footprint)
applies.

---

## sender-monad-instance

**Question:** Should P2300 senders get a Monad object (`bind = let_value`)?
**Status:** OPEN 2026-09-11
**Note:** Deferred, per the 2026-09-07 worklist: "same context, nested" is
a normalization problem for senders, not a trait fix. Revisit after
`collect` lands and after the monad-basis work has its sender-shaped
(demo) carrier tests.

---

## static-arity-array

**Question:** Should `std::array<S, N>` compose senders with a variadic
`when_all` directly, bypassing `all_of`?
**Status:** OPEN 2026-09-11
**Note:** Static arity makes the fold spellable as a pack expansion, so no
allocation at all is needed. Worth doing for the contrast (static shape →
zero allocations; runtime shape → one), possibly as `collect` on
`array<S, N>` rather than `vector<S>`. Not required by the plan.

---

## explicit-parameter-tier-sufficiency

**Question:** Does the explicit-parameter tier carry real work on its own, or
does a consumer that adopts the design end up needing the typeclass-lookup
tier?
**Status:** OPEN — one field data point recorded below; the question is not
settled by it.
**Recorded by:** Steve, from a usage report filed as issue #34.
**Observation:** [specgen](https://github.com/steve-downey/specgen), a
Clang-based generator for C++ standard-library specification wording, adopted
the `traverse`-shaped validation and monoidal-diagnostics ideas from this
repository and took **only** the explicit-parameter tier: every verb receives
its operations as ordinary arguments. No CRTP typeclass base, no
`*_typeclass` lookup, no `Foldable`/`Traversable` specialization.

It took the ideas in its own spellings, not this library's API. `mconcat(range,
monoid{combine, identity})` and `fold_with` are specgen's names; neither
exists here. The local analogue is the `*_with` family — `traverse_with` and
`transpose_with` (`include/beman/transpose/traverse.hpp`), `invoke_with`
(`include/beman/transpose/apply.hpp`) — together with the monoid objects.

It was sufficient, and reportedly not by a narrow margin.
**Why it may matter:** The reason given is scale. The tool has on the order of
four functor-shaped types and ten call sites over them. At that size an
explicit `{combine, identity}` pair passed by hand costs less than a lookup
framework and reads at the call site with one less level of indirection. The
diagnostics monoid is a production consumer — a validator folding over an IR
tree — so this is not a toy measurement.

The load-bearing part is the threshold, not the preference. Promoting an
explicit call site to a lookup later is mechanical, which is what made
deferring free. That is evidence *for* the two-tier shape: the tiers are
orderable, and a consumer can start at the cheap one and stay there as long as
it fits. It is not evidence that the lookup tier is unnecessary — specgen
never reached the scale that motivates it.
**Consequences:** One consumer's adoption path cost it nothing to skip the
lookup tier, so if that tier is a maintenance burden, this is not a consumer
who would notice it being deferred. A single data point at one scale does not
generalize; what would move the question is a consumer at ten or more
functor-shaped types reporting either way.
**Log:**
- 2026-09-13 — Recorded from issue #34. Upstream provenance: OBS-2 in
  specgen's `foundation/DIVERGENCES.md`, and decision D16 in its refactoring
  plan. No change to this library follows from it.
