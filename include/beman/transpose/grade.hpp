// include/beman/transpose/grade.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_GRADE_HPP
#define BEMAN_TRANSPOSE_GRADE_HPP

// The grade algebra: framework vocabulary only.
//
// A grade is a compile-time index on an effectful computation. This header
// says what a grade algebra IS and how the framework asks questions of one.
// It says nothing whatever about errors. `error_set`, `recover`, and
// ⊆-as-may-raise are the vocabulary of one particular MODEL, and they live in
// the model's header; an abstraction whose framework layer names its only
// model is renamed, not generic. See docs/decisions.md#grade-generality and
// docs/decisions.md#grade-machinery-home.
//
// Nothing here is on the typeclass surface. `grade_of` and `rebind_grade` are
// framework traits with structural defaults, and the algebra verbs are free
// functions in the shape the library already uses for `monoid_combine` and
// `monoid_identity` -- see docs/decisions.md#grade-operation-spelling for why
// they carry the `grade_` prefix rather than taking the bare name `join`,
// which is already the monadic join.
//
// The load-bearing default is `grade_of`: an instance that knows nothing
// about grades is treated as uniformly ∅-graded and keeps working verbatim.
// That default IS the backward-compatibility mechanism of
// docs/decisions.md#grading-footprint, not a convenience.

#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <type_traits>

namespace beman::transpose {

/** The grade of a computation that raises nothing: ∅, in any algebra.
 *
 * Deliberately NOT the shipped model's bottom. If the framework's default
 * grade were `error_set<>`, the framework would name an error type in order
 * to say "no errors", and a second grade algebra could not be registered
 * without inheriting the first one's vocabulary. `unit_grade` is the identity
 * of every join, so it composes with any model's grades.
 *
 * Note the distinction from docs/decisions.md#empty-grade-spelling: that
 * decision is about how an ∅-graded CARRIER is spelled (bare `T`). This is
 * the grade itself, as an index.
 */
struct unit_grade {};

// -- Algebra verbs. Primaries are undefined on purpose: a type is a grade
// -- exactly when a model has registered these for it, so an unregistered
// -- type fails the concept instead of silently behaving like one.

/** Join: the least upper bound of two grades. */
template <class LEFT, class RIGHT>
struct grade_join;

/** Bottom: the identity of join, in LEFT's algebra. */
template <class GRADE>
struct grade_bottom;

/** Order: does LEFT subsume into RIGHT? By order-from-join this is
 * `grade_join_t<LEFT, RIGHT>` being RIGHT, which is what makes subsumption
 * coercions canonical and therefore coherently implicit. */
template <class LEFT, class RIGHT>
struct grade_subsume;

template <class LEFT, class RIGHT>
using grade_join_t = typename grade_join<LEFT, RIGHT>::type;

template <class GRADE>
using grade_bottom_t = typename grade_bottom<GRADE>::type;

template <class LEFT, class RIGHT>
inline constexpr bool grade_subsume_v = grade_subsume<LEFT, RIGHT>::value;

/** A type is a grade when a model has given it the three algebra verbs.
 *
 * Syntactic only. The semilattice LAWS -- commutativity, idempotence,
 * associativity, unit -- are semantic requirements stated in prose and
 * checked by the opt-in law harness. Putting them in the concept would drag
 * a full law check into every constraint that mentions a grade, which is the
 * pay-once economics the standard's own posture avoids.
 */
template <class GRADE>
concept grade_semilattice = requires {
    typename grade_join<GRADE, GRADE>::type;
    typename grade_bottom<GRADE>::type;
    { grade_subsume<GRADE, GRADE>::value } -> std::convertible_to<bool>;
};

// -- unit_grade is a grade in its own right, and the identity of every other
// -- algebra's join. The constrained partials keep it from absorbing types
// -- that are not grades at all.

template <>
struct grade_join<unit_grade, unit_grade> {
    using type = unit_grade;
};

template <class RIGHT>
    requires requires { typename grade_bottom<RIGHT>::type; }
struct grade_join<unit_grade, RIGHT> {
    using type = RIGHT;
};

template <class LEFT>
    requires requires { typename grade_bottom<LEFT>::type; }
struct grade_join<LEFT, unit_grade> {
    using type = LEFT;
};

template <>
struct grade_bottom<unit_grade> {
    using type = unit_grade;
};

template <>
struct grade_subsume<unit_grade, unit_grade> : std::true_type {};

/** ∅ subsumes into everything: a computation that raises nothing is usable
 * wherever one that may raise something is. */
template <class RIGHT>
    requires requires { typename grade_bottom<RIGHT>::type; }
struct grade_subsume<unit_grade, RIGHT> : std::true_type {};

/** Nothing but ∅ subsumes into ∅. */
template <class LEFT>
    requires requires { typename grade_bottom<LEFT>::type; }
struct grade_subsume<LEFT, unit_grade> : std::false_type {};

// -- Carrier traits -------------------------------------------------------

/** The grade carried by a context type.
 *
 * The structural default is ∅, and it is the whole backward-compatibility
 * story: every carrier that predates grading -- and every gadget a user
 * registers without knowing grades exist -- answers `unit_grade` here and
 * flows through the identity path unchanged. Models specialize this for the
 * shapes they actually grade.
 */
template <class CONTEXT>
struct grade_of {
    using type = unit_grade;
};

template <class CONTEXT>
using grade_of_t = typename grade_of<CONTEXT>::type;

/** The carrier CONTEXT re-indexed at GRADE.
 *
 * The framework default handles the only case it can know about: re-indexing
 * anything at ∅ leaves it alone. Every other pairing is the model's to
 * define, because only the model knows how its grade is spelled into a
 * carrier.
 */
template <class CONTEXT, class GRADE>
struct rebind_grade;

template <class CONTEXT>
struct rebind_grade<CONTEXT, unit_grade> {
    using type = CONTEXT;
};

template <class CONTEXT, class GRADE>
using rebind_grade_t = typename rebind_grade<CONTEXT, GRADE>::type;

/** True when CONTEXT carries a grade other than ∅.
 *
 * The framework uses this to keep graded and ungraded paths mutually
 * exclusive BY CONSTRAINT rather than by overload ranking, per
 * docs/decisions.md#grading-footprint.
 */
template <class CONTEXT>
concept graded_context = !std::is_same_v<grade_of_t<CONTEXT>, unit_grade>;

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_GRADE_HPP
