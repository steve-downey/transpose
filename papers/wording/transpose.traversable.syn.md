::: add

```cpp
template<class CONTEXT>
concept $applicative-context$ =
    applicative_object<remove_cvref_t<decltype(applicative_typeclass<CONTEXT>)>,
                       CONTEXT>; // exposition only
```

```cpp
template<class OBJ>
concept $consuming-traversable-object$ =
    requires { requires bool(OBJ::consumes_rvalue_structure); }; // exposition only
```

::: wording

[x]{.pnum} The concept `$consuming-traversable-object$<OBJ>` is satisfied when `OBJ` declares `consumes_rvalue_structure` to be `true`. Such an object, when given a non-`const`, non-`volatile` rvalue structure, presents each element to the traversal function as an rvalue. An object that does not make the declaration presents elements as `const` lvalues.

[x+1]{.pnum} *Remarks*: Consumption is an instance capability rather than a property of every structure. An owning structure can hand its elements over when it is handed over. A persistent structure that shares its representation cannot in general do so without first copying each element into a temporary.

:::

```cpp
template<class Impl>
struct Traversable : protected Impl {
  // Alternate-core: Impl::traverse is the primitive; transpose is derived
  // from it. A transpose-primitive Impl would shadow transpose instead.
  using Impl::traverse;
  using element_type = typename Impl::element_type;
  static constexpr bool consumes_rvalue_structure =
      $consuming-traversable-object$<Impl>;

  // @[transpose.traversable.ops]{- .sref}@, traversal operations
  template<class T, class F> auto for_each(this auto&& self, T&& value, F&& function);

  template<class T>
    requires $applicative-context$<typename Impl::element_type>
  auto transpose(this auto&& self, T&& value);

  // @[transpose.traversable.delegate]{- .sref}@, delegated traversal
  template<class TRAVERSABLE_MAP, class T, class F>
  auto traverse_with(this auto&&, const TRAVERSABLE_MAP& traversable_map, F&& function,
                     T&& value);

  template<class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
  auto traverse_with(this auto&&, const TRAVERSABLE_MAP& traversable_map,
                     const APPLICATIVE_MAP& applicative_map, F&& function, T&& value);

  template<class TRAVERSABLE_MAP, class T>
    requires $applicative-context$<typename remove_cvref_t<TRAVERSABLE_MAP>::element_type>
  auto transpose_with(this auto&& self, const TRAVERSABLE_MAP& traversable_map,
                      T&& value);
};
```

::: wording

[x+2]{.pnum} A program that instantiates `Traversable<Impl>` is ill-formed unless `is_same_v<Impl, false_type>` is `false` and `requires { typename Impl::element_type; }` is `true`.

[x+3]{.pnum} `Traversable<Impl>::consumes_rvalue_structure` re-exports the capability declared by `Impl`. If `Impl` makes no declaration, its value is `false`.

:::

::: wording

```cpp
template<class T> inline constexpr auto traversable_typeclass = false_type{};
```

[x+4]{.pnum} *Remarks*: This variable template is the lookup point for the Traversable object of a structure type. A program may specialize it for a program-defined structure. The primary template names no traversable object.

:::

```cpp
template<class T>
using $traversable-object-t$ =
    remove_cvref_t<decltype(traversable_typeclass<remove_cvref_t<T>>)>; // exposition
                                                                        // only
```

```cpp
template<class T>
using $structure-element-t$ =
    typename $traversable-object-t$<T>::element_type; // exposition only
```

```cpp
template<class OBJ, class T>
using $traversal-argument-t$ = conditional_t<
    !is_lvalue_reference_v<T> &&
        !is_const_v<remove_reference_t<T>> &&
        !is_volatile_v<remove_reference_t<T>> &&
        $consuming-traversable-object$<remove_cvref_t<OBJ>>,
    typename remove_cvref_t<OBJ>::element_type&&,
    const typename remove_cvref_t<OBJ>::element_type&>; // exposition only
```

::: wording

[x+5]{.pnum} `$traversal-argument-t$<OBJ, T>` is the type through which the Traversable object `OBJ` presents an element when handed a structure of type and value category `T`. It is an rvalue reference only for a non-`const`, non-`volatile` rvalue structure and an object satisfying `$consuming-traversable-object$`; otherwise it is a `const` lvalue reference.

:::

```cpp
template<class T>
using $traverse-element-t$ =
    $traversal-argument-t$<$traversable-object-t$<T>, T>; // exposition only
```

```cpp
template<class F, class T>
using $traverse-context-t$ =
    remove_cvref_t<invoke_result_t<F&, $traverse-element-t$<T>>>; // exposition only
```

```cpp
template<class T>
concept $transposable-structure$ = requires {
  typename $structure-element-t$<T>;
} && $applicative-context$<$structure-element-t$<T>>; // exposition only
```

```cpp
template<class APPLICATIVE, class CONTEXT>
concept $collecting-applicative$ = requires(const APPLICATIVE& applicative) {
  applicative.collect(declval<vector<CONTEXT>>());
}; // exposition only
```

::: wording

```cpp
template<class POLICY, class CONTEXT>
concept applicative_object_for =
    applicative_object<POLICY, CONTEXT> &&
    (requires(const POLICY& policy) {
       { policy.pure(declval<applicative_value_t<CONTEXT>>()) } -> same_as<CONTEXT>;
     } || $collecting-applicative$<POLICY, CONTEXT>);
```

[x+6]{.pnum} *Remarks*: This concept is satisfied when `POLICY` is an `applicative_object` over `CONTEXT` that can compose a structure of `CONTEXT` into one `CONTEXT` of the structure. The pairwise route requires `pure` to return exactly `CONTEXT` from `CONTEXT`'s element type. The native route is available when `POLICY` supplies `collect` for a `vector<CONTEXT>` and does not require the pairwise route also to be available. A traversal prefers the native route when both exist.

:::

```cpp
template<class OBJ>
concept $transposing-object$ = requires { typename OBJ::element_type; } &&
    $applicative-context$<typename OBJ::element_type>; // exposition only
```

::: wording

```cpp
template<class IMPL, class STRUCTURE>
concept traversable_impl =
    requires(const IMPL& impl, const STRUCTURE& structure) {
      typename IMPL::element_type;
      impl.traverse(
          applicative_typeclass<optional<applicative_value_t<STRUCTURE>>>,
          $probe-witness$<optional<applicative_value_t<STRUCTURE>>>{}, structure);
    } &&
    (!$consuming-traversable-object$<IMPL> || requires(const IMPL& impl) {
      impl.traverse(
          applicative_typeclass<optional<applicative_value_t<STRUCTURE>>>,
          $consuming-probe-witness$<applicative_value_t<STRUCTURE>,
                                      optional<applicative_value_t<STRUCTURE>>>{},
          declval<STRUCTURE>());
    });
```

[x+7]{.pnum} *Remarks*: This concept is satisfied when `IMPL` supplies the minimal complete basis the `Traversable` CRTP base needs: a declared `element_type` and `traverse`, probed with a representative witness callable that lifts an element into `optional`. An `IMPL` satisfying `$consuming-traversable-object$` is additionally probed on an rvalue structure with a witness accepting an element only as an rvalue. An `IMPL` that does not declare consumption is not required to support that probe.

:::

::: wording

```cpp
template<class OBJ, class STRUCTURE>
concept traversable_object =
    requires(const OBJ& obj, const STRUCTURE& structure) {
      obj.traverse(
          applicative_typeclass<optional<applicative_value_t<STRUCTURE>>>,
          $probe-witness$<optional<applicative_value_t<STRUCTURE>>>{}, structure);
      obj.for_each(
          structure, $probe-witness$<optional<applicative_value_t<STRUCTURE>>>{});
      obj.traverse_with(
          obj, $probe-witness$<optional<applicative_value_t<STRUCTURE>>>{}, structure);
    } &&
    (!$consuming-traversable-object$<OBJ> || requires(const OBJ& obj) {
      obj.traverse(
          applicative_typeclass<optional<applicative_value_t<STRUCTURE>>>,
          $consuming-probe-witness$<applicative_value_t<STRUCTURE>,
                                      optional<applicative_value_t<STRUCTURE>>>{},
          declval<STRUCTURE>());
    }) &&
    (!$transposing-object$<OBJ> || requires(const OBJ& obj,
                                             const STRUCTURE& structure) {
      obj.transpose(structure);
      obj.transpose_with(obj, structure);
    });
```

[x+8]{.pnum} *Remarks*: This concept is satisfied when `OBJ` provides the full Traversable object surface over `STRUCTURE`. An object declaring consumption is additionally required to accept an rvalue structure with a witness that accepts only an rvalue element. `transpose` and `transpose_with` are required only when `OBJ` satisfies `$transposing-object$`. This concept does not require a Foldable object.

:::

:::
