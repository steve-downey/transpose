// include/beman/transpose/sequence.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_SEQUENCE_HPP
#define BEMAN_TRANSPOSE_SEQUENCE_HPP

// Foldable and Traversable instances for std::vector.
//
// std::vector is the shared traversable *structure* for all three of Paper A's
// motivating domains. Pairing this one Traversable instance with three
// different applicative contexts gives the three front-door results through a
// single API:
//
//   traverse(f, vector<optional<T>>) -> optional<vector<T>>   (fallible value)
//   traverse(f, vector<sender<T>>)   -> sender<vector<T>>     (deferred result)
//   traverse(f, vector<zip_list<T>>) -> zip_list<vector<T>>   (lanewise / SIMD)
//
// The traverse primitive sequences each element's effect left-to-right, lifting
// element results back into a vector while preserving the original shape.

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/traverse.hpp>

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace beman::transpose {

/// Foldable instance for std::vector: fold_map accumulates left-to-right.
/// Foldable is not proposed by this paper -- the fold family is the
/// companion recursive-tree-algorithms paper's -- so this instance carries
/// no wording.
//! \omit
template <class VALUE_TYPE>
struct VectorFoldableImpl {
    // The trailing return type is the Impl-keeps-its-basis-SFINAE-friendly
    // invariant apply.hpp records, applied to the fold basis: computed only
    // in the body, the result type would be established by instantiating
    // that body, and an availability probe would diagnose from inside it
    // rather than fail to match.
    //! \omit
    template <class FUNCTION>
    auto fold_map(this auto &&, FUNCTION &&function,
                  const std::vector<VALUE_TYPE> &values)
        -> std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const VALUE_TYPE &>> {
        using Result = std::remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const VALUE_TYPE &>>;
        auto accumulated = monoid_identity<Result>();
        for (const auto &value : values) {
            accumulated = monoid_combine(std::move(accumulated),
                                         std::invoke(function, value));
        }
        return accumulated;
    }

    //! \omit
    auto length(this auto &&, const std::vector<VALUE_TYPE> &values)
        -> std::size_t {
        return values.size();
    }
};

//! \omit
template <class VALUE_TYPE>
struct VectorFoldableMap : Foldable<VectorFoldableImpl<VALUE_TYPE>> {
    using VectorFoldableImpl<VALUE_TYPE>::fold_map;
};

/** Foldable instance for `std::vector<VALUE_TYPE>`. */
template <class VALUE_TYPE>
inline constexpr auto foldable_typeclass<std::vector<VALUE_TYPE>> =
    VectorFoldableMap<VALUE_TYPE>{};

/// Traversable instance for std::vector.
/// The primitive `traverse` sequences each element's effect via the supplied
/// applicative, collecting the in-context element results into a vector while
/// preserving the original element order and count.
template <class VALUE_TYPE>
struct VectorTraversableImpl {
    using element_type = VALUE_TYPE;

    // \ref{transpose.range.traverse}, traversable instance for vector
    template <class APPLICATIVE, class FUNCTION>
    auto traverse(this auto &&, const APPLICATIVE &applicative,
                  FUNCTION &&function, const std::vector<VALUE_TYPE> &values);

    template <class APPLICATIVE, class FUNCTION>
    auto traverse(this auto &&, const APPLICATIVE &applicative,
                  FUNCTION &&function, std::vector<VALUE_TYPE> &&values);
};

template <class VALUE_TYPE>
struct VectorTraversableMap : Traversable<VectorTraversableImpl<VALUE_TYPE>> {
    using VectorTraversableImpl<VALUE_TYPE>::traverse;
};

/// Traversable instance for `std::vector<VALUE_TYPE>`.
template <class VALUE_TYPE>
inline constexpr auto traversable_typeclass<std::vector<VALUE_TYPE>> =
    VectorTraversableMap<VALUE_TYPE>{};

namespace detail {

/// The callable the vector traversal composes with: appends one element to
/// the collected vector and returns it.
///
/// A named object rather than a lambda-expression, for the reason
/// `applicative_eval_t` is one: a lambda is a distinct, unrelated type at
/// every occurrence in the source, and the two `traverse` overloads must
/// name the same callable.
///
/// Both parameters are taken by value. That is what turns an applicative
/// object's rvalue composition into a move: the accumulated vector is
/// move-constructed into `collected` instead of copied, so appending one
/// element costs one element, not one whole prefix.
//! \omit
template <class ELEMENT>
struct vector_append_t {
    auto operator()(std::vector<ELEMENT> collected, ELEMENT element) const
        -> std::vector<ELEMENT> {
        collected.push_back(std::move(element));
        return collected;
    }
};

//! \omit
template <class ELEMENT>
inline constexpr vector_append_t<ELEMENT> vector_append{};

/// Whether `APPLICATIVE` offers a native composition for a runtime-sized
/// collection of `EFFECT` operands.
///
/// The Traversable-side instance of
/// `docs/decisions.md#derived-op-native-preference`: prefer a native
/// operation where the object supplies one, derive otherwise. The probe
/// names the same expression the preferring branch evaluates -- same
/// receiver, same spelling, a `vector<EFFECT>` rvalue -- so an object whose
/// `collect` does not accept this traversal's operands takes the fold rather
/// than failing.
///
/// DELIBERATE CONSTRAINT: this names no context, no carrier and no concept
/// beyond the member itself. Whether anything answers it is the applicative
/// object's business; the Traversable's business is to ask. See
/// `docs/decisions.md#runtime-arity-composition`, whose Sentinel this
/// spelling exists to keep.
//! \omit
template <class APPLICATIVE, class EFFECT>
concept collecting_applicative = requires(const APPLICATIVE &applicative) {
    applicative.collect(std::declval<std::vector<EFFECT>>());
};

} // namespace detail

// \rSec3[transpose.range.traverse]{Traversable instance for vector}

//! \effects Let `E` be the type `function` returns for an element of
//! `values`. Applies `function` to each element of `values` in order. If
//! `applicative.collect(v)` is a valid expression for an rvalue `vector<E>`
//! `v`, composes the resulting contextual values by passing all of them to
//! that one call; otherwise composes them pairwise with `applicative`,
//! collecting the element results into a `vector`.
//! \returns A `vector` of the element results, of the same size as `values`
//! and in the same order, held in the single context `applicative` composes
//! into.
//! \complexity Exactly `values.size()` applications of `function`. Where
//! `applicative` has no `collect`, `values.size()` composition operations on
//! `applicative`, and where it composes an rvalue operand without
//! duplicating the value it holds -- as every applicative object this
//! library registers does -- the total number of element operations is
//! linear in `values.size()`. Where `applicative` has `collect`, exactly one
//! composition operation, whose own complexity is that operation's to state.
//! \remarks Traversal preserves shape: the result holds one element per
//! element of `values`, in the same order. Elements are visited in the
//! vector's iteration order, and their contexts are composed in that same
//! order. Preferring `collect` is the Traversable-side instance of the rule
//! every derived operation in this library follows: an object that supplies
//! a native operation is asked for it, and one that does not is derived
//! against unchanged. It is what admits an `applicative` whose composition
//! is not type-stable -- a pairwise fold must assign each partial result
//! back into a variable of one type, which a context whose combination names
//! a new type at every step cannot do -- and the requirement is stated as
//! the expression rather than as a property of the context so that this
//! operation stays generic over what supplies it. On the pairwise path the
//! accumulated result is handed to each composition as an rvalue: composing
//! it as an lvalue would oblige `applicative` to copy the whole prefix built
//! so far, once per element, making a successful traversal quadratic rather
//! than linear in the elements it collects.
template <class VALUE_TYPE>
template <class APPLICATIVE, class FUNCTION>
auto VectorTraversableImpl<VALUE_TYPE>::traverse(
    this auto &&, const APPLICATIVE &applicative, FUNCTION &&function,
    const std::vector<VALUE_TYPE> &values) {
    using Effect = std::remove_cvref_t<
        std::invoke_result_t<FUNCTION &, const VALUE_TYPE &>>;

    if constexpr (detail::collecting_applicative<APPLICATIVE, Effect>) {
        std::vector<Effect> effects;
        effects.reserve(values.size());
        for (const auto &value : values) {
            effects.push_back(std::invoke(function, value));
        }
        return applicative.collect(std::move(effects));
    } else {
        using Element = applicative_value_t<Effect>;

        std::vector<Element> collected;
        collected.reserve(values.size());

        auto accumulated = applicative.pure(std::move(collected));
        for (const auto &value : values) {
            accumulated = applicative.invoke(detail::vector_append<Element>,
                                             std::move(accumulated),
                                             std::invoke(function, value));
        }
        return accumulated;
    }
}

//! \effects Equivalent to the preceding overload, except that each element of
//! `values` is passed to `function` as an rvalue.
//! \returns As the preceding overload.
//! \complexity As the preceding overload.
//! \remarks This overload is what makes a traversal that consumes its
//! argument -- `transpose(std::move(v))` -- express that intent through to
//! `function`, and what admits an element type that can be moved but not
//! copied. `values` is left in a valid but unspecified state.
template <class VALUE_TYPE>
template <class APPLICATIVE, class FUNCTION>
auto VectorTraversableImpl<VALUE_TYPE>::traverse(
    this auto &&, const APPLICATIVE &applicative, FUNCTION &&function,
    std::vector<VALUE_TYPE> &&values) {
    using Effect =
        std::remove_cvref_t<std::invoke_result_t<FUNCTION &, VALUE_TYPE &&>>;

    if constexpr (detail::collecting_applicative<APPLICATIVE, Effect>) {
        std::vector<Effect> effects;
        effects.reserve(values.size());
        for (auto &value : values) {
            effects.push_back(std::invoke(function, std::move(value)));
        }
        return applicative.collect(std::move(effects));
    } else {
        using Element = applicative_value_t<Effect>;

        std::vector<Element> collected;
        collected.reserve(values.size());

        auto accumulated = applicative.pure(std::move(collected));
        for (auto &value : values) {
            accumulated = applicative.invoke(
                detail::vector_append<Element>, std::move(accumulated),
                std::invoke(function, std::move(value)));
        }
        return accumulated;
    }
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_SEQUENCE_HPP
