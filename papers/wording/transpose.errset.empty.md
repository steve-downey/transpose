::: add

::: wording

## The empty error set [transpose.errset.empty]{- .sref} {-}

```cpp
error_set_of() = delete;
```

[x]{.pnum} *Remarks*: The empty error set is uninhabited: a computation graded by it raises nothing, so there is no value to construct.

```cpp
template<class ERROR> static constexpr auto contains() noexcept -> bool;
```

[x+1]{.pnum} *Returns*: `false`.

:::

:::
