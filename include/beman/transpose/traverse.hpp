// include/beman/transpose/traverse.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TRAVERSE_HPP
#define BEMAN_TRANSPOSE_TRAVERSE_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::transpose {

//! \remarks This concept is satisfied when `applicative_typeclass` names a
//! conforming applicative object for `CONTEXT`. It is the requirement
//! `transpose` and `transpose_with` state, spelled so that it can be an
//! associated constraint: both operations look their applicative object up
//! rather than taking it, so there is no parameter whose type could carry the
//! requirement, and without it the only way to discover that a type has no
//! applicative object is to instantiate the operation's body -- which, for a
//! deduced return type, means a diagnostic rather than a constraint that
//! simply does not match.
//! \expos
template <class CONTEXT>
concept applicative_context = applicative_object<
    std::remove_cvref_t<decltype(applicative_typeclass<CONTEXT>)>, CONTEXT>;

//! \remarks This concept is satisfied when `OBJ` declares
//! `consumes_rvalue_structure` to be `true`: a traversable object that, given
//! a structure as an rvalue, presents each element to `function` as an
//! rvalue. An object that does not declare it presents `const` lvalues
//! whatever category it was handed, which is what an object written without
//! the question in mind does.
//!
//! Consumption is declared rather than assumed because it is not a property
//! every structure can have. An owning structure -- `std::vector` -- really
//! does hand its elements over when it is handed over, and saves a copy per
//! element by doing so. A structure that shares its parts does not: a
//! persistent sequence or tree shares its interior with every earlier
//! version of itself, so an rvalue conveys the handle and not exclusive
//! ownership of the values inside, and the only way such a structure could
//! present an rvalue element is by copying it into a temporary first -- a
//! copy it then pays a move on top of, where presenting a `const` lvalue
//! would have copied straight into the result. Requiring consumption of
//! every traversable object would charge the structures that cannot benefit
//! for the benefit the ones that can receive.
//!
//! The declaration is not merely advisory: `traversable_impl` and
//! `traversable_object` probe an object that declares it with a witness
//! accepting an element only as an rvalue, so an object that claims to
//! consume and does not is reported.
//! \expos
template <class OBJ>
concept consuming_traversable_object =
    requires { requires bool(OBJ::consumes_rvalue_structure); };

/// Traversable pattern invariants:
/// - Instances are single lookup objects that provide traverse(F, T).
/// - transpose is a derived object operation implemented from
///   traverse(identity).
/// - Dispatch happens through a provided object or
///   traversable_typeclass<Concrete>.
/// - Traversal must preserve container shape while transposing structure and
///   context.

/// CRTP base for Traversable instances.
/// `Impl` must provide `traverse(applicative, f, container)` and declare
/// `element_type`. All other operations (`transpose`, `for_each`,
/// `traverse_with`, `transpose_with`) are derived.
///
/// DELIBERATE CONSTRAINT: Traversable does not and must not require a
/// Foldable instance (no Haskell-style superclass). `traverse` needs only
/// an Applicative and the walk; the fold family is proposed by the
/// companion recursive-tree-algorithms paper, and requiring it here would
/// put that paper's concept beneath this one after review. foldMapDefault-
/// style derivations (folding via traverse) remain possible as evidence,
/// never a requirement.
template <class Impl>
struct Traversable : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No traversable_typeclass<T> specialization found. "
                  "Specialize beman::transpose::traversable_typeclass<T> for "
                  "your type T, "
                  "provide traverse(applicative, F, T), and declare 'using "
                  "element_type = T;'.");
    static_assert(
        requires { typename Impl::element_type; },
        "Traversable Impl must declare 'using element_type = T;' "
        "so that transpose() and traverse_with() can deduce the element type.");
    // Alternate-core: Impl::traverse is the primitive; transpose is derived
    // from it. A transpose-primitive Impl would shadow transpose instead.
    using Impl::traverse;
    using element_type = typename Impl::element_type;

    // Re-exported the way element_type is, and for the same reason: Impl is
    // a protected base, so a declaration made on Impl is not visible on the
    // object through it. An Impl that says nothing does not consume, which
    // is what an Impl written without the question in mind does.
    static constexpr bool consumes_rvalue_structure =
        consuming_traversable_object<Impl>;

    // \ref{transpose.traversable.ops}, traversal operations
    template <class T, class F>
    auto for_each(this auto &&self, T &&value, F &&function);

    template <class T>
        requires applicative_context<typename Impl::element_type>
    auto transpose(this auto &&self, T &&value);

    // \ref{transpose.traversable.delegate}, delegated traversal
    template <class TRAVERSABLE_MAP, class T, class F>
    auto traverse_with(this auto &&, const TRAVERSABLE_MAP &traversable_map,
                       F &&function, T &&value);

    template <class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
    auto traverse_with(this auto &&, const TRAVERSABLE_MAP &traversable_map,
                       const APPLICATIVE_MAP &applicative_map, F &&function,
                       T &&value);

    template <class TRAVERSABLE_MAP, class T>
        requires applicative_context<
            typename std::remove_cvref_t<TRAVERSABLE_MAP>::element_type>
    auto transpose_with(this auto &&self,
                        const TRAVERSABLE_MAP &traversable_map, T &&value);

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }
};

//! \remarks This variable template is the lookup point for the Traversable
//! object of a structure type. A program may specialize it for a
//! program-defined structure. The primary template names no traversable
//! object.
template <class T>
inline constexpr auto traversable_typeclass = std::false_type{};

/// The Traversable object `traversable_typeclass` names for a structure
/// type, and that object's element type. Exposition-only spellings, so that
/// `transpose`'s constraint and `traverse`'s inferred context can be written
/// once rather than nested three deep. `structure_element_t` is ill-formed
/// for a type naming no traversable object, which is what makes the
/// constraints below evaluate false for such a type instead of diagnosing.
//! \expos
template <class T>
using traversable_object_t = std::remove_cvref_t<
    decltype(traversable_typeclass<std::remove_cvref_t<T>>)>;

//! \expos
template <class T>
using structure_element_t = typename traversable_object_t<T>::element_type;

//! \remarks This alias names the category in which a traversal of a
//! structure of type `T` presents an element to `function`: as an rvalue
//! when `T` is an rvalue *and* its traversable object declares
//! `consuming_traversable_object`, and as a `const` lvalue otherwise. It is
//! what lets the context be inferred before a traversable object has been
//! selected, and the declaration is what lets it be inferred correctly for
//! an object that does not consume.
//!
//! Deducing the category from `T` alone would be inferring a context from a
//! call the object will not make. The consequence is not a slower traversal
//! but an unsatisfiable one: a callable accepting only an rvalue element
//! would satisfy `traverse`'s constraint and then fail inside a body whose
//! return type is deduced, which is a diagnostic rather than a constraint
//! that does not match.
//! \expos
template <class T>
using traverse_element_t = std::conditional_t<
    !std::is_lvalue_reference_v<T> &&
        consuming_traversable_object<traversable_object_t<T>>,
    structure_element_t<T> &&, const structure_element_t<T> &>;

/// The applicative context `traverse(function, value)` would infer: the
/// return type of `function` applied to one element of `value`'s traversable
/// structure. Factored out so both the POLICY default and its constraint can
/// name it without repeating the computation. Exposition-only: it is named by
/// `traverse`'s declaration and by nothing else.
///
/// The callable is spelled `F &`, not `F`. A traversal invokes a named
/// `function` variable once per element, which is an lvalue every time; for a
/// callable passed as a temporary, `F` deduces to a non-reference type and
/// `invoke_result_t<F, ...>` would test rvalue invocation that never happens.
/// Detecting in a category the implementation does not use rejects an
/// `&`-qualified callable that would have worked, and admits an `&&`-only
/// callable that then fails inside the loop.
//! \expos
template <class F, class T>
using traverse_context_t =
    std::remove_cvref_t<std::invoke_result_t<F &, traverse_element_t<T>>>;

//! \remarks This concept is satisfied when `traversable_typeclass` names a
//! traversable object for `T` and `applicative_typeclass` names an
//! applicative object for that object's element type -- the requirement
//! `transpose`'s Constraints states.
//!
//! It is deliberately not written in terms of `traversable_object`. That
//! concept requires `transpose` where the element type is itself a context,
//! and `transpose` is constrained on this one; naming it here would make the
//! two concepts mutually recursive.
//! \expos
template <class T>
concept transposable_structure = requires {
    typename structure_element_t<T>;
} && applicative_context<structure_element_t<T>>;

namespace detail {

/// Whether `APPLICATIVE` offers a native composition for a runtime-sized
/// collection of `CONTEXT` operands.
///
/// Defined here rather than beside its first caller so that the policy
/// concept below and the traversals that take the preferring branch ask the
/// one question in the one spelling. Two spellings of the same probe are two
/// things that can disagree, and the disagreement would show up as an
/// operation that is constrained in and then does not compile.
///
/// DELIBERATE CONSTRAINT: this names no context, no carrier and no concept
/// beyond the member itself. Whether anything answers it is the applicative
/// object's business. See `docs/decisions.md#runtime-arity-composition`,
/// whose Sentinel this spelling exists to keep.
//! \expos
template <class APPLICATIVE, class CONTEXT>
concept collecting_applicative = requires(const APPLICATIVE &applicative) {
    applicative.collect(std::declval<std::vector<CONTEXT>>());
};

} // namespace detail

//! \remarks This concept is satisfied when `POLICY` is an `applicative_object`
//! over `CONTEXT` that can compose a structure of `CONTEXT` into one
//! `CONTEXT` of the structure -- by either of the two routes a traversal has.
//! The first is that `pure` returns exactly `CONTEXT` from `CONTEXT`'s element
//! type, which is what the pairwise composition needs: it assigns each partial
//! result back into a variable of one type, starting from `pure`, so composing
//! element results into anything other than `CONTEXT` itself would not
//! preserve `traverse`'s "shape of `value` held in context" contract. The
//! second is that `POLICY` offers `collect`, which composes the whole
//! structure in one operation and so is not bound by what `pure` returns. A
//! traversal takes whichever route `POLICY` offers, preferring `collect`, so
//! this concept asks for whichever route it will take rather than for the one
//! it would take by default: an object that supplies `collect` is not
//! required to also satisfy a requirement of the composition it will not
//! perform. Constraining the policy parameter on the full deep concept, not
//! merely on `pure`, is what makes an argument that is not a real applicative
//! object -- a further container, say -- ill-formed rather than silently
//! accepted.
template <class POLICY, class CONTEXT>
concept applicative_object_for =
    applicative_object<POLICY, CONTEXT> && (requires(const POLICY &policy) {
        {
            policy.pure(std::declval<applicative_value_t<CONTEXT>>())
        } -> std::same_as<CONTEXT>;
    } || detail::collecting_applicative<POLICY, CONTEXT>);

//! \remarks This concept is satisfied when `OBJ` declares an `element_type`
//! that is itself a context with a conforming applicative object -- the
//! condition `Traversable::transpose` and `Traversable::transpose_with`
//! carry on their own declarations. It exists so that `traversable_object`
//! can make those two operations conditional on that condition rather than
//! on a second, weaker spelling of it. The conjunction is what lets the
//! `element_type` half be asked first: an `OBJ` that declares none is not a
//! transposing object, and naming `OBJ::element_type` in an atomic
//! constraint on its own would be a substitution failure rather than an
//! answer.
//! \expos
template <class OBJ>
concept transposing_object = requires { typename OBJ::element_type; } &&
                             applicative_context<typename OBJ::element_type>;

//! \remarks This concept is satisfied when `IMPL` supplies the minimal
//! complete basis the `Traversable` CRTP base needs: a declared
//! `element_type` and `traverse`, probed with a representative witness
//! callable that lifts an element into `std::optional` (always a
//! registered applicative context, for any element type). This is the
//! `MINIMAL` pragma to `traversable_object`'s class declaration --
//! `for_each`, `transpose`, `traverse_with` and `transpose_with` are all
//! derived and belong to `traversable_object` alone. Traversable admits
//! exactly one basis, so there is no disjunction here the way there is for
//! `applicative_impl` and `foldable_impl`.
//!
//! An `IMPL` that declares `consuming_traversable_object` is probed a second
//! time, on an rvalue structure and with a witness that accepts an element
//! only as an rvalue. That is what makes the declaration a statement rather
//! than a claim: `traverse_element_t` infers the applicative context from
//! it, so an `IMPL` that says it consumes and then hands `const` lvalues on
//! would have a context inferred for a call it never makes. An `IMPL` that
//! does not declare it is not asked.
template <class IMPL, class STRUCTURE>
concept traversable_impl =
    requires(const IMPL &impl, const STRUCTURE &structure) {
        typename IMPL::element_type;
        impl.traverse(applicative_typeclass<
                          std::optional<applicative_value_t<STRUCTURE>>>,
                      detail::probe_witness<
                          std::optional<applicative_value_t<STRUCTURE>>>{},
                      structure);
    } && (!consuming_traversable_object<IMPL> || requires(const IMPL &impl) {
        impl.traverse(applicative_typeclass<
                          std::optional<applicative_value_t<STRUCTURE>>>,
                      detail::consuming_probe_witness<
                          applicative_value_t<STRUCTURE>,
                          std::optional<applicative_value_t<STRUCTURE>>>{},
                      std::declval<STRUCTURE>());
    });

//! \remarks This concept is satisfied when `OBJ` provides the full
//! Traversable object surface over `STRUCTURE`: `traverse`, `for_each` and
//! `traverse_with`, each probed with a representative witness callable that
//! lifts an element into `std::optional` (always a registered applicative
//! context, for any element type). `transpose` and `transpose_with` are
//! required only where `OBJ::element_type` itself names a registered
//! applicative context: both are hard-wired to
//! `applicative_typeclass<element_type>`, which names no applicative object
//! for a structure like `std::vector<int>` whose elements are not
//! themselves an applicative context -- transposing such a structure is not
//! a meaningful operation, not a missing one, so this concept treats
//! `transpose`/`transpose_with` as conditional the same way
//! `applicative_object` treats `ap` and `subsume`. An `OBJ` that declares
//! `consuming_traversable_object` is probed a second time on an rvalue
//! structure, for the reason `traversable_impl` records; one that does not
//! declare it is not asked, and presents `const` lvalues. The condition is
//! `transposing_object`, which is the one those two operations' own
//! declarations carry, not a second spelling of it: a condition that merely
//! asked whether `pure` were usable would be the weaker of the two, and
//! would demand an operation that is constrained out for every element type
//! lying between them. This concept does not
//! require a Foldable object: Traversable needs only an Applicative and the
//! walk, the DELIBERATE CONSTRAINT `Traversable` itself carries.
template <class OBJ, class STRUCTURE>
concept traversable_object =
    requires(const OBJ &obj, const STRUCTURE &structure) {
        obj.traverse(applicative_typeclass<
                         std::optional<applicative_value_t<STRUCTURE>>>,
                     detail::probe_witness<
                         std::optional<applicative_value_t<STRUCTURE>>>{},
                     structure);
        obj.for_each(structure,
                     detail::probe_witness<
                         std::optional<applicative_value_t<STRUCTURE>>>{});
        obj.traverse_with(obj,
                          detail::probe_witness<
                              std::optional<applicative_value_t<STRUCTURE>>>{},
                          structure);
    } &&
    (!consuming_traversable_object<OBJ> ||
     requires(const OBJ &obj) {
         obj.traverse(applicative_typeclass<
                          std::optional<applicative_value_t<STRUCTURE>>>,
                      detail::consuming_probe_witness<
                          applicative_value_t<STRUCTURE>,
                          std::optional<applicative_value_t<STRUCTURE>>>{},
                      std::declval<STRUCTURE>());
     }) &&
    (!transposing_object<OBJ> ||
     requires(const OBJ &obj, const STRUCTURE &structure) {
         obj.transpose(structure);
         obj.transpose_with(obj, structure);
     });

// \rSec3[transpose.traversable.ops]{Traversal operations}

//! \effects Applies `function` to each element of `value` and transposes the
//! resulting contextual values, preserving the shape of `value`. The
//! applicative object is the one `applicative_typeclass` names for the
//! context `function` returns.
//! \returns The shape of `value` held in that single context.
//! \complexity Exactly one application of `function` per element of `value`.
//! \remarks Elements are visited in the structure's iteration order.
template <class Impl>
template <class T, class F>
auto Traversable<Impl>::for_each(this auto &&self, T &&value, F &&function) {
    using Context =
        std::remove_cvref_t<std::invoke_result_t<F &, const element_type &>>;
    const auto &applicative = applicative_typeclass<Context>;
    return self.traverse(applicative, std::forward<F>(function),
                         std::forward<T>(value));
}

//! \constraints `element_type` is a context for which
//! `applicative_typeclass` names an applicative object.
//! \effects Equivalent to traversing `value` with the identity function: a
//! structure of contextual values becomes a single contextual value of the
//! structure, preserving shape.
//! \returns That single contextual value.
//! \remarks Elements are visited in the structure's iteration order. The
//! constraint is what lets a caller ask whether this operation is available
//! for a structure whose elements are not a context: the return type is
//! deduced, so without it the only answer available is a diagnostic from
//! inside the body.
template <class Impl>
template <class T>
    requires applicative_context<typename Impl::element_type>
auto Traversable<Impl>::transpose(this auto &&self, T &&value) {
    using Context = element_type;
    const auto &applicative = applicative_typeclass<Context>;
    return self.traverse(
        applicative, [](auto &&x) { return std::forward<decltype(x)>(x); },
        std::forward<T>(value));
}

// \rSec3[transpose.traversable.delegate]{Delegated traversal}

//! \effects Traverses `value` using the traversable object
//! `traversable_map` rather than `*this`. The applicative object is the one
//! `applicative_typeclass` names for the context `function` returns.
//! \returns The result of that traversal.
template <class Impl>
template <class TRAVERSABLE_MAP, class T, class F>
auto Traversable<Impl>::traverse_with(this auto &&,
                                      const TRAVERSABLE_MAP &traversable_map,
                                      F &&function, T &&value) {
    using Context = std::remove_cvref_t<std::invoke_result_t<
        F &,
        const typename std::remove_cvref_t<TRAVERSABLE_MAP>::element_type &>>;
    const auto &applicative = applicative_typeclass<Context>;
    return traversable_map.traverse(applicative, std::forward<F>(function),
                                    std::forward<T>(value));
}

//! \effects-equiv
template <class Impl>
template <class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
auto Traversable<Impl>::traverse_with(this auto &&,
                                      const TRAVERSABLE_MAP &traversable_map,
                                      const APPLICATIVE_MAP &applicative_map,
                                      F &&function, T &&value) {
    return traversable_map.traverse(applicative_map, std::forward<F>(function),
                                    std::forward<T>(value));
}

//! \effects-equiv
template <class Impl>
template <class TRAVERSABLE_MAP, class T>
    requires applicative_context<
        typename std::remove_cvref_t<TRAVERSABLE_MAP>::element_type>
auto Traversable<Impl>::transpose_with(this auto &&self,
                                       const TRAVERSABLE_MAP &traversable_map,
                                       T &&value) {
    return self.traverse_with(
        traversable_map, [](auto &&x) { return std::forward<decltype(x)>(x); },
        std::forward<T>(value));
}

// \rSec3[transpose.alg.traverse]{traverse}

//! \constraints `POLICY` satisfies `applicative_object_for` for the context
//! that applying `function` to an element of `value` yields.
//! \effects Applies `function` to each element of `value` and composes the
//! resulting contextual values with `policy`, preserving the shape of
//! `value`. Elements are visited in the structure's iteration order, and the
//! contextual values are composed in that same order.
//! \returns The shape of `value` held in the single context `policy`
//! composes into.
//! \complexity Exactly one application of `function` per element of `value`.
//! \remarks Let CONTEXT be the type that applying `function` to an element
//! of `value` yields. `POLICY` defaults to the type of the applicative
//! object `applicative_typeclass<CONTEXT>` names, which retains the first
//! failure and discards the rest, and the constraint is
//! `applicative_object_for<POLICY, CONTEXT>`. Passing the object
//! `accumulating_applicative_typeclass<CONTEXT>` names instead composes
//! every element's evidence. The two differ in what the result carries, not
//! in what runs: under either, `function` is applied to every element, as
//! the Complexity clause above says, and it is only the reconstruction of
//! the value in context that stops. No element's context depends on another
//! element's value, so this is independent contextual composition rather
//! than sequential dependence -- which is also why retaining the first
//! failure is as far as the default policy can go. There is nothing for a
//! composition to decline to produce: by the time it sees an element's
//! context, that context has already been computed.
template <class F, class T,
          class POLICY = std::remove_cvref_t<
              decltype(applicative_typeclass<traverse_context_t<F, T>>)>>
    requires applicative_object_for<POLICY, traverse_context_t<F, T>>
auto traverse(F &&function, T &&value, POLICY policy = POLICY{}) {
    const auto &map = traversable_typeclass<std::remove_cvref_t<T>>;
    return map.traverse(policy, std::forward<F>(function),
                        std::forward<T>(value));
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_TRAVERSE_HPP
