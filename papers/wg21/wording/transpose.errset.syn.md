::: add

```cpp

```

```cpp
template<class... ERRORS>
struct type_list {};
```

```cpp

```

```cpp
template<class T, class... ERRORS>
struct list_prepend<T, type_list<ERRORS...>> {
  using type = type_list<T, ERRORS...>;
};
```

```cpp

```

```cpp
template<class T>
struct list_remove<T, type_list<>> {
  using type = type_list<>;
};
```

```cpp
template<class T, class HEAD, class... TAIL>
struct list_remove<T, type_list<HEAD, TAIL...>> {
public:
  using type =
      conditional_t<is_same_v<T, HEAD>, rest, list_prepend_t<HEAD, rest>>;
};
```

```cpp

```

```cpp
template<>
struct list_dedupe<type_list<>> {
  using type = type_list<>;
};
```

```cpp
template<class HEAD, class... TAIL>
struct list_dedupe<type_list<HEAD, TAIL...>> {
  using type = list_prepend_t<
      HEAD,
      typename list_dedupe<typename list_remove<HEAD, type_list<TAIL...>>::type>::type>;
};
```

```cpp

```

```cpp
template<class T>
struct list_insert_sorted<T, type_list<>> {
  using type = type_list<T>;
};
```

```cpp
template<class T, class HEAD, class... TAIL>
struct list_insert_sorted<T, type_list<HEAD, TAIL...>> {
  using type = conditional_t<
      type_precedes_v<T, HEAD>, type_list<T, HEAD, TAIL...>,
      list_prepend_t<HEAD, typename list_insert_sorted<T, type_list<TAIL...>>::type>>;
};
```

```cpp

```

```cpp
template<>
struct list_sort<type_list<>> {
  using type = type_list<>;
};
```

```cpp
template<class HEAD, class... TAIL>
struct list_sort<type_list<HEAD, TAIL...>> {
  using type =
      typename list_insert_sorted<HEAD,
                                  typename list_sort<type_list<TAIL...>>::type>::type;
};
```

```cpp

```

```cpp
template<class... ERRORS>
struct error_set_from_list<type_list<ERRORS...>> {
  using type = error_set_of<ERRORS...>;
};
```

```cpp
template<class T, class... ERRORS>
inline constexpr bool $error-set-has-v$ = (is_same_v<T, ERRORS> || ...); // exposition only
```

```cpp
template<class... ERRORS>
class error_set_of {
public:
  // @[transpose.errset.obs]{- .sref}@, observers
  template<class ERROR> static constexpr auto contains() noexcept -> bool;

  // @[transpose.errset.cons]{- .sref}@, constructors
  template<class ERROR>
    requires($error-set-has-v$<remove_cvref_t<ERROR>, ERRORS...>)
  constexpr error_set_of(ERROR&& error); // NOLINT(*-explicit-constructor)

  template<class... NARROWER>
    requires(sizeof...(NARROWER) > 0) &&
            (!is_same_v<error_set_of<NARROWER...>, error_set_of<ERRORS...>>) &&
            ($error-set-has-v$<NARROWER, ERRORS...> && ...)
  constexpr error_set_of(
      const error_set_of<NARROWER...>& narrower); // NOLINT(*-explicit-*)

  // @[transpose.errset.obs]{- .sref}@, observers
  template<class ERROR>
    requires($error-set-has-v$<ERROR, ERRORS...>)
  constexpr auto holds() const noexcept -> bool;

  template<class ERROR>
    requires($error-set-has-v$<ERROR, ERRORS...>)
  constexpr auto witness() const -> const optional<ERROR>&;

  constexpr auto witness_count() const noexcept -> size_t;

  template<class HANDLER>
  constexpr auto visit(HANDLER&& handler) const -> decltype(auto);

  // @[transpose.errset.ops]{- .sref}@, operations
  friend auto operator==(const error_set_of&, const error_set_of&) -> bool = default;

  constexpr auto combined_with(const error_set_of& other) const -> error_set_of;

private:
  tuple<optional<ERRORS>...> $d-witnesses$; // exposition only
};
```

::: wording

[x]{.pnum} A program that instantiates `error_set_of<ERRORS...>` is ill-formed unless `error_set_is_canonical_v<ERRORS...>` is `true` and `error_set_names_distinct_v<ERRORS...>` is `true`.

:::

:::
