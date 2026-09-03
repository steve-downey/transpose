::: add

```cpp
template<class Impl>
struct Applicative : protected Impl {
  using Impl::pure;

  // @[transpose.applicative.basis]{- .sref}@, basis operations
  template<class FUNCTION, class FIRST_ARGUMENT, class... REST_ARGUMENTS>
  auto invoke(this auto&& self, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
              REST_ARGUMENTS&&... rest_arguments);

  template<class FUNCTION_IN_CONTEXT, class ARGUMENT_IN_CONTEXT>
  auto ap(this auto&& self, FUNCTION_IN_CONTEXT&& function,
          ARGUMENT_IN_CONTEXT&& argument)
    requires requires(const Impl& impl) {
      impl.ap(forward<FUNCTION_IN_CONTEXT>(function),
              forward<ARGUMENT_IN_CONTEXT>(argument));
    } || requires(const Impl& impl) {
      impl.invoke(detail::applicative_eval, forward<FUNCTION_IN_CONTEXT>(function),
                  forward<ARGUMENT_IN_CONTEXT>(argument));
    };

  // @[transpose.applicative.derived]{- .sref}@, derived operations
  template<class FUNCTION, class ARGUMENT>
  auto map(this auto&& self, FUNCTION&& function, ARGUMENT&& argument);

  template<class VALUE> auto lift(this auto&& self, VALUE&& value);

  template<class FUNCTION, class FIRST_ARGUMENT, class SECOND_ARGUMENT>
  auto zip_with(this auto&& self, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
                SECOND_ARGUMENT&& second_argument);

  template<class FIRST_ARGUMENT, class SECOND_ARGUMENT>
  auto discard_first(this auto&& self, FIRST_ARGUMENT&& first_argument,
                     SECOND_ARGUMENT&& second_argument);

  template<class FIRST_ARGUMENT, class SECOND_ARGUMENT>
  auto discard_second(this auto&& self, FIRST_ARGUMENT&& first_argument,
                      SECOND_ARGUMENT&& second_argument);

  // @[transpose.applicative.grade]{- .sref}@, grade re-indexing
  template<class TARGET_GRADE, class CARRIER>
  constexpr auto subsume(this auto&&, CARRIER&& value)
    requires requires { grade_subsume<TARGET_GRADE>(forward<CARRIER>(value)); };

  // @[transpose.applicative.delegate]{- .sref}@, delegated application
  template<class APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
           class... REST_ARGUMENTS>
  auto invoke_with(this auto&&, const APPLICATIVE_MAP& applicative_map,
                   FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
                   REST_ARGUMENTS&&... rest_arguments);

  template<const auto& APPLICATIVE_MAP, class FUNCTION, class FIRST_ARGUMENT,
           class... REST_ARGUMENTS>
  auto invoke_with(this auto&&, FUNCTION&& function, FIRST_ARGUMENT&& first_argument,
                   REST_ARGUMENTS&&... rest_arguments);
};
```

::: wording

[x]{.pnum} A program that instantiates `Applicative<Impl>` is ill-formed unless `is_same_v<Impl, false_type>` is `false`.

:::

```cpp
template<class VALUE_TYPE>
struct OptionalApplicativeImpl {
  // @[transpose.applicative.optional]{- .sref}@, applicative instance for optional
  template<class VALUE>
  auto pure(this auto&&, VALUE&& value) -> optional<remove_cvref_t<VALUE>>;

  template<class FUNCTION, class FIRST, class... REST>
  auto invoke(this auto&&, FUNCTION&& function, const optional<FIRST>& first,
              const optional<REST>&... rest)
      -> optional<
          remove_cvref_t<invoke_result_t<FUNCTION&, const FIRST&, const REST&...>>>;
};
```

```cpp
template<class VALUE_TYPE>
struct OptionalApplicativeMap : Applicative<OptionalApplicativeImpl<VALUE_TYPE>> {
  using OptionalApplicativeImpl<VALUE_TYPE>::invoke;
  using OptionalApplicativeImpl<VALUE_TYPE>::pure;
};
```

:::
