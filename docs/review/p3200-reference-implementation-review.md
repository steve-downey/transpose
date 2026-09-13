# Review of the P3200 Reference Implementation

Date: 2026-09-11
Reviewed commit: `9359461` (`main`)

## Executive summary

The implementation demonstrates a coherent central abstraction and has an
unusually extensive test suite. The same `traverse` and `transpose` front door
works for the project's optional, deferred, and lanewise examples, and the
typeclass-object implementation is exercised by both behavioral and law-based
tests.

It is not yet suitable as a publication-ready reference implementation,
however. Three defects directly contradict proposed normative properties:

1. Successful vector traversal has quadratic rather than linear complexity.
2. Deferred sender effects are not sequenced left-to-right and run in reverse
   order with GCC 16.
3. `transpose` does not encode its stated constraint and hard-errors during
   detection.

Several additional issues impose undocumented construction, copyability, and
callability requirements. The accompanying paper is also still explicitly an
outline skeleton, and the sender-shaped demonstration does not validate
interoperation with P2300 senders.

I recommend resolving the three high-severity findings, adding the focused
regression tests described below, and reconciling the specification's
short-circuit language before presenting the implementation as conformance
evidence.

## Scope and method

This review covered:

- the proposed `Applicative` and `Traversable` customization surfaces;
- the `traverse` and `transpose` front-door algorithms;
- the `std::vector`, `std::optional`, `std::expected`, array, sender-like,
  zip-list, and SIMD-lane implementations;
- the generated wording under `papers/wording/` and its source comments;
- relevant design records and existing review material; and
- the build, behavioral tests, examples, and focused compile-time/runtime
  probes.

The review treats header comments as specification sources because
`papers/D3200R0.md` states that normative wording is generated from the
headers.

## Findings

### High: vector traversal violates the linear complexity guarantee

`VectorTraversableImpl::traverse` initializes an in-context empty vector, then
combines every element with the accumulated vector:

```cpp
auto accumulated = applicative.pure(std::vector<Element>{});
for (const auto& value : values) {
    accumulated = applicative.invoke(
        [](std::vector<Element> collected, const Element& element) {
            collected.push_back(element);
            return collected;
        },
        accumulated, std::invoke(function, value));
}
```

The optional and expected Applicative instances expose their operands as
`const&`. Constructing the lambda's by-value `collected` argument therefore
copies every element accumulated so far. A successful traversal performs
roughly `0 + 1 + ... + (n - 1)` element copies, making it quadratic.

A focused probe over 100 `optional<X>` elements counted 5,150 copies. This
contradicts the `transpose` wording's unconditional "Linear in the number of
elements" complexity guarantee.

Locations:

- `include/beman/transpose/sequence.hpp:118`
- `include/beman/transpose/apply.hpp:683`
- `include/beman/transpose/expected.hpp:453`
- `include/beman/transpose/transpose.hpp:53`

Recommended action:

- Introduce a traversal/building strategy that can move or mutate the
  accumulated structure without copying its entire prefix.
- Reconsider whether the generic front door can promise linear complexity for
  every user-provided Traversable instance.
- Add a copy-counting regression test that distinguishes linear from
  quadratic construction.

### High: sender effects do not obey left-to-right order

The deferred sender implementation runs its operands as arguments to
`std::invoke`:

```cpp
return std::invoke(function, first.get(), rest.get()...);
```

C++ does not specify the evaluation order of function arguments. The calls to
`get()` are indeterminately sequenced, but their relative order is unspecified.
This cannot implement a normative left-to-right composition guarantee.

A focused GCC 16 probe transposed senders numbered `1`, `2`, `3`, `4`. Their
returned values retained the correct structure, but their observable effects
ran in the order `4`, `3`, `2`, `1`.

Location:

- `include/beman/transpose/sender.hpp:76`

Recommended action:

- Evaluate each sender into explicitly sequenced storage before invoking the
  callable, for example by building a result tuple through a left-to-right
  construction and then using `std::apply`.
- Add tests that record operand execution order for direct `invoke` and for
  `transpose(vector<sender<T>>)`. Counting executions alone does not detect
  this defect.

### High: `transpose` does not implement its stated constraint

The generated wording says that `transpose` is constrained on registered
Traversable and Applicative objects, but its declaration is unconstrained:

```cpp
template <class T>
auto transpose(T&& value) {
    const auto& map = traversable_typeclass<remove_cvref_t<T>>;
    return map.transpose(std::forward<T>(value));
}
```

Because the return type is deduced from the function body, attempting to detect
`transpose` for a non-traversable type instantiates the body and fails with a
hard diagnostic. For example, a concept containing
`requires(T value) { transpose(value); }` cannot cleanly evaluate false for
`int`.

Locations:

- `include/beman/transpose/transpose.hpp:44`
- `include/beman/transpose/transpose.hpp:56`
- `papers/wording/transpose.alg.transpose.md`

Recommended action:

- Encode the stated requirement as an associated constraint and ensure its
  expressions do not instantiate an unconstrained deduced-return-type body.
- Add positive and negative detection tests around the public front door.

### Medium: Applicative probes require default-constructible result values

The representative concept witnesses return `RESULT{}`:

```cpp
template <class RESULT>
struct probe_witness {
    template <class ARGUMENT>
    constexpr auto operator()(const ARGUMENT&) const -> RESULT {
        return RESULT{};
    }
};
```

Although these witnesses are used in requires-expressions, probing derived
operations with deduced `auto` return types instantiates enough of their bodies
to instantiate the witness call. A context such as
`optional<NonDefaultConstructible>` is consequently rejected by
`applicative_object_for`, even when all real operations are otherwise valid.
No default-construction requirement appears in the wording.

Locations:

- `include/beman/transpose/detail/typeclass_base.hpp:85`
- `include/beman/transpose/apply.hpp:299`
- `include/beman/transpose/traverse.hpp:113`

Recommended action:

- Make the witness establish only a result type, without constructing a
  `RESULT`.
- Add concept tests using a copyable, non-default-constructible value.

### Medium: forwarding-reference front doors do not provide consuming traversal

The public algorithms accept and forward `T&&`, but the vector Traversable
primitive accepts only `const std::vector<VALUE_TYPE>&`. Optional and expected
Applicative primitives likewise accept operands through `const&`.

As a result, even this consuming-looking call is ill-formed:

```cpp
std::vector<std::optional<std::unique_ptr<int>>> values;
auto result = transpose(std::move(values));
```

The implementation attempts to copy the optional and its `unique_ptr`. The
same const-only path also contributes to the quadratic behavior described
above.

Locations:

- `include/beman/transpose/sequence.hpp:111`
- `include/beman/transpose/apply.hpp:683`
- `include/beman/transpose/expected.hpp:453`
- `include/beman/transpose/traverse.hpp:204`

Recommended action:

- Add consuming/rvalue paths through Traversable and the applicable contexts,
  or change the public signatures and wording to expose the actual copy-only
  contract.
- State all copyability requirements explicitly if move-only values are out of
  scope.

### Medium: callable detection and invocation use different value categories

`traverse_context_t` and several instance implementations use
`std::invoke_result_t<F, ...>`. When a temporary callable is passed, this tests
invocation on an rvalue. The implementation later invokes the named `function`
variable as an lvalue.

A temporary mutable function object with only `operator() &` is rejected even
though the loop would invoke it in precisely that supported state. Conversely,
an `&&`-only callable may appear viable in type computation and then fail in
the body.

Locations include:

- `include/beman/transpose/traverse.hpp:97`
- `include/beman/transpose/sequence.hpp:114`
- `include/beman/transpose/array.hpp:89`
- `include/beman/transpose/zip_list.hpp:133`
- `include/beman/transpose/simd_lanes.hpp:56`

Recommended action:

- Model the category actually used by the implementation, normally `F&` for a
  callable stored or repeatedly invoked as a named object.
- Add ref-qualified callable tests.

### Medium: array operations impose unnecessary construction requirements

Both `ArrayApplicativeImpl::pure` and `invoke` default-construct the output
array and then assign its elements. Consequently, they require result elements
to be default-constructible and assignable.

The documented operations only require copying the input value or constructing
each invocation result. A copy-constructible but non-default-constructible and
non-assignable type therefore fails despite satisfying the apparent semantic
requirements.

Locations:

- `include/beman/transpose/array.hpp:71`
- `include/beman/transpose/array.hpp:86`
- `include/beman/transpose/simd_lanes.hpp:35`
- `include/beman/transpose/simd_lanes.hpp:53`

Recommended action:

- Construct result arrays directly with an index sequence instead of
  default-constructing and assigning.
- Add non-default-constructible and non-assignable result tests.

### Medium: the short-circuit specification is internally inconsistent

The `traverse` wording makes both of these claims:

- `function` is applied exactly once per element; and
- the default policy "stops at the first failing element."

The implementation always evaluates `std::invoke(function, value)` before
calling the Applicative composition operation. A four-element probe whose
second element failed still called the function four times. Only reconstruction
inside the context stops; production of the remaining contextual values does
not.

Locations:

- `include/beman/transpose/traverse.hpp:264`
- `include/beman/transpose/traverse.hpp:267`
- `include/beman/transpose/sequence.hpp:119`

Recommended action:

- If eager independent composition is intended, replace "stops at the first
  failing element" with wording such as "retains the first failure" and make
  the before/after example's observable differences clear.
- If actual short-circuiting is intended, the generic Applicative interface
  needs a way to avoid producing subsequent effects; the current interface
  cannot express that generically.

### Evidence gap: the sender example is not a P2300 sender

The demonstration `sender<T>` is a `std::function<T()>` wrapper. It proves that
the front door can preserve laziness for a copyable, single-value deferred
computation. It does not exercise the defining constraints of P2300 senders:

- completion signatures;
- error and stopped channels;
- environments and scheduler interaction;
- multiple or zero value completions;
- one-shot, potentially non-copyable operation states; or
- connection and start lifetime rules.

This limitation is acknowledged in the source, but the example cannot support
a broad claim that the design has been validated for senders.

Location:

- `include/beman/transpose/sender.hpp:6`

Recommended action:

- Add an adapter for an actual P2300 implementation, even if it remains
  non-normative, and specify how error/stopped completions compose.
- Narrow the paper's evidence claim until that experiment exists.

### Documentation readiness

The paper describes itself as an outline skeleton and still contains literal
placeholder text in its Motivation and customization-mechanism sections. It
also defers the standard spellings and whether explicit object parameters
appear in the specification.

The README's opening example has a malformed code fence: the build section is
inserted between the introduction and `structure<context<T>>`, leaving an
extra closing fence. It also says `std::optional` is the only registered
standard context even though the current implementation registers
`std::expected` and `std::array`.

Locations:

- `papers/D3200R0.md:72`
- `papers/D3200R0.md:91`
- `papers/D3200R0.md:250`
- `README.md:19`
- `README.md:55`

These do not affect library execution, but they prevent the repository from
serving as a self-contained review package for the proposal.

## Test and tooling assessment

The existing suite is broad and valuable. It covers baseline type deductions,
Applicative and Monad laws, grade algebra, accumulation, cross-translation-unit
type identity, the three motivating front-door examples, and the customization
object machinery.

Verification performed for this review:

- GCC 16 build: successful.
- GCC 16 test run: all 227 configured tests passed.
- Clang 23 release workflow: successful.
- Clang 23 test run: all 225 configured tests passed.
- Focused compile/runtime probes reproduced the complexity, ordering,
  detection, construction, move-only, and callable-category findings.
- The local pre-commit run could not provision its Node environment because of
  a TLS certificate verification failure. This was an environment failure,
  not a reported repository lint failure.

The clean suite alongside the findings points to specific missing sensors:

- sender tests count executions but do not record their order;
- traversal tests compare values but do not measure construction complexity;
- front-door tests do not perform negative requires-expression detection;
- concept tests use regular, default-constructible scalar values;
- traversal tests do not cover consuming move-only contexts; and
- callable tests do not use ref-qualified function objects.

## Recommended order of work

1. Repair sender sequencing and add an observable-order regression test.
2. Replace the quadratic vector-building path and pin its complexity with a
   copy-counting test.
3. Make `transpose` properly constrained and detection-friendly.
4. Remove accidental default-construction and assignment requirements from
   concept probes and array construction.
5. Decide and document the value-category/copyability model, then align the
   signatures, constraints, and implementations with it.
6. Resolve the short-circuit wording contradiction.
7. Add real P2300 integration evidence or narrow the sender claim.
8. Complete the paper's placeholder sections and repair README drift.

## Conclusion

The implementation is a strong experimental vehicle: its central decomposition
of structure traversal from contextual composition is visible in the code, and
the law and grading work is substantially more rigorous than a typical early
prototype. The remaining blockers are concentrated in operational properties
that matter directly to a standard facility: complexity, sequencing,
constraints, and generic type requirements.

Addressing those blockers would materially strengthen both the implementation
and the proposal's evidence. Until then, the repository should be described as
an exploratory implementation rather than a conforming reference
implementation.
