::: add

::: wording

## traverse [transpose.alg.traverse]{- .sref} {-}

```cpp
template<class F, class T,
         class POLICY =
             remove_cvref_t<decltype(applicative_typeclass<$traverse-context-t$<F, T>>)>>
  requires applicative_object_for<POLICY, $traverse-context-t$<F, T>>
auto traverse(F&& function, T&& value, POLICY policy = POLICY{});
```

[x]{.pnum} *Constraints*: `POLICY` satisfies `applicative_object_for` for the context that applying `function` to an element of `value` yields.

[x+1]{.pnum} *Effects*: Applies `function` to each element of `value` and composes the resulting contextual values with `policy`, preserving the shape of `value`. Elements are visited in the structure's iteration order, and the contextual values are composed in that same order.

[x+2]{.pnum} *Returns*: The shape of `value` held in the single context `policy` composes into.

[x+3]{.pnum} *Complexity*: Exactly one application of `function` per element of `value`.

[x+4]{.pnum} *Remarks*: Let CONTEXT be the type that applying `function` to an element of `value` yields. `POLICY` defaults to the type of the applicative object `applicative_typeclass<CONTEXT>` names, which retains the first failure and discards the rest, and the constraint is `applicative_object_for<POLICY, CONTEXT>`. Passing the object `accumulating_applicative_typeclass<CONTEXT>` names instead composes every element's evidence. The two differ in what the result carries, not in what runs: under either, `function` is applied to every element, as the Complexity clause above says, and it is only the reconstruction of the value in context that stops. No element's context depends on another element's value, so this is independent contextual composition rather than sequential dependence -- which is also why retaining the first failure is as far as the default policy can go. There is nothing for a composition to decline to produce: by the time it sees an element's context, that context has already been computed.

:::

:::
