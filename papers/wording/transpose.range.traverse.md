::: add

::: wording

## Traversable instance for vector [transpose.range.traverse]{- .sref} {-}

```cpp
template<class APPLICATIVE, class FUNCTION>
auto traverse(this auto&&, const APPLICATIVE& applicative, FUNCTION&& function,
              const vector<VALUE_TYPE>& values);
```

[x]{.pnum} *Effects*: Let `E` be the type `function` returns for an element of `values`. Applies `function` to each element of `values` in order. If `applicative.collect(v)` is a valid expression for an rvalue `vector<E>` `v`, composes the resulting contextual values by passing all of them to that one call; otherwise composes them pairwise with `applicative`, collecting the element results into a `vector`.

[x+1]{.pnum} *Returns*: A `vector` of the element results, of the same size as `values` and in the same order, held in the single context `applicative` composes into.

[x+2]{.pnum} *Complexity*: Exactly `values.size()` applications of `function`. Where `applicative` has no `collect`, `values.size()` composition operations on `applicative`, and where it composes an rvalue operand without duplicating the value it holds -- as every pairwise applicative object this library registers does -- the total number of element operations is linear in `values.size()`. Where `applicative` has `collect`, exactly one composition operation, whose own complexity is that operation's to state.

[x+3]{.pnum} *Remarks*: Traversal preserves shape: the result holds one element per element of `values`, in the same order. Elements are visited in the vector's iteration order, and their contexts are composed in that same order. A native `collect` is preferred because a runtime-sized pairwise fold requires every partial composition to have one invariant type, while some contexts name a new type at each composition. On the pairwise path, the accumulated result is handed to each composition as an rvalue: composing it as an lvalue would oblige `applicative` to copy the whole prefix built so far, once per element, making a successful traversal quadratic rather than linear in the elements it collects.

```cpp
template<class APPLICATIVE, class FUNCTION>
auto traverse(this auto&&, const APPLICATIVE& applicative, FUNCTION&& function,
              vector<VALUE_TYPE>&& values);
```

[x+4]{.pnum} *Effects*: Equivalent to the preceding overload, except that each element of `values` is passed to `function` as an rvalue.

[x+5]{.pnum} *Returns*: As the preceding overload.

[x+6]{.pnum} *Complexity*: As the preceding overload.

[x+7]{.pnum} *Remarks*: This overload is what makes a traversal that consumes its argument -- `transpose(std::move(v))` -- express that intent through to `function`, and what admits an element type that can be moved but not copied. `values` is left in a valid but unspecified state.

:::

:::
