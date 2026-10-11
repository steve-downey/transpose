::: add

```cpp
template<class T, size_t N>
struct $is-std-array-of-extent$ : false_type {}; // exposition only
```

```cpp
template<class U, size_t N>
struct $is-std-array-of-extent$<array<U, N>, N> : true_type {}; // exposition only
```

```cpp
template<class T, size_t N>
inline constexpr bool $is-std-array-of-extent-v$ =
    $is-std-array-of-extent$<remove_cvref_t<T>, N>::value; // exposition only
```

```cpp
template<class T, size_t N>
struct $array-applicative-impl$ {
  // @[transpose.array.applicative]{- .sref}@, applicative instance for array
  template<class VALUE> auto pure(this auto&&, VALUE&& value);

  template<class FUNCTION, class FIRST, class... REST>
    requires $is-std-array-of-extent-v$<FIRST, N> &&
             ($is-std-array-of-extent-v$<REST, N> && ...)
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
