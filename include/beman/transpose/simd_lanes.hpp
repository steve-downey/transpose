// include/beman/transpose/simd_lanes.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_SIMD_LANES_HPP
#define BEMAN_TRANSPOSE_SIMD_LANES_HPP

// Non-normative demonstration type. simd_lanes models a fixed-width lanewise
// applicative context for Paper A's SIMD domain, backed by std::simd::vec for
// arithmetic element types. Unlike zip_list (which has dynamic/infinite width),
// simd_lanes has a compile-time width N matching a hardware SIMD register.
//
// Transposing a structure of simd_lanes produces a simd_lanes of structures:
//   vector<simd_lanes<T, N>>  ->  simd_lanes<vector<T>, N>
//
// The general storage is std::array<T, N> because the transposed result may
// hold non-arithmetic T (e.g., vector<int>). Examples show construction from
// std::simd::vec for the arithmetic case.

#include <beman/transpose/apply.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace beman::transpose {

template <class T, int N>
struct simd_lanes {
    using value_type = T;
    static constexpr int width = N;

    std::array<T, N> data;

    static auto repeat(T value) -> simd_lanes {
        // Built element by element rather than default-constructed and
        // filled, so T need only be copy-constructible.
        return simd_lanes{detail::make_array<T, static_cast<std::size_t>(N)>(
            [&value](std::size_t) -> T { return T(value); },
            std::make_index_sequence<static_cast<std::size_t>(N)>{})};
    }

    friend auto operator==(const simd_lanes &, const simd_lanes &)
        -> bool = default;
};

namespace detail {

/** Whether `T`, ignoring cv-qualification and reference, is a `simd_lanes`
 * of exactly width `N` -- the operand shape this applicative composes. Lets
 * `invoke` deduce its operands through forwarding references, which is what
 * carries the caller's value category into the operation, without accepting
 * operands of some other shape. The counterpart of
 * `is_std_array_of_extent` for the lanewise objects' fixed-width member;
 * the width is part of the shape for the reason recorded there. */
template <class T, int N>
struct is_simd_lanes_of_width : std::false_type {};

template <class U, int N>
struct is_simd_lanes_of_width<simd_lanes<U, N>, N> : std::true_type {};

template <class T, int N>
inline constexpr bool is_simd_lanes_of_width_v =
    is_simd_lanes_of_width<remove_cvref_t<T>, N>::value;

} // namespace detail

template <class T, int N>
struct SimdLanesApplicativeImpl {
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        using U = remove_cvref_t<VALUE>;
        return simd_lanes<U, N>::repeat(U(std::forward<VALUE>(value)));
    }

    // Operands are deduced through forwarding references so that an rvalue
    // operand's lanes are handed to `function` rather than copied. Each lane
    // is read exactly once, so that is safe. It matters because the
    // accumulator of a traversal into this context is an rvalue at every
    // step: copying it would rebuild the whole accumulated result per lane.
    // The operands are constrained on their shape for the same reason the
    // other registered objects constrain theirs: the return type is deduced,
    // so an operand of some other shape would reach the body and diagnose
    // there rather than leave this overload unmatched.
    template <class FUNCTION, class FIRST, class... REST>
        requires detail::is_simd_lanes_of_width_v<FIRST, N> &&
                 (detail::is_simd_lanes_of_width_v<REST, N> && ...)
    auto invoke(this auto &&, FUNCTION &&function, FIRST &&first,
                REST &&...rest) {
        using Result = std::invoke_result_t<
            FUNCTION &,
            detail::element_ref_t<decltype(std::forward<FIRST>(first).data)>,
            detail::element_ref_t<decltype(std::forward<REST>(rest).data)>...>;
        using U = remove_cvref_t<Result>;

        // Lanes are constructed, not default-constructed and assigned, so U
        // need not be default-constructible or assignable.
        return simd_lanes<U, N>{
            detail::make_array<U, static_cast<std::size_t>(N)>(
                [&](std::size_t index) -> U {
                    return std::invoke(
                        function,
                        detail::forward_at(std::forward<FIRST>(first).data,
                                           index),
                        detail::forward_at(std::forward<REST>(rest).data,
                                           index)...);
                },
                std::make_index_sequence<static_cast<std::size_t>(N)>{})};
    }
};

/** Applicative map for simd_lanes<T, N>: the native n-ary invoke core. */
template <class T, int N>
struct SimdLanesApplicativeMap : Applicative<SimdLanesApplicativeImpl<T, N>> {
    using SimdLanesApplicativeImpl<T, N>::invoke;
    using SimdLanesApplicativeImpl<T, N>::pure;
};

template <class T, int N>
inline constexpr auto applicative_typeclass<simd_lanes<T, N>> =
    SimdLanesApplicativeMap<T, N>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_SIMD_LANES_HPP
