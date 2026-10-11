# Stage review — sender-registration

Stage [sender-registration](../transpose-execution-plan.md#sender-registration),
the second stage of [transpose-execution-plan.md](../transpose-execution-plan.md).
Run 2026-09-13.

**Status: COMPLETE.** All four deliverables done, suite green on both presets
in both option states, `include/` untouched, goldens untouched.

This stage was an audit, not a build — Steve's ruling of 2026-09-13 turned it
into one after Stage 0 found that `examples/p2300_adapter.hpp` already
supplied the basis the plan had scheduled as new work. The audit found the
basis correct and the two gaps the adapter's own header comment had already
declared.

---

## 1. What changed

**`examples/p2300_adapter.hpp`** gained three things and lost nothing:

- `single_value_sender` — `ex::sender<S>` plus exactly one value completion
  of exactly one argument, read under `env<>`.
- `applicative_value<S>` for those senders, reading
  `value_types_of_t<S, env<>, type_identity_t, type_identity_t>` decayed, and
  constrained to senders *without* a `value_type` member so it stays disjoint
  from the framework's primary path.
- A constrained partial specialization of `applicative_typeclass<S>` — one
  object, `P2300ApplicativeMap`, for every registered sender type.

The `pure`/`invoke` basis is unchanged; the audit confirmed it forwards
operands rather than taking `const&`, which is what
[operand-value-category](../decisions.md#operand-value-category) requires and
what the move-only test now exercises through the registration.

**`tests/beman/transpose/test_support.hpp`** — the four applicative law
helpers gained a defaulted observation parameter. Existing call sites are
unchanged and behave identically; a sender passes an observer that runs the
context under `sync_wait`. **One statement of each law**, which is the point:
a parallel sender-flavoured law suite could drift from the one `optional` is
checked against, and this cannot.

**`tests/beman/transpose/p2300.test.cpp`** — 254 lines of audit assertions
and five new `TEST_CASE`s.

---

## 2. Findings

Six, logged in full under
[sender-instance-keying](../decisions.md#sender-instance-keying) as the ruling
directs. Summarised here in the order they mattered; the sixth is in section 4,
because it is less a finding about this stage than a bill Stage 3 inherits.

**(a) A Stage 0 conclusion was wrong, and this stage paid for it first.**
Stage 0 measured that `value_types_of_t<…, type_identity_t, type_identity_t>`
is ill-formed for a two-argument sender — true — and concluded that a
`requires` clause naming the alias is therefore "a hard error, not a graceful
constraint failure". **That does not follow**, and the probe Stage 0 wrote to
support it already disproved it: ill-formed *inside a requires-expression* is
a constraint failure. `single_value_sender` is spelled directly in terms of
the alias with no pre-check, and five negatives — `just(1, 2)`, `just()`,
`just_error(...)`, `just_stopped()`, a raw `when_all` — evaluate to false and
compile. Corrected in the log and in the Stage 0 review note rather than
quietly dropped, because the wrong version would have cost the next agent an
afternoon building a pre-check nothing needs.

**(b) The keying works as decided, and the sentinel holds.** One object is
reached from every registered sender type — `just(1)`, a `then`-adapted
sender and `just(std::string{})` all resolve to the same object *type*,
asserted rather than assumed. `pure(1)` returns `decltype(just(1))`, which is
not the `S` the object was found under; that is pinned because it is the
concrete reason the object cannot be templated on `S`.

**(c) The demonstration sender is not an `ex::sender`.** Checked, not
assumed. The feared ambiguity between this concept-keyed registration and
`sender.hpp`'s per-type one does not arise. Pinned, so that if it ever does,
the keying decision's tripwire applies — resolve by subsumption, never by a
tie-breaker tag.

**(d) A raw `when_all` is not an Applicative element.** This surprised the
stage and is correct. `when_all(just(1), just(2))` completes with *two* value
arguments, fails the concept, and is unregistered. The adapter's `invoke` is
`when_all(...) | then(f)`, and the `then` collapses the pack back to one
value — so the *result* of `invoke` is an element even though its middle term
is not. Both directions pinned, because "fixing" the negative would break the
arity contract `when_all` and
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) both
depend on.

**(e) Registration makes `transpose(vector<S>)` fail *worse*.** The finding
to carry to Stage 3.

| | Diagnostic |
|---|---|
| Unregistered sender element | clean `no matching function for call to 'transpose'`, at the call site |
| Registered sender element | dies inside `sequence.hpp` at `accumulated = applicative.invoke(...)` — *"no viable overloaded `=`"*, three-frame backtrace |

The message is honest: that assignment **is** the invariance the fold needs
and a real sender does not have. But it is a regression in message quality
that registration bought, and it is not fixed here — the fix is the `collect`
hook, which is Stage 3's deliverable, and Stage 1's acceptance forbids making
the vector front door work early.

---

## 3. Acceptance

| Requirement | Result |
|---|---|
| Deliverable 1 — audit adapter against keying + value reading, close gaps | Done; gaps logged under the keying slug |
| Deliverable 2 — laziness, n-ary 2/3/5, lifted callable, move-only, further pipelines | Done; 2 and 5 and the pipeline case added, the rest already present |
| Deliverable 3 — law harness with `sync_wait` as the observation | Done, through the shared helpers rather than a parallel suite |
| Deliverable 4 — `applicative_object<obj, S>` for `just(1)` and a `then`-adapted sender | Done; both hold |
| Goldens unchanged | `baseline_deduction.test.cpp` byte-identical to `main` |
| No `transpose` over a vector yet | Confirmed — it does not work |
| Nothing execution-dependent under `include/` | `git diff main..HEAD -- include` is empty |

| Preset | Option OFF | Option ON |
|---|---|---|
| gcc-debug | 240 | 251 |
| llvm-debug | 240 | 251 |

Before this stage: 240 / 247. The option-OFF count is **unchanged**, which is
the `include/` purity constraint showing up as a number: nothing this stage
added is reachable without asking for the dependency.

**Tripwires.** None fired. `when_all` rejected no operand the registration
admits. `applicative_value` is unambiguous — measured, and the exclusion that
keeps it so is currently vacuous, which is why it is worth keeping as a
sensor. Nothing execution-dependent reached `include/`.

---

## 4. What Stage 3's agent needs that the plan does not say

**`applicative_object` holds for every sender shape; the *policy* concept
holds for exactly one, and that is Stage 3's first obstacle.** Deliverable 4's
concept does not constrain what `pure` returns. `traverse`'s policy concept
does: per
[typeclass-conformance-depth](../decisions.md#typeclass-conformance-depth),
`applicative_object_for<POLICY, CONTEXT>` adds
`pure(element) -> std::same_as<CONTEXT>`. Measured, not reasoned:

| `CONTEXT` | `applicative_object` | `applicative_object_for` |
|---|---|---|
| `decltype(just(1))` | yes | **yes** |
| a `then`-adapted sender | yes | **no** |
| `when_all(…) \| then(…)` | yes | **no** |

`pure(x)` is always `just(x)`, so the refinement is satisfied only when
`CONTEXT` happens to *be* `decltype(just(x))` — and fails for every adapted
sender, which is what a caller actually holds. This is finding (b) showing up
as a consequence rather than a virtue: one object for all sender types means
`pure` returns a type of its own choosing, and the policy concept asks for the
opposite.

So Stage 3 cannot reach the vector front door by satisfying the existing
policy concept. Either the `collect` hook is probed *before* that refinement
is applied, or the refinement relaxes for objects that supply `collect`.
**The plan does not raise this, and it is the first thing Stage 3 will hit.**
All three rows are pinned in `p2300.test.cpp` so the choice is made against
measurements rather than against this paragraph.

**The `collect` hook's acceptance has a free extra signal.** Finding (e) gives
Stage 3 a before-state it can assert against: today `transpose(vector<S>)`
produces a hard error inside `sequence.hpp`. After the hook it should produce
either a working call or a clean constraint failure. Either is an improvement
and both are checkable; the current state is checkable only by compiling and
reading, which is why there is no negative `static_assert` for it.

**The law helpers now take an observer; use it rather than widening it.**
`observe_directly` is the default and every existing caller keeps it. If Stage
3 needs to check a law over a *transposed* sender, the observer is already the
right seam — do not add a second comparison mechanism beside it.

**Arity is contagious.** Finding (d) means any intermediate that widens a
sender past one value argument leaves the registered set. `all_of` must
complete with exactly one `std::vector<T>` — which
[all-of-failure-semantics](../decisions.md#all-of-failure-semantics) already
requires — or its result will not itself be an Applicative element, and
`transpose` of a transposed structure stops composing.
