// include/beman/transpose/simd.hpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_SIMD_HPP
#define BEMAN_TRANSPOSE_SIMD_HPP

// Applicative instance for std::simd::basic_vec (P1928, C++26): the context
// that PROVES the invoke core. A vec<T> holds only vectorizable scalars, so
// vec<callable> is not a type: `apply` cannot be spelled at all, and the
// base's derived apply must (and does) constrain itself away. pure is the
// broadcast constructor; invoke is the lane-wise generator constructor.
//
// NOTE: this is an applicative instance only -- NOT a traversal context for
// rebuilding structures (a vec cannot hold a vector<T> either). It is the
// in-tree example of an Applicative that is not Traversable, the reason the
// two typeclasses are distinct. The traversal-capable lanewise context
// remains simd_lanes<T, N>.

#if __has_include(<simd>) && __cplusplus > 202302L

#include <beman/transpose/apply.hpp>

#include <simd>

#include <functional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

/** Invoke-native Applicative Impl for std::simd::basic_vec<T, ABI>.
 * No `apply` is written and none can be derived: there is no vec of
 * callables to hold a function-in-context. pure and invoke carry trailing
 * return types / constraints so every probe fails cleanly (SFINAE) rather
 * than hard-erroring.
 */
template <class T, class ABI>
struct SimdVecApplicativeImpl {
    static constexpr auto width = std::simd::basic_vec<T, ABI>::size();

    /** Broadcast a scalar to every lane. */
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value)
        -> std::simd::vec<remove_cvref_t<VALUE>, width>
        requires requires {
            std::simd::vec<remove_cvref_t<VALUE>, width>(
                std::forward<VALUE>(value));
        }
    {
        return std::simd::vec<remove_cvref_t<VALUE>, width>(
            std::forward<VALUE>(value));
    }

    /** N-ary core: a plain function applied lane by lane across the
     * operands, via the generator constructor. */
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function, const FIRST &first,
                const REST &...rest)
        -> std::simd::vec<remove_cvref_t<std::invoke_result_t<
                              FUNCTION &, const typename FIRST::value_type &,
                              const typename REST::value_type &...>>,
                          width> {
        using U = remove_cvref_t<
            std::invoke_result_t<FUNCTION &, const typename FIRST::value_type &,
                                 const typename REST::value_type &...>>;
        return std::simd::vec<U, width>([&](auto lane) -> U {
            return std::invoke(function, first[lane], rest[lane]...);
        });
    }
};

/** Invoke-only map: no native apply, and the base's derived apply cannot
 * instantiate for vec operands -- that non-instantiation is the point. */
template <class T, class ABI>
struct SimdVecApplicativeMap : Applicative<SimdVecApplicativeImpl<T, ABI>> {
    using SimdVecApplicativeImpl<T, ABI>::invoke;
    using SimdVecApplicativeImpl<T, ABI>::pure;
};

/** Registers the Applicative instance for std::simd::basic_vec<T, ABI>,
 * which the std::simd::vec<T, N> alias pattern-matches. */
template <class T, class ABI>
inline constexpr auto applicative_typeclass<std::simd::basic_vec<T, ABI>> =
    SimdVecApplicativeMap<T, ABI>{};

} // namespace beman::transpose

#endif // __has_include(<simd>) && __cplusplus > 202302L
#endif // BEMAN_TRANSPOSE_SIMD_HPP
