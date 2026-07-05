// include/beman/transpose/array.hpp                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_ARRAY_HPP
#define BEMAN_TRANSPOSE_ARRAY_HPP

// Applicative instance for std::array<T, N> with positional (zip) semantics,
// plus a tuple-specific transpose for the SoA <-> AoS transformation:
//
//   tuple<array<T1, N>, array<T2, N>, ...>  ->  array<tuple<T1, T2, ...>, N>
//
// The array applicative applies functions lane-by-lane (like simd_lanes and
// zip_list) but over a standard fixed-size container. pure(x) broadcasts x
// to all N positions.
//
// The tuple transpose is a heterogeneous traversal: std::tuple cannot use the
// homogeneous Traversable loop, so transpose_tuple uses a fold over indices
// to combine the per-field arrays through the array applicative's invoke.

#include <beman/transpose/apply.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beman::transpose {

template <class T, std::size_t N>
struct ArrayApplicativeImpl {
    template <class VALUE>
    auto pure(this auto&&, VALUE&& value) {
        using U = remove_cvref_t<VALUE>;
        std::array<U, N> result;
        result.fill(U(std::forward<VALUE>(value)));
        return result;
    }

    template <class F, class A>
    auto apply(this auto&&, const std::array<F, N>& functions,
               const std::array<A, N>& arguments) {
        using Result = std::invoke_result_t<const F&, const A&>;
        using U = remove_cvref_t<Result>;

        std::array<U, N> result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = std::invoke(functions[i], arguments[i]);
        }
        return result;
    }

    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto&&, FUNCTION&& function, const FIRST& first,
                const REST&... rest) {
        using Result =
            std::invoke_result_t<FUNCTION, const typename FIRST::value_type&,
                                 const typename REST::value_type&...>;
        using U = remove_cvref_t<Result>;

        std::array<U, N> result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = std::invoke(function, first[i], rest[i]...);
        }
        return result;
    }
};

template <class T, std::size_t N>
struct ArrayApplicativeMap : Applicative<ArrayApplicativeImpl<T, N>> {
    using ArrayApplicativeImpl<T, N>::apply;
    using ArrayApplicativeImpl<T, N>::pure;
};

template <class T, std::size_t N>
inline constexpr auto applicative_typeclass<std::array<T, N>> =
    ArrayApplicativeMap<T, N>{};

namespace detail {

template <std::size_t N, class... Ts, std::size_t... Is>
auto transpose_tuple_impl(const std::tuple<std::array<Ts, N>...>& soa,
                          std::index_sequence<Is...>)
    -> std::array<std::tuple<Ts...>, N> {
    const auto& app = applicative_typeclass<std::array<std::tuple<Ts...>, N>>;
    return app.invoke(
        [](const Ts&... elems) { return std::tuple{elems...}; },
        std::get<Is>(soa)...);
}

} // namespace detail

/** Transpose a tuple of arrays (SoA) into an array of tuples (AoS).
 *
 * This is a heterogeneous traversal: each tuple element may have a different
 * value type, so the standard Traversable loop (which requires a single
 * element_type) does not apply. Instead, the fold happens at compile time
 * over the tuple indices via the array applicative's positional invoke.
 *
 * @param soa  A tuple where each element is a std::array of the same size N.
 * @return     An array of N tuples, one per position across the input arrays.
 */
template <std::size_t N, class... Ts>
auto transpose_tuple(const std::tuple<std::array<Ts, N>...>& soa)
    -> std::array<std::tuple<Ts...>, N> {
    return detail::transpose_tuple_impl(soa, std::index_sequence_for<Ts...>{});
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_ARRAY_HPP
