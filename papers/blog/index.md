<div class="abstract" id="orgf816354">
<p>
One operation &mdash; <code>transpose</code> &mdash; turns out to hide a whole customization mechanism behind it.
The series starts from a loop everyone has written, names the two typeclasses underneath, shows what it costs a type to opt in and an algorithm to consume it, and ends among the prior art that kept reinventing the same idea.
Read it in order, or jump to the part you need.
</p>

</div>


# Contents

1.  [Transposing Structure and Context](transposing-structure-and-context.md) &mdash; one operation hiding inside a loop you have written a dozen times.
2.  [Context is Applicative, Structure is Traversable](how-traverse-and-transpose-work.md) &mdash; the two typeclasses behind the one verb.
3.  [Adapting a Type to a Typeclass](adapting-a-type-to-a-typeclass.md) &mdash; three lines to opt in, and you never reopen anyone's class.
4.  [Writing Algorithms with Typeclass Objects](writing-algorithms-with-typeclass-objects.md) &mdash; one algorithm, every instance, no ADL and no registry.
5.  [Prior Art: How Others Have Brought Typeclasses to C++](prior-art-typeclasses-in-cpp.md) &mdash; FC++, `cat`, libfn, Flux, and the customization point that keeps getting reinvented.


# With running code

A second arc: the same operations demonstrated from compiled examples, with the program output captured in the posts.

6.  [Graded Gadgets](graded-gadgets.md) &mdash; indexing fallible computation by the set of errors it can raise; `error_set`, lazy joins, and `recover`.
7.  [Transpose at Work](transpose-at-work.md) &mdash; `transpose` run over optionals, senders, and hardware SIMD lanes, with receipts.
8.  [Transpose Over Real Senders](transpose-over-real-senders.md) &mdash; the deferred domain over `std::execution`: why the fold could not be spelled, and one allocation for *n* children.
9.  [Traverse Does All the Work](traverse-does-all-the-work.md) &mdash; `transpose` is `traverse` with identity, and an applicative object is the whole contract.
