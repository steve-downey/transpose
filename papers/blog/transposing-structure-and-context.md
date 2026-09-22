<div class="abstract" id="org8594be6">
<p>
You have a <code>vector&lt;optional&lt;T&gt;&gt;</code> and you want an <code>optional&lt;vector&lt;T&gt;&gt;</code>.
You have written the loop. So has everyone.
It is the same loop for futures, for parsed results, for lanewise computation &mdash; and it does not have to be a loop at all.
This is the opening: the problem, and the single verb that solves it. What follows opens the machinery behind that verb, then works the two sides of the mechanism &mdash; a type that wants in, and an algorithm that wants to use it &mdash; before placing the whole approach among its predecessors.
</p>

</div>

**Next:** [Context is Applicative, Structure is Traversable](how-traverse-and-transpose-work.md) &mdash; **Up:** [Contents](index.md)


# A loop you have written before

Here is a `std::vector<std::optional<int>>`. You want a `std::optional<std::vector<int>>`: all of the values if every element is present, and nothing if any element is missing.

The data is trivial. The intent is obvious. There is no standard operation for it, so you write the loop:

```cpp
std::optional<std::vector<int>>
collect(const std::vector<std::optional<int>>& in) {
    std::vector<int> out;
    out.reserve(in.size());
    for (const auto& o : in) {
        if (!o) return std::nullopt;   // early exit
        out.push_back(*o);             // accumulate
    }
    return out;
}
```

It is six lines and four decisions: reserve, the early exit, the dereference, the accumulate. None of them is interesting. All of them are easy to get subtly wrong.

Worse, it does not travel. The day the element stops being `optional` &mdash; it becomes an `expected`, a future, a parser result &mdash; you write the loop again. The *shape* of the problem never changed, but the code did. The loop hard-codes one context into its control flow.


# Name the operation: transpose

Step back and the shape is this:

```text
structure<context<T>>   ->   context<structure<T>>
```

A structure whose elements each sit inside some context, turned into one context wrapping the whole structure. The two axes &mdash; *structure* and *context* &mdash; swap places. Nothing else moves.

That is a **transpose**, in the same sense as transposing a matrix. A `vector` of three `optional` s becomes one `optional` of a three-element `vector`. Order preserved, count preserved, only the nesting inverted.

Once you have the name, you see it everywhere.


# One verb, several contexts

[`beman.transpose`](https://github.com/bemanproject/transpose) provides exactly one verb for this: `transpose`. The fallible case is the loop above, gone:

```cpp
#include <beman/transpose/transpose.hpp>
namespace bt = beman::transpose;

std::vector<std::optional<int>> in{ {1}, {2}, {3} };
auto out = bt::transpose(in);     // out : std::optional<std::vector<int>>
```

Now change the context, not the verb.

A **lanewise** (SIMD-flavored) computation: each element is a `zip_list`, a value indexed by lane. A vector of lanewise values transposes into one lanewise value over the vector, truncated to the common width:

```cpp
std::vector<bt::zip_list<int>> lanes{ {{1, 2}}, {{10, 20}}, {{100, 200}} };
auto out = bt::transpose(lanes);  // out : zip_list<vector<int>> --- 2 lanes of 3
```

Same verb. No per-domain loop, no per-domain overload at the call site. The hand-written version of *that* one is a double loop with a min-width calculation and a transpose-the-ragged-matrix nested index &mdash; the kind of code that is correct exactly once.

And `zip_list` is the portable stand-in. The lanewise applicative that `transpose` actually runs on is `simd_lanes<T, N>` &mdash; an `N`-lane array. Because it is an array it *can* hold a whole `vector`, so `simd_lanes<vector<T>, N>` is a real type and `transpose` flips a `vector<simd_lanes<T, N>>` into one lane-array of vectors, as `zip_list` did above.

And the lanes can be real hardware. Since GCC 16 you can compute each lane with [`std::simd::vec`](https://en.cppreference.com/w/cpp/numeric/simd) &mdash; an actual SIMD register &mdash; and pour the results into a `simd_lanes` that then transposes:

```cpp
#include <simd>
namespace dp = std::simd;

dp::vec<float, 4> xs = ...;      // a hardware register of four lanes
dp::vec<float, 4> ys = 10.0f;    // broadcast to every lane
dp::vec<float, 4> hyp([&](auto lane) { return std::hypot(xs[lane], ys[lane]); });
// hyp's four lanes fill a bt::simd_lanes<float, 4> --- which transposes.
```

There is one honest caveat, and it is the revealing one. `std::simd::vec` holds only scalars, so `vec<vector<T>>` is not a type: a bare register of lanes is not something you can `transpose` directly. That is why `transpose` runs on `simd_lanes` for transposition (an array, which *can* hold a vector), with `std::simd::vec` as the hardware that fills the lanes. The register is not left outside, though: `std::simd::vec` is a registered applicative context in its own right &mdash; `map` and `zip_with` run lane by lane through the same mechanism as `optional` and senders &mdash; it just is not a *structure* that can be walked and rebuilt. Part two makes that distinction precise, and the precise point where the real register stops typechecking is the precise point where the abstraction pays for itself.

The third context is the interesting one for modern C++: a **deferred** computation, a sender. A vector of senders transposes into one sender of a vector &mdash; "run these and gather the results" &mdash; with nothing executed until the composed sender is run.

The type that does it here is a stand-in: `bt::sender<T>` is a `std::function<T()>` that runs when you call `get()`, with no completion signatures, no error or stopped channel, no environment and no scheduler. It shows that the front door preserves laziness, and that is all it shows. Whether the shape survives contact with a real P2300 sender is a different question, and the only way to answer it was to write the thing. So: an applicative object over [`beman.execution`](https://github.com/bemanproject/execution) senders now lives in the repository at `examples/p2300_adapter.hpp`, exercised by `tests/beman/transpose/p2300.test.cpp`, behind an off-by-default build option because it is the one piece here with an external dependency. It supplies `pure` and `invoke` and nothing else &mdash; `pure` is `just`, `invoke` is `when_all` followed by `then` &mdash; so `map`, `zip_with` and `lift` in those tests are this library's own derived operations running over a context nobody designed them against. Values, laziness, the error channel and move-only values all compose.

And then it stops composing, which is the part worth the page.

I had wanted to say that you never need a type-erased `task<>` to name "a sender of a vector": a `std::vector` is *homogeneous*, every element the same concrete sender type, so the result is one concrete type and the erasure everybody reaches for was never needed. Writing the adapter is what showed that to be false. The vector traversal accumulates with an assignment &mdash; `accumulated = applicative.invoke(...)` &mdash; and an assignment requires the context type to be invariant under composition. `optional` is. A real sender is not: `then(when_all(acc, elem), f)` names a new type at every step, so a runtime-sized traversal of real senders cannot be written as that loop at all. It needs a type-erased sender, and `beman.execution` does not ship one.

The stand-in passes precisely because `std::function<T()>` had already erased the type. The convenience that made it a good demonstration is the same property that hid the limit. Compile-time-sized transposition &mdash; the shape `transpose_tuple` already uses &mdash; has no such problem, and that is what the adapter demonstrates.

The honest claim is narrower than the one I wanted and more useful. The abstraction does not make erasure unnecessary. It moves the question to a place where you can see the answer: erasure is needed exactly when the structure's size is a run-time property, and not otherwise. A hand-written gathering loop never tells you that, because it never had to name the type.

One more thing fell out of writing it. `when_all` does not sequence its children; it completes when all of them have. So when `transpose` promises the contexts are composed in order, that is a promise about the order results are assembled into the structure, not the order in which effects run. A synchronous context makes the two coincide. A concurrent one cannot, and the specification should not be read as saying it does.


# Why this is not `transform`, `ranges`, or `zip`

The obvious objection: isn't this just `views::transform` and a fold, or `zip`, by another name?

No, and the reason is the whole point.

The per-element computations here are **independent**. Gathering a vector of senders does not feed one sender's result into the next. The lanewise example composes position by position with no cross-position dependence. Collecting `optional` s reports overall success or failure without one element's value steering another's computation.

That independence is what separates this from `and_then` / `flat_map` style chaining, where each step depends on the value the previous step produced. The standard library already serves *sequential dependence* well: monadic `optional`, sender adaptors, coroutines. What it has no uniform spelling for is *independent* composition over a structure.

The independence isn't a defect. It is the property that lets `transpose` be specified once &mdash; shape preserved, effects combined left to right &mdash; and then work without modification for fallible, deferred, and lanewise contexts. A design built on chaining cannot make that promise, because chaining bakes in an evaluation-order dependence that these contexts neither need nor want.


# The question this leaves open

So one verb collapses three hand-written loops, across three contexts that have nothing to do with each other, with one specification.

Which should make you suspicious. What actually holds those three contexts together? `optional`, a sender, and a lanewise list share no base class, no member named `transpose`, no common header. `std::optional` lives in `std` and cannot be reopened. Yet each one plugs into the same mechanism, statically, with no virtual calls and no central registry.

The thing that makes that work is a small, old idea &mdash; a variable template used as a concept map &mdash; dressed up with C++23. But the operation itself needs an explanation before the mechanism does. Part two, [Context is Applicative, Structure is Traversable](how-traverse-and-transpose-work.md), opens `traverse` and `transpose` from scratch: the structure is *Traversable*, the context is *Applicative*, and that pair is the entire theory. Part three, [Adapting a Type to a Typeclass](adapting-a-type-to-a-typeclass.md), takes the side of a type that wants in: what it costs to make `optional`, or a sender, or your own type answer to the same call (about three lines, and you never reopen a class you don't own). Part four, [Writing Algorithms with Typeclass Objects](writing-algorithms-with-typeclass-objects.md), takes the side of the algorithm author: how one algorithm runs over every one of those types at once, and why this bundled mechanism beats the customization tools C++ already has &mdash; the argument for why it belongs in the standard. Part five, [Prior Art: How Others Have Brought Typeclasses to C++](prior-art-typeclasses-in-cpp.md), steps back to place the approach among the other attempts &mdash; FC++, `cat`, libfn, Flux, and the concept maps the standard itself dropped.
