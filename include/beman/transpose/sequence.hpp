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
    //! \omit
    template <class FUNCTION>
    auto fold_map(this auto &&, FUNCTION &&function,
                  const std::vector<VALUE_TYPE> &values) {
        using Result =
            remove_cvref_t<std::invoke_result_t<FUNCTION &, const VALUE_TYPE &>>;
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
template <class ELEMENT>
struct vector_append_t {
    auto operator()(std::vector<ELEMENT> collected, ELEMENT element) const
        -> std::vector<ELEMENT> {
        collected.push_back(std::move(element));
        return collected;
    }
};

template <class ELEMENT>
inline constexpr vector_append_t<ELEMENT> vector_append{};

} // namespace detail

// \rSec3[transpose.range.traverse]{Traversable instance for vector}

//! \effects Applies `function` to each element of `values` in order and
//! composes the resulting contextual values with `applicative`, collecting
//! the element results into a `vector`.
//! \returns A `vector` of the element results, of the same size as `values`
//! and in the same order, held in the single context `applicative` composes
//! into.
//! \complexity Exactly `values.size()` applications of `function`, and
//! `values.size()` composition operations on `applicative`. Where
//! `applicative` composes an rvalue operand without duplicating the value it
//! holds -- as every applicative object this library registers does -- the
//! total number of element operations is linear in `values.size()`.
//! \remarks Traversal preserves shape: the result holds one element per
//! element of `values`, in the same order. Elements are visited in the
//! vector's iteration order, and their contexts are composed in that same
//! order. The accumulated result is handed to each composition as an rvalue:
//! composing it as an lvalue would oblige `applicative` to copy the whole
//! prefix built so far, once per element, making a successful traversal
//! quadratic rather than linear in the elements it collects.
template <class VALUE_TYPE>
template <class APPLICATIVE, class FUNCTION>
auto VectorTraversableImpl<VALUE_TYPE>::traverse(
    this auto &&, const APPLICATIVE &applicative, FUNCTION &&function,
    const std::vector<VALUE_TYPE> &values) {
    using Effect =
        remove_cvref_t<std::invoke_result_t<FUNCTION &, const VALUE_TYPE &>>;
    using Element = applicative_value_t<Effect>;

    std::vector<Element> collected;
    collected.reserve(values.size());

    auto accumulated = applicative.pure(std::move(collected));
    for (const auto &value : values) {
        accumulated = applicative.invoke(
            detail::vector_append<Element>, std::move(accumulated),
            std::invoke(function, value));
    }
    return accumulated;
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
        remove_cvref_t<std::invoke_result_t<FUNCTION &, VALUE_TYPE &&>>;
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

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_SEQUENCE_HPP
