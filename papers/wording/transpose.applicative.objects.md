::: add

::: wording

## Operation objects [transpose.applicative.objects]{- .sref} {-}

```cpp
inline constexpr $unspecified$ invoke;
```

[x]{.pnum} *Constraints*: For a call `invoke(function, first_argument, rest_arguments...)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.invoke(function, first_argument, rest_arguments...)` is a valid expression.

[x+1]{.pnum} *Effects*: Equivalent to `applicative<C>.invoke(function, first_argument, rest_arguments...)`, with each argument forwarded in its own value category.

[x+2]{.pnum} *Remarks*: The name `invoke` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The context is deduced from the first in-context argument; every operand shares it.

```cpp
inline constexpr $unspecified$ ap;
```

[x+3]{.pnum} *Constraints*: For a call `ap(function, argument)`, let `C` be `remove_cvref_t<decltype((argument))>`. `applicative<C>.ap(function, argument)` is a valid expression.

[x+4]{.pnum} *Effects*: Equivalent to `applicative<C>.ap(function, argument)`, with each argument forwarded in its own value category.

[x+5]{.pnum} *Remarks*: The name `ap` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it. The context is deduced from the argument in context, not from the callable in context.

```cpp
template<class CONTEXT> inline constexpr $unspecified$ pure;
```

[x+6]{.pnum} *Constraints*: For a call `pure<CONTEXT>(value)`, let `C` be `remove_cvref_t<CONTEXT>`. `applicative<C>.pure(value)` is a valid expression.

[x+7]{.pnum} *Effects*: Equivalent to `applicative<C>.pure(value)`, with `value` forwarded in its own value category.

[x+8]{.pnum} *Remarks*: The name `pure` denotes a family of operation objects, one per context. None is found by argument-dependent lookup, and `CONTEXT` is the only template argument a call gives. The context is named rather than deduced: `pure` builds a context instead of consuming one, so no argument carries it.

```cpp
inline constexpr $unspecified$ zip_with;
```

[x+9]{.pnum} *Constraints*: For a call `zip_with(function, first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.zip_with(function, first_argument, second_argument)` is a valid expression.

[x+10]{.pnum} *Effects*: Equivalent to `applicative<C>.zip_with(function, first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+11]{.pnum} *Remarks*: The name `zip_with` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it.

```cpp
inline constexpr $unspecified$ discard_first;
```

[x+12]{.pnum} *Constraints*: For a call `discard_first(first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.discard_first(first_argument, second_argument)` is a valid expression.

[x+13]{.pnum} *Effects*: Equivalent to `applicative<C>.discard_first(first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+14]{.pnum} *Remarks*: The name `discard_first` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it.

```cpp
inline constexpr $unspecified$ discard_second;
```

[x+15]{.pnum} *Constraints*: For a call `discard_second(first_argument, second_argument)`, let `C` be `remove_cvref_t<decltype((first_argument))>`. `applicative<C>.discard_second(first_argument, second_argument)` is a valid expression.

[x+16]{.pnum} *Effects*: Equivalent to `applicative<C>.discard_second(first_argument, second_argument)`, with each argument forwarded in its own value category.

[x+17]{.pnum} *Remarks*: The name `discard_second` denotes an operation object: it is not found by argument-dependent lookup, and no explicit template argument list may be given in a call to it.

:::

:::
