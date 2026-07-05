<div class="abstract" id="org5c07295">
<p>
Part one ended on a puzzle: <code>optional</code>, a <code>std::execution</code> sender, and a lanewise SIMD value share no base class, no common header, no member named <code>transpose</code> &#x2014; yet each plugs into the same front door.
This is how, from the side that matters most to you if you own a type: what does it cost to make <i>your</i> type join in?
The answer is about three lines, and you never reopen a class you don't own.
This is part three of five.
</p>

</div>

*This is part three of a short series.* *Part one,* [Transposing Structure and Context](transposing-structure-and-context.md), *named the problem; part two,* [Context is Applicative, Structure is Traversable](how-traverse-and-transpose-work.md), *opened the machinery &#x2014; `traverse` and `transpose` built from a Traversable structure and an Applicative context.* *This post takes the* type adapter's *chair: what it takes to make `optional`, a sender, or your own type into one of those instances. Part four,* [Writing Algorithms with Typeclass Objects](writing-algorithms-with-typeclass-objects.md), *takes the* algorithm author's.


# You already know this pattern

You use `std::numeric_limits<T>` all the time.

```cpp
auto max_int = std::numeric_limits<int>::max();       // 2147483647
auto is_signed = std::numeric_limits<int>::is_signed; // true
```

It is a struct, specialized per type, providing named operations and values. If you wanted to teach it about a new numeric type, you would specialize the template. That is the whole idea.

The version this series is built on generalizes `numeric_limits` in three small ways: the operations are functions rather than only constants, a batch of derived operations comes for free via CRTP, and lookup goes through a *variable template* rather than a class template. That last change is what lets `optional`, a sender, and a lanewise value &#x2014; three types with nothing in common &#x2014; all answer to the same name.

Here is that name for Foldable.

From `include/beman/transpose/fold.hpp`:

```cpp
template <class T>
inline constexpr auto foldable_typeclass = std::false_type{};
```

The default is `std::false_type`: no instance. A type is Foldable exactly when someone has specialized this variable template for it. Nothing is Foldable until it opts in, and opting in is a specialization.


# The opt-in is three lines

Here is a type opting in.

From `examples/binary_tree.hpp`:

```cpp
template <class T>
inline constexpr auto foldable_typeclass<BinaryTree<T>> =
    BinaryTreeFoldableMap<T>{};
```

Three lines, and look at what is *not* here.

No base class was added to `BinaryTree<T>`. No macro was expanded inside it. No `friend` declaration, no member function, no member typedef. The data type was not touched at all. The specialization lives in its own header, next to the type it adapts &#x2014; or next to the algorithm that needs it, or in a third file you own &#x2014; and it names the type from the outside.

That "from the outside" is the part that matters when the type is not yours. `std::optional` lives in `std`, and you may not reopen `std`. You do not need to. The variable template lives in your namespace; the type it adapts lives in `std`; the specialization reaches across the boundary and says "this type is Foldable, and here is how."

```cpp
template <class VALUE_TYPE>
inline constexpr auto functor_typeclass<std::optional<VALUE_TYPE>> =
    OptionalFunctorMap<VALUE_TYPE>{};
```

There is no monkey-patching here and no central registry that every adapter must edit. Two types adapted by two different people in two different headers never collide, because a variable-template specialization is just a definition keyed on a type.


# An instance, not the typeclass &#x2014; and not necessarily for everyone

Notice what you did *not* do. You did not define Foldable. Foldable &#x2014; the concept, the `fold_map` primitive, the ten derived operations, the base that produces them &#x2014; is part of the proposal, written once, for everyone. What you wrote is an *instance*: the single fact that *your* type is Foldable, and how. That division of labour is the whole reason three lines is enough &#x2014; the typeclass and every algorithm written against it already exist, waiting only for an instance to connect your type to them.

And the instance is a *value*, which makes registering it globally a choice rather than an obligation. The specialization above is a *global* declaration: for the whole program, `BinaryTree<T>` is Foldable *this* way. For a `BinaryTree` that is exactly right &#x2014; there is one obvious way it folds. But when the type is a third party's, or a `std` vocabulary type, a global instance quietly commits *everyone* to your choice of the canonical mapping, and that may not be yours to make. You are not forced into it. Because the instance is an ordinary object, you can leave it unregistered and hand it to an algorithm directly &#x2014; the generic algorithms accept an explicit instance, or take it as a pinned template parameter, precisely so an adaptation can be local to the code that needs it instead of a fact imposed on the whole program. Part four walks through those lookup modes; the point here is only that *writing an instance* and *declaring it for everyone* are separate steps, and you may stop after the first.


# You write a little; you get a lot

The opt-in points the variable template at a *Map* object, and the Map is where the small amount of real work goes. The trick is that "the real work" is genuinely small: you write the one or two operations that are irreducible for your type, and a CRTP base derives the rest.

Take the smallest typeclass, Functor. Its primitive is `fmap`; its derived operation is `replace`.

****Layer 1 &#x2014; the primitive.**** You write `fmap` for your type. Here it is for `std::optional`:

From `include/beman/transpose/functor.hpp`:

```cpp
template <class VALUE_TYPE>
struct OptionalFunctorImpl {
    template <class F>
    auto fmap(this auto&&, F&& function,
              const std::optional<VALUE_TYPE>& value) {
        using Result = std::invoke_result_t<F, const VALUE_TYPE&>;
        if (!value) {
            return std::optional<remove_cvref_t<Result>>{};
        }
        return std::optional<remove_cvref_t<Result>>{
            std::invoke(std::forward<F>(function), *value)};
    }
};
```

(The `this auto&&` is C++23 deducing `this`; it carries value category and constness through without a hand-written overload set.)

****Layer 2 &#x2014; the derived surface, for free.**** The CRTP base `Functor<Impl>` inherits your `Impl` and adds `replace`, defined in terms of your `fmap`:

```cpp
template <class Impl>
struct Functor : protected Impl {
    using Impl::fmap;

    template <class T, class U>
    auto replace(this auto&& self, T&& value, U&& replacement) {
        return self.fmap([replacement = std::forward<U>(replacement)](
                             const auto&) { return replacement; },
                         std::forward<T>(value));
    }
};
```

You wrote one function; the user got two.

****Layer 3 &#x2014; the Map, which wires it together and names the primitive:****

```cpp
template <class VALUE_TYPE>
struct OptionalFunctorMap : Functor<OptionalFunctorImpl<VALUE_TYPE>> {
    using OptionalFunctorImpl<VALUE_TYPE>::fmap;
};
```

For Functor the one-to-two ratio is modest. It gets dramatic quickly. For Foldable, a single `fold_map` hands you `length`, `to_vector`, `fold_left`, `fold_right`, `any_of`, `all_of`, `empty`, `find_first`, `combine_all`, and `fold` &#x2014; ten operations from one.

Here is the whole implementer surface, across the four typeclasses this design uses:

| Typeclass   | Primitive(s)                 | Derived operations                                                                                                 |
|----------- |---------------------------- |------------------------------------------------------------------------------------------------------------------ |
| Functor     | `fmap`                       | `replace`                                                                                                          |
| Foldable    | `fold_map` (or `fold_right`) | `length`, `to_vector`, `fold_left`, `fold_right`, `any_of`, `all_of`, `empty`, `find_first`, `combine_all`, `fold` |
| Applicative | `pure` + `apply`             | `invoke`, `map`, `lift`, `ap`, `zip_with`, `discard_first`, `discard_second`                                       |
| Traversable | `traverse`                   | `transpose`, `for_each`, `traverse_with`, `transpose_with`                                                         |

Write one or two functions. Get a full API. That is the trade, and it is the whole reason adapting a type is cheap enough to actually do.


# The technique is optional; the contract is not

The three layers &#x2014; Impl, base, Map &#x2014; are a *convenience*, not a requirement. They exist so the common case costs one or two functions instead of a dozen. But a typeclass instance is, in the end, just the looked-up *dictionary*: a value that carries the concept's operations under their agreed names. The only contract it must honour is that the names are all *there* and that they *behave lawfully* &#x2014; that `fold_map` really folds, that `pure` and `apply` satisfy the applicative laws, that `traverse` preserves shape.

*How* you produce a dictionary that meets that contract is entirely your business. Derive it from a small core with the CRTP base, as here; or hand-write every operation; or generate it; or wrap an existing library. Two instances of the same typeclass can be built by completely different means and remain interchangeable to every algorithm, because the algorithm sees only the dictionary and trusts only its laws &#x2014; never how the dictionary was made. That is the freedom `std::numeric_limits` has always had: nothing dictates *how* a specialization computes `max()`, only that it is present and correct.


# You write the primitive you *can* write

Read the Foldable row again: `fold_map` **or** `fold_right`. That parenthetical is not a footnote. It is the property that decides whether some types can join at all.

Haskell has had this for decades &#x2014; a class can declare more than one valid minimal core:

```haskell
class Foldable t where
  foldMap :: Monoid m => (a -> m) -> t a -> m
  foldr :: (a -> b -> b) -> b -> t a -> b
  {-# MINIMAL foldMap | foldr #-}
```

Either primitive suffices; the base derives the other. A type contributes whichever one is natural for it. This design does the same, and the Map's `using` declaration is the switch: it names which operation you are supplying as primitive, and the CRTP base fills in the rest &#x2014; including deriving your missing "primitive" from the one you did provide.

For Foldable that is a nicety &#x2014; `fold_map` and `fold_right` each recover the other, and a type supplies whichever is natural. Applicative is where the choice of primitive stops being cosmetic, and the revealing case is `std::simd`.

An applicative's textbook primitive is `apply`: a function *already inside the context* applied to an argument *inside the context*.

```cpp
// apply : C<(T -> U)>, C<T> -> C<U>
```

That works for `optional`, for senders, for a zip-style list, for any *array* of lanes &#x2014; anything that can hold a callable. Now try to spell `apply` for [`std::simd::vec`](https://en.cppreference.com/w/cpp/numeric/simd), real in GCC 16. You cannot. A `std::simd::vec<T>` holds only vectorizable scalars, so `vec<callable>` is not even a type &#x2014; there is no function-in-a-register, and `apply` *cannot be spelled at all*.

But the operation you actually want &#x2014; run a scalar function across every lane &#x2014; is perfectly well defined:

```cpp
// invoke : (T... -> U), C<T>... -> C<U>     (no C<callable> ever formed)
std::simd::vec<float> a = ..., b = ...;
auto c = std::simd::vec<float>(
    [&](auto lane) { return std::hypot(a[lane], b[lane]); });  // lane by lane
```

That operation is `invoke`: a *plain* function applied to in-context arguments, forming no context-of-functions. It is not sugar for `apply` &#x2014; it is the more general shape. Where `apply` needs the context to hold a callable, `invoke` never does, and `map` and `zip_with` are just `invoke` with one or two arguments. `std::simd::vec` is the proof: a context that expresses `invoke` but *physically cannot* express `apply`.

So which primitive does the *library* actually build on? `apply`. The Applicative base takes `pure` + `apply` as its core and derives `invoke`, `map`, and `zip_with` from it, and every *registered* context provides `apply` &#x2014; because the lanewise applicative that goes through the front door is `simd_lanes`, an array of lanes. Being an array, `simd_lanes` can hold a callable, so it has a perfectly good `apply` (and `invoke`), and it transposes; `std::simd::vec` is the hardware that computes the scalars those lanes hold. `std::simd::vec` itself stays a *partial* applicative &#x2014; `invoke` yes, `apply` no &#x2014; a sharp illustration rather than a registered instance.

There is a real question lurking, worth naming and leaving open: could a context be an applicative on `invoke` *alone*, with no `apply` underneath? You would give up partial application &#x2014; there is no in-context function to hand arguments to one at a time. But every applicative chain can be refactored into a single `invoke` that starts from `pure(f)` and takes all its arguments at once &#x2014; which is exactly what `invoke` does &#x2014; so perhaps not much is lost. That is a thread for another day; `std::simd` is the reason it is worth pulling.

The adapter's takeaway is smaller and concrete. Because the operations are *bundled* and the base derives the many from the few, you write one or two primitives and inherit a full surface &#x2014; and, as Foldable shows, the bundle can even accept *either* of two cores and fill in the other. A pile of independent one-CPO-per-operation hooks cannot make that trade: each operation stands alone, with nowhere to say "derive this one from that one," or "either of these will do." That coherence &#x2014; primitive and derived kept together &#x2014; is what a standard vocabulary for this should provide, and it is the through-line of part four.


# What it cost you, and what you got

Tally the adapter's bill. You wrote one primitive operation &#x2014; or two, for Applicative &#x2014; in a small `Impl` struct. You wrote a Map that names which primitive that was. You wrote a three-line variable-template specialization to register it. You did not derive from anything, reopen anything, edit a registry, or coordinate with any other adapter. You did not touch the data type, which may not even be yours to touch.

In return your type gained the full derived surface in the table above, and &#x2014; this is the part that pays off in part four &#x2014; it now flows through every generic algorithm written against these typeclasses, unchanged, with static dispatch and no virtual calls. The optional you just adapted, the tree someone else adapted, the `simd_lanes` carrying a hardware SIMD result: the same algorithm runs over all of them.

That is the other half of the story, and the reason the cheap opt-in is worth anything at all. Part four, [Writing Algorithms with Typeclass Objects](writing-algorithms-with-typeclass-objects.md), takes the algorithm author's chair: how you *consume* these machines, why the names compose, and why traits, CPOs, and concepts-alone cannot hold an applicative family together the way a bundled typeclass object can.
