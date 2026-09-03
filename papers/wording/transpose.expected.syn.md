::: add

```cpp
template<class CARRIER>
struct carrier_value {
  using type = CARRIER;
};
```

```cpp
template<class VALUE, class ERROR>
struct carrier_value<expected<VALUE, ERROR>> {
  using type = VALUE;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedApplicativeImpl {
  // @[transpose.expected.applicative]{- .sref}@, applicative instance for expected
  template<class VALUE>
  auto pure(this auto&&, VALUE&& value) -> expected<remove_cvref_t<VALUE>, ERROR_TYPE>;

  template<class FUNCTION, class FIRST, class... REST>
  auto invoke(this auto&&, FUNCTION&& function,
              const expected<FIRST, ERROR_TYPE>& first,
              const expected<REST, ERROR_TYPE>&... rest)
      -> expected<
          remove_cvref_t<invoke_result_t<FUNCTION&, const FIRST&, const REST&...>>,
          ERROR_TYPE>;

  template<class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
            (!all_declare_v<ERROR_TYPE, CARRIERS...>) &&
            all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
  auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
      -> expected<remove_cvref_t<
                      invoke_result_t<FUNCTION&, const carrier_value_t<CARRIERS>&...>>,
                  ERROR_TYPE>;

  template<class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
            (!all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
            (mixes_with_model<grade_of_t<expected<VALUE_TYPE, ERROR_TYPE>>, CARRIERS> &&
             ...)
  auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
      -> mixed_result_t<grade_of_t<expected<VALUE_TYPE, ERROR_TYPE>>,
                        expected<remove_cvref_t<invoke_result_t<
                                     FUNCTION&, const carrier_value_t<CARRIERS>&...>>,
                                 ERROR_TYPE>,
                        CARRIERS...>;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedApplicativeMap : Applicative<ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>> {
  using ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::invoke;
  using ExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct AccumulatingExpectedApplicativeImpl {
  // @[transpose.expected.accumulating]{- .sref}@, accumulating instance
  template<class VALUE>
  auto pure(this auto&&, VALUE&& value) -> expected<remove_cvref_t<VALUE>, ERROR_TYPE>;

  template<class FUNCTION, class FIRST, class... REST>
  auto invoke(this auto&&, FUNCTION&& function,
              const expected<FIRST, ERROR_TYPE>& first,
              const expected<REST, ERROR_TYPE>&... rest)
      -> expected<
          remove_cvref_t<invoke_result_t<FUNCTION&, const FIRST&, const REST&...>>,
          ERROR_TYPE>;

  template<class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
            (!all_declare_v<ERROR_TYPE, CARRIERS...>) &&
            all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>
  auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
      -> expected<remove_cvref_t<
                      invoke_result_t<FUNCTION&, const carrier_value_t<CARRIERS>&...>>,
                  ERROR_TYPE>;

  template<class FUNCTION, class... CARRIERS>
    requires(sizeof...(CARRIERS) > 0) &&
            (is_expected_v<remove_cvref_t<CARRIERS>> || ...) &&
            (!all_declare_or_bare_v<ERROR_TYPE, CARRIERS...>) &&
            (mixes_with_model<grade_of_t<expected<VALUE_TYPE, ERROR_TYPE>>, CARRIERS> &&
             ...)
  auto invoke(this auto&&, FUNCTION&& function, const CARRIERS&... operands)
      -> mixed_result_t<grade_of_t<expected<VALUE_TYPE, ERROR_TYPE>>,
                        expected<remove_cvref_t<invoke_result_t<
                                     FUNCTION&, const carrier_value_t<CARRIERS>&...>>,
                                 ERROR_TYPE>,
                        CARRIERS...>;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct AccumulatingExpectedApplicativeMap
    : Applicative<AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>> {
  using AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::invoke;
  using AccumulatingExpectedApplicativeImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadImpl {
  using element_type = VALUE_TYPE;
};
```

```cpp
template<class VALUE_TYPE, class ERROR_TYPE>
struct ExpectedMonadMap : Monad<ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>> {
  using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::bind;
  using ExpectedMonadImpl<VALUE_TYPE, ERROR_TYPE>::pure;
};
```

:::
