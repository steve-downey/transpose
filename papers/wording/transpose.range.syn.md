::: add

```cpp
template<class VALUE_TYPE>
struct $vector-traversable-impl$ {
  using element_type = VALUE_TYPE;
  static constexpr bool consumes_rvalue_structure = true;

  // @[transpose.range.traverse]{- .sref}@, traversable instance for vector
  template<class APPLICATIVE, class FUNCTION>
  auto traverse(this auto&&, const APPLICATIVE& applicative, FUNCTION&& function,
                const vector<VALUE_TYPE>& values);

  template<class APPLICATIVE, class FUNCTION>
  auto traverse(this auto&&, const APPLICATIVE& applicative, FUNCTION&& function,
                vector<VALUE_TYPE>&& values);
}; // exposition only
```

::: wording

[x]{.pnum} *Remarks*: A `vector` handed over as a non-`const`, non-`volatile` rvalue owns its elements and this instance presents each element to the traversal function as an rvalue. The declaration is what permits traversal to infer the consuming function's context before selecting this object.

:::

```cpp
template<class VALUE_TYPE>
struct $vector-traversable-map$
    : derive_traversable<$vector-traversable-impl$<VALUE_TYPE>> {
  using $vector-traversable-impl$<VALUE_TYPE>::traverse;
}; // exposition only
```

:::
