// include/beman/transpose/transpose.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TRANSPOSE_HPP
#define BEMAN_TRANSPOSE_TRANSPOSE_HPP

// Umbrella header for beman.transpose plus the front-door `transpose` verb.
//
// `transpose` flips a structure of contextual values into a single contextual
// value of the structure, preserving shape:
//
//   structure<context<T>>  ->  context<structure<T>>
//
// It is `traverse` with the identity function; the context's applicative is
// inferred from the structure's element type.

#include <beman/transpose/config.hpp>

#include <beman/transpose/apply.hpp>
#include <beman/transpose/array.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/dual_monoid.hpp>
#include <beman/transpose/fold.hpp>
#include <beman/transpose/functor.hpp>
#include <beman/transpose/monad.hpp>
#include <beman/transpose/monoid.hpp>
#include <beman/transpose/sender.hpp>
#include <beman/transpose/sequence.hpp>
#include <beman/transpose/traverse.hpp>
#include <beman/transpose/zip_list.hpp>

#if __has_include(<simd>)
#include <beman/transpose/simd.hpp> // self-gates further on C++26
#include <beman/transpose/simd_lanes.hpp>
#endif

#include <utility>

namespace beman::transpose {

/** Operation object for Traversable's derived `transpose`; the
 * object form of the verb, matching `traverse`.
 */
struct transpose_fn {
    /** @brief Transposes a structure of contextual values into a contextual
     *         structure, preserving shape: `structure<context<T>>` becomes
     *         `context<structure<T>>`.
     *
     * Equivalent to `traverse(identity, value)`; the applicative context is
     * inferred from the structure's element type.
     *
     * @param value  A traversable structure whose elements are an applicative
     *               context (e.g. `std::vector<std::optional<T>>`).
     * @return       The single outer context holding the structure.
     * @tparam TC  The Traversable instance, from the lookup for the keying
     * argument. Not for callers to supply; pin with mode 2.
     */
    template <class T, const auto &TC = traversable<remove_cvref_t<T>>>
    constexpr auto operator()(T &&value) const {
        static_assert(has_instance_v<decltype(TC)>,
                      "No Traversable instance for this type. Specialize "
                      "beman::transpose::traversable<T> with an "
                      "object providing traverse(applicative, f, container) "
                      "and 'using element_type = ...;'.");
        return TC.transpose(std::forward<T>(value));
    }
};

/** Structure of contexts to context of structure: `transpose(structure)`. */
inline constexpr transpose_fn transpose{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_TRANSPOSE_HPP
