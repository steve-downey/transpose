::: add

::: wording

## transpose [transpose.alg.transpose]{- .sref} {-}

```cpp
template<class T>
  requires $transposable-structure$<T>
auto transpose(T&& value);
```

[x]{.pnum} *Constraints*: `traversable_typeclass` names a traversable object for `value`'s type, and `applicative_typeclass` names an applicative object for that structure's element type.

[x+1]{.pnum} *Effects*: Equivalent to traversing `value` with the identity function: a structure of contextual values, `structure<context<T>>`, becomes a single contextual value of the structure, `context<structure<T>>`, preserving shape. The applicative object is inferred from the structure's element type.

[x+2]{.pnum} *Returns*: That single contextual value.

[x+3]{.pnum} *Complexity*: Exactly one composition operation on the inferred applicative object per element of `value`.

[x+4]{.pnum} *Remarks*: Elements are visited in the structure's iteration order, and their contexts are composed in that same order. The complexity is stated in composition operations rather than in element operations because the latter is not the algorithm's to promise: an applicative object that composes its operands only as `const` lvalues must copy the accumulated result at every step, which is quadratic in the elements collected however the traversal is written. Every applicative object this library registers composes an rvalue operand without duplicating the value it holds, so for those the total number of element operations is linear in the number of elements.

:::

:::
