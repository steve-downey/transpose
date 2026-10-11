::: add

```cpp
template<class T>
struct $is-std-array$ : false_type {}; // exposition only
```

```cpp
template<class U, size_t M>
struct $is-std-array$<array<U, M>> : true_type {}; // exposition only
```

```cpp
template<class T>
inline constexpr bool $is-std-array-v$ =
    $is-std-array$<remove_cvref_t<T>>::value; // exposition only
```

```cpp
template<class T, size_t N>
struct $array-applicative-impl$ {
  // @[transpose.array.applicative]{- .sref}@, applicative instance for array
  template<class VALUE> auto pure(this auto&&, VALUE&& value);

  template<class FUNCTION, class FIRST, class... REST>
    requires $is-std-array-v$<FIRST> && ($is-std-array-v$<REST> && ...)
  auto invoke(this auto&&, FUNCTION&& function, FIRST&& first, REST&&... rest);
}; // exposition only
```

```cpp
template<class T, size_t N>
struct $array-applicative-map$ : derive_applicative<$array-applicative-impl$<T, N>> {
  using $array-applicative-impl$<T, N>::invoke;
  using $array-applicative-impl$<T, N>::pure;
}; // exposition only
```

:::
