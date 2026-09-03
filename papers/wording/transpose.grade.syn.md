::: add

```cpp
struct unit_grade {};
```

```cpp

```

```cpp

```

```cpp

```

```cpp

```

```cpp
template<class CONTEXT>
struct grade_of {
  using type = unit_grade;
};
```

```cpp

```

```cpp

```

```cpp
template<class GRADE>
struct grade_join_all<GRADE> {
  using type = GRADE;
};
```

```cpp
template<class LEFT, class RIGHT, class... REST>
struct grade_join_all<LEFT, RIGHT, REST...>
    : grade_join_all<grade_join_t<LEFT, RIGHT>, REST...> {};
```

```cpp
template<class MODEL_GRADE, class OPERAND>
struct grade_lifted_into_model {
public:
  using type = conditional_t<is_same_v<raw_grade, unit_grade>,
                                       grade_bottom_t<MODEL_GRADE>, raw_grade>;
};
```

```cpp
template<class MODEL_GRADE, class RAW_GRADE>
struct mixes_with_model_impl
    : bool_constant<is_same_v<grade_model_t<RAW_GRADE>, grade_model_t<MODEL_GRADE>>> {};
```

```cpp
template<class MODEL_GRADE>
struct mixes_with_model_impl<MODEL_GRADE, unit_grade> : true_type {};
```

:::
