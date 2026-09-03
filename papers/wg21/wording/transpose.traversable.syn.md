::: add

```cpp
template<class Impl>
struct Traversable : protected Impl {
  // Alternate-core: Impl::traverse is the primitive; transpose is derived
  // from it. A transpose-primitive Impl would shadow transpose instead.
  using Impl::traverse;
  using element_type = typename Impl::element_type;

  // @[transpose.traversable.ops]{- .sref}@, traversal operations
  template<class T, class F> auto for_each(this auto&& self, T&& value, F&& function);

  template<class T> auto transpose(this auto&& self, T&& value);

  // @[transpose.traversable.delegate]{- .sref}@, delegated traversal
  template<class TRAVERSABLE_MAP, class T, class F>
  auto traverse_with(this auto&&, const TRAVERSABLE_MAP& traversable_map, F&& function,
                     T&& value);

  template<class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
  auto traverse_with(this auto&&, const TRAVERSABLE_MAP& traversable_map,
                     const APPLICATIVE_MAP& applicative_map, F&& function, T&& value);

  template<class TRAVERSABLE_MAP, class T>
  auto transpose_with(this auto&& self, const TRAVERSABLE_MAP& traversable_map,
                      T&& value);
};
```

::: wording

[x]{.pnum} A program that instantiates `Traversable<Impl>` is ill-formed unless `is_same_v<Impl, false_type>` is `false` and `requires { typename Impl::element_type; }` is `true`.

:::

:::
