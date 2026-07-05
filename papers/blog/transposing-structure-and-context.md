<div class="abstract" id="org3db5c65">
<p>
You have a <code>vector&lt;optional&lt;T&gt;&gt;</code> and you want an <code>optional&lt;vector&lt;T&gt;&gt;</code>.
You have written the loop. So has everyone.
It is the same loop for futures, for parsed results, for lanewise computation &#x2014; and it does not have to be a loop at all.
This is part one of four: the problem, and the single front door that solves it. Parts two and three are the mechanism that makes the front door work &#x2014; one from the side of a type that wants in, one from the side of an algorithm that wants to use it &#x2014; and part four places the whole approach among its predecessors.
</p>

</div>


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

It is six lines and four decisions: reserve, the early exit, the dereference, the accumulate. None of them is interesting and all of them are easy to get subtly wrong.

Worse, it does not travel. The day the element stops being `optional` &#x2014; it becomes an `expected`, a future, a parser result &#x2014; you write the loop again. The *shape* of the problem never changed, but the code did, because the loop hard-codes one context into its control flow.


# Name the operation: transpose

Step back and the shape is this:

```text
structure<context<T>>   ->   context<structure<T>>
```

A structure whose elements each sit inside some context, turned into one context wrapping the whole structure. The two axes &#x2014; *structure* and *context* &#x2014; swap places. Nothing else moves.

That is a **transpose**, in the same sense as transposing a matrix. A `vector` of three `optional` s becomes one `optional` of a three-element `vector`. Order preserved, count preserved, only the nesting inverted.

Once you have the name, you start seeing it everywhere.


# One front door, several contexts

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

Same verb. No per-domain loop, no per-domain overload at the call site. The hand-written version of *that* one is a double loop with a min-width calculation and a transpose-the-ragged-matrix nested index &#x2014; the kind of code that is correct exactly once.

And `zip_list` is a stand-in for the real thing. Since GCC 16 you can run the identical lanewise composition on actual hardware SIMD registers, through the same applicative surface:

```cpp
#include <simd>
namespace dp = std::simd;

dp::vec<float> xs = ...;        // a register of lanes
dp::vec<float> ys = 10.0f;      // broadcast to every lane
const auto& lanewise = bt::applicative_typeclass<dp::vec<float>>;
auto hyp = lanewise.zip_with(
    [](float x, float y){ return std::hypot(x, y); }, xs, ys);  // lane by lane
```

There is one honest caveat, and it is the revealing one. `std::simd::vec` holds only scalars, so `vec<vector<T>>` is not a type, and `transpose` of a vector of registers is not even something you can spell. The `transpose` verb stays with `zip_list` (which *can* hold a vector); `std::simd` carries the same *independence* on real lanes through `zip_with` and `map`. The lanewise story is therefore both faithfully modeled and real &#x2014; and the exact point where the real type stops typechecking is the exact point where the abstraction earns its keep.

The third context is the interesting one for modern C++: a **deferred** computation, a `std::execution` sender. A vector of senders transposes into one sender of a vector &#x2014; "run these and gather the results" &#x2014; with nothing executed until the composed sender is run:

```cpp
#include <beman/transpose/exec.hpp>   // sender-aware transpose, on beman.execution
namespace ex = beman::execution;

std::vector<S> senders = ...;                    // S is a real std::execution sender of T
auto out = bt::transpose(senders);               // out : a sender of std::vector<T>
auto [values] = ex::sync_wait(std::move(out)).value();   // now it runs
```

This one is real, on [`beman.execution`](https://github.com/bemanproject/execution) (a P2300 `std::execution` implementation) &#x2014; not a stand-in. And here is the part I like. You might expect to need a type-erased `task<>` to name "a sender of a vector," since composing senders changes their type at every step. You don't. A `std::vector` is *homogeneous*: every element is the same concrete sender type `S`, so the result is a single concrete sender type, and the transpose gathers the children with no `task<>` and no `std::function` anywhere. The type system already had the common type; the abstraction just had to stop reaching for erasure it didn't need.


# Why this is not `transform`, `ranges`, or `zip`

The obvious objection: isn't this just `views::transform` and a fold, or `zip`, by another name?

No, and the reason is the whole point.

The per-element computations here are **independent**. Gathering a vector of senders does not feed one sender's result into the next. The lanewise example composes position by position with no cross-position dependence. Collecting `optional` s reports overall success or failure without one element's value steering another's computation.

That independence is what separates this from `and_then` / `flat_map` style chaining, where each step depends on the value the previous step produced. The standard library already serves *sequential dependence* well: monadic `optional`, sender adaptors, coroutines. What it has no uniform spelling for is *independent* composition over a structure.

The independence is not a limitation to apologize for. It is the property that lets `transpose` be specified once &#x2014; shape preserved, effects combined left to right &#x2014; and then work without modification for fallible, deferred, and lanewise contexts. A design built on chaining cannot make that promise, because chaining bakes in an evaluation-order dependence that these contexts neither need nor want.


# The question this leaves open

So one verb collapses three hand-written loops, across three contexts that have nothing to do with each other, with one specification.

Which should make you suspicious. What actually holds those three contexts together? `optional`, a sender, and a lanewise list share no base class, no member named `transpose`, no common header. `std::optional` lives in `std` and cannot be reopened. Yet each one plugs into the same front door, statically, with no virtual calls and no central registry.

The thing that makes that work is a small, old idea &#x2014; a variable template used as a concept map &#x2014; dressed up with C++23. It has two sides, and each gets a post. Part two, [Adapting a Type to a Typeclass](adapting-a-type-to-a-typeclass.md), takes the side of a type that wants in: what it costs to make `optional`, or a sender, or your own type answer to the same front door (about three lines, and you never reopen a class you don't own). Part three, [Writing Algorithms with Typeclass Objects](writing-algorithms-with-typeclass-objects.md), takes the side of the algorithm author: how one algorithm runs over every one of those types at once, and why this bundled mechanism beats the customization tools C++ already has &#x2014; the argument for why it belongs in the standard. Part four, [Prior Art: How Others Have Brought Typeclasses to C++](prior-art-typeclasses-in-cpp.md), steps back to place the approach among the other attempts &#x2014; FC++, `cat`, libfn, Flux, and the concept maps the standard itself dropped.
