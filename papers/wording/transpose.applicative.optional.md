::: add

::: wording

## Applicative instance for optional [transpose.applicative.optional]{- .sref} {-}

```cpp
template<class VALUE>
auto pure(this auto&&, VALUE&& value) -> optional<remove_cvref_t<VALUE>>;
```

[x]{.pnum} *Returns*: An engaged `optional` holding `value`.

```cpp
template<class FUNCTION, class FIRST, class... REST>
  requires $is-optional-v$<FIRST> && ($is-optional-v$<REST> && ...)
auto invoke(this auto&&, FUNCTION&& function, FIRST&& first, REST&&... rest)
    -> optional<remove_cvref_t<
        invoke_result_t<FUNCTION&, $contained-ref-t$<FIRST>, $contained-ref-t$<REST>...>>>;
```

[x+1]{.pnum} *Returns*: If every operand is engaged, an engaged `optional` holding the result of invoking `function` with the contained values, in the order written; otherwise a disengaged `optional`.

[x+2]{.pnum} *Remarks*: `function` is invoked at most once. Each operand's held value is passed on with that operand's own value category, so a caller that hands over an rvalue operand has its value moved from rather than copied. This is what lets a traversal carry its accumulated result forward without duplicating it at every step.

:::

:::
