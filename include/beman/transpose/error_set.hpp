// include/beman/transpose/error_set.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_ERROR_SET_HPP
#define BEMAN_TRANSPOSE_ERROR_SET_HPP

// error_set: the set of error types a computation may raise.
//
// NOMINAL, not a structural sum. `error_set_of<Es...>` is its own type. It
// composes a std::variant for storage and visitation, and never slices into
// it: the public surface is deliberately impoverished -- injection,
// subsumption, membership, visitation -- because "the set of errors this
// computation may raise" is an interpretation, not a representation, and
// interpretations need names. See docs/decisions.md#error-set-identity.
//
// CANONICAL BY CONSTRUCTION. `error_set<Es...>` is the spelling users and the
// framework write; it is an alias that sorts and deduplicates its pack, so
// `error_set<A,B>` and `error_set<B,A>` are THE SAME TYPE. That is what makes
// grade arithmetic well-defined and inclusions unique -- sorted normalization
// is names-not-positions. `error_set_of` accepts only an already-canonical
// pack and says so with a static_assert; forming a non-canonical one is the
// invariant violation the class exists to prevent.
//
// The conversions are exactly the ⊆ widenings and nothing else. There is no
// narrowing conversion, no conversion between incomparable sets, and no
// accessor that hands out the underlying variant. Having only the subset
// conversions is the coherence argument compiled into the overload set.
//
// TYPE ORDERING is P2830's job. Until it is available this header uses an
// interim __PRETTY_FUNCTION__ ordering, valid only for the documented
// restrictions: named types, external linkage, stable spelling. Two distinct
// types that render to the same name would be silently conflated, so that
// case is a hard error rather than a wrong answer -- see the static_assert in
// error_set_of.

#include <beman/transpose/detail/typeclass_base.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace beman::transpose {

template <class... ERRORS>
class error_set_of;

namespace detail {

// -- Interim type ordering (P2830 fallback) -------------------------------

/** The compiler's spelling of T, extracted from the enclosing signature.
 * Valid as an ordering key only under the documented restrictions: named
 * types with external linkage and a stable spelling.
 */
template <class T>
consteval auto type_name() -> std::string_view {
    const std::string_view signature{__PRETTY_FUNCTION__};
    constexpr std::string_view marker{"T = "};

    const auto start = signature.find(marker) + marker.size();
    auto stop = signature.find(';', start);
    if (stop == std::string_view::npos) {
        stop = signature.rfind(']');
    }
    return signature.substr(start, stop - start);
}

/** Strict total order on types, by name. Interim stand-in for P2830. */
template <class LEFT, class RIGHT>
inline constexpr bool type_precedes_v = type_name<LEFT>() < type_name<RIGHT>();

// -- Type list machinery --------------------------------------------------

template <class... ERRORS>
struct type_list {};

template <class T, class LIST>
inline constexpr bool list_contains_v = false;

template <class T, class... ERRORS>
inline constexpr bool list_contains_v<T, type_list<ERRORS...>> =
    (std::is_same_v<T, ERRORS> || ...);

template <class T, class LIST>
struct list_prepend;

template <class T, class... ERRORS>
struct list_prepend<T, type_list<ERRORS...>> {
    using type = type_list<T, ERRORS...>;
};

template <class T, class LIST>
using list_prepend_t = typename list_prepend<T, LIST>::type;

template <class T, class LIST>
struct list_remove;

template <class T>
struct list_remove<T, type_list<>> {
    using type = type_list<>;
};

template <class T, class HEAD, class... TAIL>
struct list_remove<T, type_list<HEAD, TAIL...>> {
  private:
    using rest = typename list_remove<T, type_list<TAIL...>>::type;

  public:
    using type = std::conditional_t<std::is_same_v<T, HEAD>, rest,
                                    list_prepend_t<HEAD, rest>>;
};

/** Deduplicate by type identity, keeping the first occurrence. Identity is
 * std::is_same, never the name: names are an ordering key, not an equality. */
template <class LIST>
struct list_dedupe;

template <>
struct list_dedupe<type_list<>> {
    using type = type_list<>;
};

template <class HEAD, class... TAIL>
struct list_dedupe<type_list<HEAD, TAIL...>> {
    using type =
        list_prepend_t<HEAD, typename list_dedupe<typename list_remove<
                                 HEAD, type_list<TAIL...>>::type>::type>;
};

template <class T, class LIST>
struct list_insert_sorted;

template <class T>
struct list_insert_sorted<T, type_list<>> {
    using type = type_list<T>;
};

template <class T, class HEAD, class... TAIL>
struct list_insert_sorted<T, type_list<HEAD, TAIL...>> {
    using type = std::conditional_t<
        type_precedes_v<T, HEAD>, type_list<T, HEAD, TAIL...>,
        list_prepend_t<
            HEAD, typename list_insert_sorted<T, type_list<TAIL...>>::type>>;
};

template <class LIST>
struct list_sort;

template <>
struct list_sort<type_list<>> {
    using type = type_list<>;
};

template <class HEAD, class... TAIL>
struct list_sort<type_list<HEAD, TAIL...>> {
    using type = typename list_insert_sorted<
        HEAD, typename list_sort<type_list<TAIL...>>::type>::type;
};

/** The canonical form of a pack: deduplicated, then sorted. */
template <class... ERRORS>
using canonical_t =
    typename list_sort<typename list_dedupe<type_list<ERRORS...>>::type>::type;

/** False when two DISTINCT types render to the same name, which is exactly
 * the case the interim ordering cannot handle. */
template <class... ERRORS>
consteval auto names_are_distinct() -> bool {
    constexpr std::size_t count = sizeof...(ERRORS);
    const std::array<std::string_view, count> names{type_name<ERRORS>()...};
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            if (names[i] == names[j]) {
                return false;
            }
        }
    }
    return true;
}

template <class LIST>
struct error_set_from_list;

template <class... ERRORS>
struct error_set_from_list<type_list<ERRORS...>> {
    using type = error_set_of<ERRORS...>;
};

} // namespace detail

/** The set of error types a computation may raise, at a canonical pack.
 *
 * Write `error_set<...>` rather than naming this template directly: the alias
 * canonicalizes, this template only accepts an already-canonical pack.
 *
 * @tparam ERRORS the alternatives, sorted and deduplicated
 */
template <class... ERRORS>
class error_set_of {
    static_assert(
        std::is_same_v<detail::canonical_t<ERRORS...>,
                       detail::type_list<ERRORS...>>,
        "error_set_of requires a canonical (sorted, deduplicated) pack. "
        "Spell error_set<...> instead, which canonicalizes for you.");
    static_assert(
        detail::names_are_distinct<ERRORS...>(),
        "Two distinct error types render to the same name, so the interim "
        "type ordering cannot separate them. The P2830 fallback is valid "
        "only for named types with external linkage and a stable spelling; "
        "this is outside those restrictions.");

  public:
    /** Injection: an error value becomes the set it belongs to. */
    template <class ERROR>
        requires detail::list_contains_v<remove_cvref_t<ERROR>,
                                         detail::type_list<ERRORS...>>
    constexpr error_set_of(ERROR &&error) // NOLINT(*-explicit-constructor)
        : d_alternative(std::forward<ERROR>(error)) {}

    /** Subsumption: widening along ⊆, and nothing else. A narrower set is
     * usable wherever a wider one is; there is no conversion the other way,
     * and none between incomparable sets. Conversion FROM the empty set is
     * absent on purpose rather than by oversight: ∅ is uninhabited, so no
     * value ever needs it.
     */
    template <class... NARROWER>
        requires(sizeof...(NARROWER) > 0) &&
                (!std::is_same_v<error_set_of<NARROWER...>, error_set_of>) &&
                (detail::list_contains_v<NARROWER,
                                         detail::type_list<ERRORS...>> &&
                 ...)
    constexpr error_set_of(
        const error_set_of<NARROWER...> &narrower) // NOLINT(*-explicit-*)
        : d_alternative(narrower.visit([](const auto &error) {
              return std::variant<ERRORS...>{error};
          })) {}

    /** Membership, for `recover`: is ERROR one of the alternatives? */
    template <class ERROR>
    static constexpr auto contains() noexcept -> bool {
        return detail::list_contains_v<ERROR, detail::type_list<ERRORS...>>;
    }

    /** Which alternative this value currently holds. */
    template <class ERROR>
        requires(contains<ERROR>())
    constexpr auto holds() const noexcept -> bool {
        return std::holds_alternative<ERROR>(d_alternative);
    }

    /** Visitation, for `recover`: apply `handler` to the held alternative. */
    template <class HANDLER>
    constexpr auto visit(HANDLER &&handler) const -> decltype(auto) {
        return std::visit(std::forward<HANDLER>(handler), d_alternative);
    }

  private:
    std::variant<ERRORS...> d_alternative;
};

/** The empty error set: bottom of the lattice, and uninhabited.
 *
 * A computation graded ∅ raises nothing, so there is no value to hold. Note
 * that ∅ as a *grade* is spelled by the bare value type, not by this
 * (docs/decisions.md#empty-grade-spelling); this type is the lattice bottom
 * and the explicit uniform spelling, never something the framework deduces
 * on a user's behalf.
 */
template <>
class error_set_of<> {
  public:
    error_set_of() = delete;

    template <class ERROR>
    static constexpr auto contains() noexcept -> bool {
        return false;
    }
};

/** The canonicalizing spelling of an error set: sorted and deduplicated, so
 * that `error_set<A,B>` and `error_set<B,A>` denote the same type. */
template <class... ERRORS>
using error_set =
    typename detail::error_set_from_list<detail::canonical_t<ERRORS...>>::type;

// -- Semilattice operations, in error vocabulary --------------------------
// The framework layer speaks grade vocabulary and gets these through a model
// registration; that indirection arrives with the grade concept. Here they
// are the native API of the model itself.

/** Join: set union. Canonicalization reduces it to pack concatenation, which
 * is the whole payoff of making error_set canonical. */
template <class LEFT, class RIGHT>
struct error_set_join;

template <class... LEFT, class... RIGHT>
struct error_set_join<error_set_of<LEFT...>, error_set_of<RIGHT...>> {
    using type = error_set<LEFT..., RIGHT...>;
};

template <class LEFT, class RIGHT>
using error_set_join_t = typename error_set_join<LEFT, RIGHT>::type;

/** Bottom: the identity of join. */
using error_set_bottom = error_set<>;

/** Order: LEFT ⊆ RIGHT, which by order-from-join is join(LEFT,RIGHT) == RIGHT.
 */
template <class LEFT, class RIGHT>
inline constexpr bool error_set_subsumes_v = false;

template <class... LEFT, class... RIGHT>
inline constexpr bool
    error_set_subsumes_v<error_set_of<LEFT...>, error_set_of<RIGHT...>> =
        (detail::list_contains_v<LEFT, detail::type_list<RIGHT...>> && ...);

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_ERROR_SET_HPP
