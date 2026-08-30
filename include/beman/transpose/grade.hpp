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
// framework traits; `grade_of` defaults unregistered carriers to the
// ungraded sentinel, while models own registered carrier specializations.
// The algebra verbs are free functions in the shape the library already uses
// for `monoid_combine` and `monoid_identity` -- see
// docs/decisions.md#grade-operation-spelling for why they carry the `grade_`
// prefix rather than taking the bare name `join`, which is already the
// monadic join.
//
// The load-bearing default is `grade_of`: an instance that knows nothing
// about grades is treated as uniformly ∅-graded and keeps working verbatim.
// That default IS the backward-compatibility mechanism of
// docs/decisions.md#grading-footprint, not a convenience.

#include <beman/transpose/detail/typeclass_base.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace beman::transpose {

/** Sentinel for an ungraded computation.
 *
 * Deliberately NOT a grade in any model's lattice. If the framework's
 * default were a model bottom, the framework would name one model in order
 * to say "not yet in a family", and a second grade algebra would inherit the
 * first one's vocabulary. Mixing points are where the sentinel meets a
 * model: the model lifts ungraded operands to its own bottom.
 *
 * Note the distinction from docs/decisions.md#empty-grade-spelling: that
 * decision is about how an ∅-graded carrier is spelled (bare `T`). This is
 * the framework's model-less state.
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
struct grade_subsumes;

/** The model a grade belongs to. Models specialize this for their grades. */
template <class GRADE>
struct grade_model;

template <class LEFT, class RIGHT>
using grade_join_t = typename grade_join<LEFT, RIGHT>::type;

template <class GRADE>
using grade_bottom_t = typename grade_bottom<GRADE>::type;

template <class LEFT, class RIGHT>
inline constexpr bool grade_subsumes_v = grade_subsumes<LEFT, RIGHT>::value;

/** A type is a grade when a model has given it identity plus the three
 * algebra verbs.
 *
 * Syntactic only. The semilattice LAWS -- commutativity, idempotence,
 * associativity, unit -- are semantic requirements stated in prose and
 * checked by the opt-in law harness. Putting them in the concept would drag
 * a full law check into every constraint that mentions a grade, which is the
 * pay-once economics the standard's own posture avoids.
 */
template <class GRADE>
concept grade_semilattice = requires {
    typename grade_model<GRADE>::type;
    typename grade_join<GRADE, GRADE>::type;
    typename grade_bottom<GRADE>::type;
    { grade_subsumes<GRADE, GRADE>::value } -> std::convertible_to<bool>;
};

// -- Carrier traits -------------------------------------------------------

/** The grade carried by a context type.
 *
 * The structural default is the ungraded sentinel, and it is the whole
 * backward-compatibility story: every carrier that predates grading -- and
 * every gadget a user registers without knowing grades exist -- answers
 * `unit_grade` here and flows through the identity path unchanged. Models
 * specialize this for the shapes they actually grade.
 */
template <class CONTEXT>
struct grade_of {
    using type = unit_grade;
};

template <class CONTEXT>
using grade_of_t = typename grade_of<CONTEXT>::type;

/** The carrier CONTEXT re-indexed at GRADE.
 *
 * There is no framework default: the sentinel is not a grade and has no
 * carrier machinery. Every pairing is the model's to define, because only
 * the model knows how its grade is spelled into a carrier.
 */
template <class CONTEXT, class GRADE>
struct rebind_grade;

template <class CONTEXT, class GRADE>
using rebind_grade_t = typename rebind_grade<CONTEXT, GRADE>::type;

/** True when CONTEXT carries a model grade rather than the ungraded sentinel.
 *
 * The framework uses this to keep graded and ungraded paths mutually
 * exclusive BY CONSTRAINT rather than by overload ranking, per
 * docs/decisions.md#grading-footprint.
 */
template <class CONTEXT>
concept graded_context = !std::is_same_v<grade_of_t<CONTEXT>, unit_grade>;

// -- Model identity and detail helpers ------------------------------------

template <class GRADE>
using grade_model_t = typename grade_model<GRADE>::type;

namespace detail {

template <class... GRADES>
struct grade_join_all;

template <class GRADE>
struct grade_join_all<GRADE> {
    using type = GRADE;
};

template <class LEFT, class RIGHT, class... REST>
struct grade_join_all<LEFT, RIGHT, REST...>
    : grade_join_all<grade_join_t<LEFT, RIGHT>, REST...> {};

template <class... GRADES>
using grade_join_all_t = typename grade_join_all<GRADES...>::type;

template <class MODEL_GRADE, class OPERAND>
struct grade_lifted_into_model {
  private:
    using raw_grade = grade_of_t<remove_cvref_t<OPERAND>>;

  public:
    using type = std::conditional_t<std::is_same_v<raw_grade, unit_grade>,
                                    grade_bottom_t<MODEL_GRADE>, raw_grade>;
};

template <class MODEL_GRADE, class OPERAND>
using grade_lifted_into_model_t =
    typename grade_lifted_into_model<MODEL_GRADE, OPERAND>::type;

template <class MODEL_GRADE, class RAW_GRADE>
struct mixes_with_model_impl
    : std::bool_constant<
          std::is_same_v<grade_model_t<RAW_GRADE>, grade_model_t<MODEL_GRADE>>> {
};

template <class MODEL_GRADE>
struct mixes_with_model_impl<MODEL_GRADE, unit_grade> : std::true_type {};

template <class MODEL_GRADE, class OPERAND>
concept mixes_with_model = mixes_with_model_impl<
    MODEL_GRADE, grade_of_t<remove_cvref_t<OPERAND>>>::value;

template <class MODEL_GRADE, class... OPERANDS>
    requires(sizeof...(OPERANDS) > 0) &&
            (mixes_with_model<MODEL_GRADE, OPERANDS> && ...)
using mixed_grade_t =
    grade_join_all_t<grade_lifted_into_model_t<MODEL_GRADE, OPERANDS>...>;

template <class MODEL_GRADE, class CARRIER, class... OPERANDS>
using mixed_result_t =
    rebind_grade_t<CARRIER, mixed_grade_t<MODEL_GRADE, OPERANDS...>>;

/** Value-level evidence combination at one grade.
 *
 * Implementation detail for the accumulating expected object. The default is
 * left-biased same-type evidence, enough for single-slot errors; models with
 * multi-witness evidence can add a detail overload in their own header.
 */
template <class EVIDENCE>
constexpr auto combine_grade_evidence(const EVIDENCE &lhs,
                                      const EVIDENCE & /*rhs*/) -> EVIDENCE {
    return lhs;
}

} // namespace detail

/** Subsumption: use a computation at a wider grade.
 *
 * The defaulted coercion is the carrier's OWN converting constructor. There
 * is deliberately no bespoke machinery here: because the order is read off
 * the join, the inclusion between two grades is unique, so there is exactly
 * one coercion to write and the carrier already writes it. For the shipped
 * model that is `error_set`'s ⊆-only converting constructor; for the ∅ case
 * it is `expected`'s converting constructor from `T`, which is to say the
 * standard already implements η's coercion
 * (docs/decisions.md#empty-grade-spelling). The source may be the framework
 * sentinel; the target must be a model grade.
 *
 * Constrained, so it disappears cleanly rather than hard-erroring when the
 * widening is not licensed by the order or the carrier cannot express it.
 */
template <class TARGET_GRADE, class CARRIER>
    requires grade_semilattice<TARGET_GRADE> &&
             (std::is_same_v<grade_of_t<remove_cvref_t<CARRIER>>, unit_grade> ||
              grade_subsumes_v<grade_of_t<remove_cvref_t<CARRIER>>,
                                TARGET_GRADE>) &&
             std::constructible_from<
                 rebind_grade_t<remove_cvref_t<CARRIER>, TARGET_GRADE>, CARRIER>
constexpr auto grade_subsume(CARRIER &&value)
    -> rebind_grade_t<remove_cvref_t<CARRIER>, TARGET_GRADE> {
    return rebind_grade_t<remove_cvref_t<CARRIER>, TARGET_GRADE>(
        std::forward<CARRIER>(value));
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_GRADE_HPP
