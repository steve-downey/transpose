// include/beman/transpose/error_set.hpp                              -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_ERROR_SET_HPP
#define BEMAN_TRANSPOSE_ERROR_SET_HPP

// error_set: the set of error types a computation may raise.
//
// NOMINAL, not a structural sum. `error_set_of<Es...>` is its own type. The
// public surface is deliberately impoverished -- injection, subsumption,
// membership, visitation -- because "the set of errors this computation may
// raise" is an interpretation, not a representation, and interpretations
// need names. See docs/decisions.md#error-set-identity.
//
// VALUE LEVEL: a WITNESSED SUBSET, not a one-of union. The TYPE says "may
// raise {A,B}"; a VALUE says "did raise", and did-raise is a non-empty subset
// of may-raise: one witness per raised type, at most. Storage is per-type
// slots (a tuple of `std::optional<Es>...`) bounded by |grade|, never
// allocation, never a discriminated one-of. This is the amendment recorded
// at docs/decisions.md#error-set-identity, made by
// docs/decisions.md#accumulation-evidence: the short-circuit applicative
// object only ever produces a singleton witness (exactly the old "one
// alternative" behavior, preserved as a special case); the accumulating
// object (docs/transpose-grading-plan.md#accumulating-object) combines
// witnesses from multiple failures, left-biased per type via
// `error_set_combine` -- two witnesses of the SAME type keep the leftmost,
// which is deterministic only because left-to-right traversal is already
// normative (docs/decisions.md#applicative-objects).
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
#include <beman/transpose/grade.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <optional>
#include <string_view>
#include <tuple>
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

/** A precondition violation the type system cannot catch.
 *
 * Deliberately NOT constexpr. During constant evaluation, calling a
 * non-constexpr function makes the evaluation non-constant, so a violated
 * precondition becomes a COMPILE ERROR naming this function -- which is where
 * most of this library's code runs, and where the contract is therefore not
 * merely documented but enforced. At runtime it stops the program instead of
 * letting a silently-wrong answer escape.
 *
 * This library has no assertion vocabulary and is not acquiring one. The
 * point is narrower: a narrow contract whose violation returns a plausible
 * wrong answer is worse than one that stops, because the plausible answer is
 * the one that reaches a user.
 */
[[noreturn]] inline void error_set_precondition_violated(const char *what) {
    std::fputs(what, stderr);
    std::fputs("\n", stderr);
    std::abort();
}

// -- Per-type witness storage ----------------------------------------------
// One std::optional slot per alternative, never a discriminated one-of. See
// docs/decisions.md#accumulation-evidence for why: the grade join needs no
// monoid, but accumulating VALUE-level evidence does, and the semigroup it
// needs is "per type, keep one witness" -- which a tuple of optionals gives
// for free, with no allocation and no extra discriminant to keep in sync.

/** The slot for ERROR, set from `error` when it is the injected alternative,
 * empty otherwise. Called once per member of the target pack, so exactly one
 * instantiation actually forwards-constructs from `error`; the others never
 * touch it.
 */
template <class ERROR, class INJECTED>
constexpr auto witness_slot(INJECTED &&error) -> std::optional<ERROR> {
    if constexpr (std::is_same_v<ERROR, remove_cvref_t<INJECTED>>) {
        return std::optional<ERROR>(std::forward<INJECTED>(error));
    } else {
        return std::optional<ERROR>{};
    }
}

/** The slot for ERROR when widening from a narrower witnessed subset: the
 * narrower value's own witness if it names ERROR among its alternatives,
 * empty otherwise. This is the value-level half of the ⊆ conversion --
 * copying over whichever witnesses the narrower value actually carries. */
template <class ERROR, class... NARROWER>
constexpr auto widen_slot(const error_set_of<NARROWER...> &narrower)
    -> std::optional<ERROR> {
    if constexpr (error_set_of<NARROWER...>::template contains<ERROR>()) {
        return narrower.template witness<ERROR>();
    } else {
        return std::optional<ERROR>{};
    }
}

} // namespace detail

/** The set of error types a computation may raise, at a canonical pack.
 *
 * Write `error_set<...>` rather than naming this template directly: the alias
 * canonicalizes, this template only accepts an already-canonical pack.
 *
 * VALUE-LEVEL INVARIANT: a non-empty witnessed subset of {ERRORS...} -- for
 * each raised type, at most one witness. See
 * docs/decisions.md#error-set-identity (amended) and
 * docs/decisions.md#accumulation-evidence.
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
    /** Injection: an error value becomes the set it belongs to, witnessing
     * exactly that one alternative. */
    template <class ERROR>
        requires detail::list_contains_v<remove_cvref_t<ERROR>,
                                         detail::type_list<ERRORS...>>
    constexpr error_set_of(ERROR &&error) // NOLINT(*-explicit-constructor)
        : d_witnesses(
              detail::witness_slot<ERRORS>(std::forward<ERROR>(error))...) {}

    /** Subsumption: widening along ⊆, and nothing else. A narrower set is
     * usable wherever a wider one is; there is no conversion the other way,
     * and none between incomparable sets. Conversion FROM the empty set is
     * absent on purpose rather than by oversight: ∅ is uninhabited, so no
     * value ever needs it.
     *
     * Every witness the narrower value carries is preserved; slots for
     * alternatives the narrower type cannot name stay empty.
     */
    template <class... NARROWER>
        requires(sizeof...(NARROWER) > 0) &&
                (!std::is_same_v<error_set_of<NARROWER...>, error_set_of>) &&
                (detail::list_contains_v<NARROWER,
                                         detail::type_list<ERRORS...>> &&
                 ...)
    constexpr error_set_of(
        const error_set_of<NARROWER...> &narrower) // NOLINT(*-explicit-*)
        : d_witnesses(detail::widen_slot<ERRORS>(narrower)...) {}

    /** Membership, for `recover`: is ERROR one of the alternatives? Type
     * level -- "may raise", not "did raise". */
    template <class ERROR>
    static constexpr auto contains() noexcept -> bool {
        return detail::list_contains_v<ERROR, detail::type_list<ERRORS...>>;
    }

    /** Whether this value currently witnesses ERROR -- "did raise". */
    template <class ERROR>
        requires(contains<ERROR>())
    constexpr auto holds() const noexcept -> bool {
        return std::get<std::optional<ERROR>>(d_witnesses).has_value();
    }

    /** The witness for ERROR, if this value raised it. */
    template <class ERROR>
        requires(contains<ERROR>())
    constexpr auto witness() const -> const std::optional<ERROR> & {
        return std::get<std::optional<ERROR>>(d_witnesses);
    }

    /** How many alternatives this value witnesses -- "how many did raise".
     *
     * Always at least one, by the class invariant. The short-circuit
     * applicative object produces exactly one; the accumulating object may
     * produce more. Public so that a caller can CHECK `visit`'s precondition
     * rather than trip it: an unenforceable contract that callers cannot
     * inspect is not a contract, it is a trap.
     */
    constexpr auto witness_count() const noexcept -> std::size_t {
        return (static_cast<std::size_t>(
                    std::get<std::optional<ERRORS>>(d_witnesses).has_value()) +
                ... + std::size_t{0});
    }

    /** Visitation, for `recover`: apply `handler` to the single held witness.
     *
     * PRECONDITION: exactly one witness is present, and it is CHECKED. Every
     * error_set produced by the short-circuit (monad-derived) applicative
     * object satisfies it -- "short-circuit only ever produces singletons"
     * (docs/decisions.md#applicative-objects). A witnessed subset with more
     * than one member, which only the accumulating object produces, has no
     * single value for `visit` to hand back.
     *
     * Violating it is a compile error in a constant expression and stops the
     * program at runtime. It is NOT a silent pick of the leftmost witness:
     * the two features that meet here -- multi-witness values and visitation
     * -- shipped one stage apart, and a quiet wrong answer at that seam would
     * surface as a dropped error much later, somewhere else.
     *
     * For a multi-witness subset use `witness<ERROR>()` per alternative,
     * guarded by `holds<ERROR>()` or `witness_count()`.
     */
    template <class HANDLER>
    constexpr auto visit(HANDLER &&handler) const -> decltype(auto) {
        return std::visit(std::forward<HANDLER>(handler), to_variant());
    }

    /** Equality: same present set, and equal witnesses. Not a step toward
     * being a variant competitor -- it is what keeps a graded carrier as
     * usable as an ungraded one. `expected<T,E>` compares equal whenever E
     * does, and `expected<T, error_set<...>>` would silently lose that if
     * this were absent, which would make grading a usability regression at
     * exactly the point it claims to be additive. Defaulted, so it is
     * deleted rather than ill-formed when an alternative is not comparable.
     * The tuple-of-optionals representation makes this fall out of
     * `std::optional`'s own equality: both empty compares equal, one empty
     * compares unequal, both present compares the values. See
     * docs/decisions.md#error-set-identity.
     */
    friend auto operator==(const error_set_of &, const error_set_of &)
        -> bool = default;

    /** Combines with `other` at the SAME grade, left-biased per type: where
     * both sides witness the same error type, the LEFT (`*this`) witness is
     * kept. This is the accumulating applicative object's evidence-combining
     * step; see `error_set_combine` and
     * docs/decisions.md#accumulation-evidence.
     */
    constexpr auto combined_with(const error_set_of &other) const
        -> error_set_of {
        return error_set_of(
            combine_witnesses(other, std::index_sequence_for<ERRORS...>{}));
    }

  private:
    explicit constexpr error_set_of(
        std::tuple<std::optional<ERRORS>...> witnesses)
        : d_witnesses(std::move(witnesses)) {}

    template <std::size_t... IDX>
    constexpr auto combine_witnesses(const error_set_of &other,
                                     std::index_sequence<IDX...>) const
        -> std::tuple<std::optional<ERRORS>...> {
        return std::tuple<std::optional<ERRORS>...>{
            (std::get<IDX>(d_witnesses).has_value()
                 ? std::get<IDX>(d_witnesses)
                 : std::get<IDX>(other.d_witnesses))...};
    }

    /** The single held witness, reinterpreted as a std::variant, for `visit`.
     * Checks the precondition rather than assuming it: see `visit`. */
    constexpr auto to_variant() const -> std::variant<ERRORS...> {
        if (witness_count() != 1) {
            detail::error_set_precondition_violated(
                "beman::transpose::error_set::visit requires exactly one "
                "witness, but this value witnesses a different number. Use "
                "witness<E>() per alternative for a multi-witness subset; "
                "see docs/decisions.md#accumulation-evidence.");
        }

        std::optional<std::variant<ERRORS...>> found;
        auto try_slot = [&found](const auto &slot) {
            if (!found.has_value() && slot.has_value()) {
                found = std::variant<ERRORS...>{*slot};
            }
        };
        std::apply(
            [&try_slot](const auto &...slots) { (try_slot(slots), ...); },
            d_witnesses);
        return *found;
    }

    std::tuple<std::optional<ERRORS>...> d_witnesses;
};

/** True when T is a witnessed subset -- an error_set_of<...> specialization,
 * whatever its pack. Used to tell apart "this declared error is itself a
 * grade carrier" from "this declared error is a single bare type", which is
 * exactly the fork `combine_errors` needs. */
template <class T>
inline constexpr bool is_error_set_of_v = false;

template <class... ERRORS>
inline constexpr bool is_error_set_of_v<error_set_of<ERRORS...>> = true;

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

// -- Evidence combination, in error vocabulary -----------------------------
// VALUE level, not grade level: the grade join above is type-level union and
// needs no semigroup at all. What the accumulating applicative object needs
// is a semigroup on the witnessed evidence, which is what this section
// supplies. See docs/decisions.md#accumulation-evidence.

/** Combines two witnessed subsets at the SAME grade, left-biased per type:
 * see `error_set_of::combined_with`. Free-function spelling to match
 * `error_set_join` / `error_set_bottom` / `error_set_subsumes_v` above. */
template <class... ERRORS>
constexpr auto error_set_combine(const error_set_of<ERRORS...> &lhs,
                                 const error_set_of<ERRORS...> &rhs)
    -> error_set_of<ERRORS...> {
    return lhs.combined_with(rhs);
}

/** Combines two same-typed declared errors, left-biased. When ERROR is a
 * witnessed subset this unions witnesses via `error_set_combine`; when it is
 * a bare type there is only one slot to begin with, so this degrades to
 * "keep the left" -- the same leftmost-wins behavior the short-circuit
 * object already has, which is why an accumulating traverse over an unmixed
 * pipeline is observationally identical to a short-circuit one. This is the
 * uniform step the accumulating applicative object folds with, regardless of
 * whether the declared error happens to be a set.
 */
template <class ERROR>
constexpr auto combine_errors(const ERROR &lhs, const ERROR & /*rhs*/) -> ERROR
    requires(!is_error_set_of_v<ERROR>)
{
    return lhs;
}

template <class... ERRORS>
constexpr auto combine_errors(const error_set_of<ERRORS...> &lhs,
                              const error_set_of<ERRORS...> &rhs)
    -> error_set_of<ERRORS...> {
    return error_set_combine(lhs, rhs);
}

// -- Registration as a model of the grade algebra -------------------------
// The direction of dependency is the point: the model knows about the
// framework, never the reverse. Everything above this line is error
// vocabulary; everything below translates it into grade vocabulary, and is
// the only place the two meet. A second algebra registers the same way and
// needs nothing from here -- which is the claim the law harness exists to
// test (docs/decisions.md#grade-generality).

namespace detail {

template <class T>
inline constexpr bool is_expected_v = false;

template <class VALUE, class ERROR>
inline constexpr bool is_expected_v<std::expected<VALUE, ERROR>> = true;

} // namespace detail

template <class... LEFT, class... RIGHT>
struct grade_join<error_set_of<LEFT...>, error_set_of<RIGHT...>> {
    using type =
        error_set_join_t<error_set_of<LEFT...>, error_set_of<RIGHT...>>;
};

template <class... ERRORS>
struct grade_bottom<error_set_of<ERRORS...>> {
    using type = error_set_bottom;
};

template <class... LEFT, class... RIGHT>
struct grade_subsumes<error_set_of<LEFT...>, error_set_of<RIGHT...>>
    : std::bool_constant<
          error_set_subsumes_v<error_set_of<LEFT...>, error_set_of<RIGHT...>>> {
};

/** Structural grade detection: an expected whose error alternative IS an
 * error_set is graded at that set.
 *
 * Nominality is what makes this sound -- it conscripts only the willing. An
 * `expected<T, std::variant<A,B>>` is a value-level sum error and stays
 * ∅-graded, because the user did not say otherwise at the declaration site.
 */
template <class VALUE, class... ERRORS>
struct grade_of<std::expected<VALUE, error_set_of<ERRORS...>>> {
    using type = error_set_of<ERRORS...>;
};

/** Re-indexing an expected at a non-empty error set replaces its error
 * alternative. */
template <class VALUE, class ERROR, class HEAD, class... TAIL>
struct rebind_grade<std::expected<VALUE, ERROR>, error_set_of<HEAD, TAIL...>> {
    using type = std::expected<VALUE, error_set_of<HEAD, TAIL...>>;
};

/** Re-indexing a BARE value at a non-empty error set is the promotion into a
 * carrier: the ∅ fiber is the ungraded type itself, so this is where a
 * pipeline first acquires an error alternative. */
template <class VALUE, class HEAD, class... TAIL>
    requires(!detail::is_expected_v<VALUE>)
struct rebind_grade<VALUE, error_set_of<HEAD, TAIL...>> {
    using type = std::expected<VALUE, error_set_of<HEAD, TAIL...>>;
};

/** Re-indexing at the EMPTY error set yields the bare value, never
 * `expected<T, error_set<>>`.
 *
 * This is the mechanized form of the sentinel in
 * docs/decisions.md#empty-grade-spelling: the uniform degenerate-expected
 * form stays available as an explicit spelling, and no framework path
 * materializes it on a user's behalf.
 */
template <class VALUE>
    requires(!detail::is_expected_v<VALUE>)
struct rebind_grade<VALUE, error_set_of<>> {
    using type = VALUE;
};

template <class VALUE, class ERROR>
struct rebind_grade<std::expected<VALUE, ERROR>, error_set_of<>> {
    using type = VALUE;
};

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_ERROR_SET_HPP
