// include/beman/transpose/simd.hpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_SIMD_HPP
#define BEMAN_TRANSPOSE_SIMD_HPP

// Real-hardware lanewise context for Paper A's third motivating domain.
//
// `zip_list` (zip_list.hpp) is a *model* of the lanewise applicative: it can
// hold composite payloads (e.g. zip_list<vector<T>>), so it can be the target
// of the `transpose` front-door verb. `std::simd::vec<T>` is the *real* type
// for the same independent, position-wise composition, but it can only hold
// vectorizable scalars. Two consequences, both load-bearing for the paper:
//
//   1. `transpose(vector<vec<T>>)` is NOT expressible: it would require
//      `vec<vector<T>>`, and `vec` cannot hold a `vector`. So std::simd
//      participates at the *applicative-operation* layer (pure / invoke /
//      zip_with / map), not at the front-door `transpose` verb. zip_list
//      carries the verb; std::simd proves the semantics are real hardware
//      data-parallelism.
//
//   2. `vec` has no expressible `apply` primitive, because `vec<callable>` is
//      ill-formed. The lanewise context's natural primitive is `invoke` (apply
//      a scalar function across all lanes). This is exactly the "alternate
//      core" the Applicative customization supports: this instance provides
//      pure + invoke and no apply. See apply.hpp.
//
// Non-normative demonstration support, not a proposed standard instance.
// Requires a C++26 standard library that ships <simd> (e.g. GCC 16); the
// header is inert otherwise.

#if defined(__has_include)
#if __has_include(<simd>)
#define BEMAN_TRANSPOSE_HAS_STD_SIMD 1
#endif
#endif

#if defined(BEMAN_TRANSPOSE_HAS_STD_SIMD)

#include <beman/transpose/apply.hpp>

#include <cstddef>
#include <functional>
#include <simd>
#include <type_traits>
#include <utility>

namespace beman::transpose {

/** Applicative instance for `std::simd::vec<T>` with lanewise (positional)
 * semantics. `pure(x)` broadcasts x to every lane; `invoke(f, a, b, ...)`
 * applies the scalar `f` independently in each lane. There is deliberately no
 * `apply`: `vec` cannot hold a callable, so `invoke` is the primitive core.
 */
template <class T>
struct SimdApplicativeImpl {
    /** Broadcast a scalar into every lane. */
    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) {
        using U = remove_cvref_t<VALUE>;
        return std::simd::vec<U>(U(std::forward<VALUE>(value)));
    }

    /** Lanewise application: build a result vector whose lane `i` is
     * `f(first[i], rest[i]...)`. All operands share the same static width.
     */
    template <class FUNCTION, class FIRST, class... REST>
    auto invoke(this auto &&, FUNCTION &&function, const FIRST &first,
                const REST &...rest) {
        using R = remove_cvref_t<std::invoke_result_t<
            FUNCTION &, decltype(first[std::size_t{0}]),
            decltype(rest[std::size_t{0}])...>>;
        auto callable = std::forward<FUNCTION>(function);
        return std::simd::vec<R>([&](auto lane) -> R {
            return std::invoke(callable, first[lane], rest[lane]...);
        });
    }
};

/** Applicative map for std::simd::vec<T>. Exposes pure; the base detects and
 * uses the Impl's invoke as the composition primitive (no apply exists). */
template <class T>
struct SimdApplicativeMap : Applicative<SimdApplicativeImpl<T>> {
    using SimdApplicativeImpl<T>::pure;
};

/** Registers SimdApplicativeMap as the Applicative instance for
 * `std::simd::vec<T>`. */
template <class T>
inline constexpr auto applicative_typeclass<std::simd::vec<T>> =
    SimdApplicativeMap<T>{};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_HAS_STD_SIMD

#endif // BEMAN_TRANSPOSE_SIMD_HPP
