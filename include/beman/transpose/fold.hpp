// include/beman/transpose/fold.hpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_FOLD_HPP
#define BEMAN_TRANSPOSE_FOLD_HPP

#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/monoid.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

// EVIDENCE, NOT PROPOSED WORDING. D3200R0 proposes Applicative and
// Traversable; it does not propose Foldable. The fold family (fold_map and
// its derived operations, over structures that are not ranges) is proposed
// by the recursive-tree-algorithms companion paper (Paper D), whose
// motivation it belongs to -- std::ranges::fold_left already covers flat
// sequences. This header ships as proof that the typeclass-object mechanism
// scales to the fold family; treat its surface as implementation evidence.

namespace beman::transpose::detail {

// The two fold programs fold_left and fold_right derive through are
// endomorphisms `STATE -> STATE`, type-erased through std::function so that
// combine returns what it takes: a monoid's value type must close under
// combine, and a bare lambda type does not. One value type, two instances
// -- composition in traversal order for fold_left, in reverse for
// fold_right -- which is docs/decisions.md#monoid-selection in miniature:
// the choice between them is made at the operation, not by wrapping the
// function in a differently named carrier.

/** Composition monoid on `std::function<STATE(STATE)>` in application
 * order: `combine(l, r)` runs `l` first, then `r`. Identity is the identity
 * function.
 */
template <class STATE>
struct left_compose_monoid {
    using value_type = std::function<STATE(STATE)>;

    auto identity() const -> value_type {
        return [](STATE s) { return s; };
    }

    auto combine(const value_type &lhs, const value_type &rhs) const
        -> value_type {
        return [lhs, rhs](STATE s) { return rhs(lhs(std::move(s))); };
    }
};

/** Composition monoid on `std::function<STATE(STATE)>` in reverse
 * application order: `combine(l, r)` runs `r` first, then `l`.
 */
template <class STATE>
struct right_compose_monoid {
    using value_type = std::function<STATE(STATE)>;

    auto identity() const -> value_type {
        return [](STATE s) { return s; };
    }

    auto combine(const value_type &lhs, const value_type &rhs) const
        -> value_type {
        return [lhs, rhs](STATE s) { return lhs(rhs(std::move(s))); };
    }
};

/** The result type of applying `F` to one element of an Impl that declares
 * `element_type`: the value type a two-argument `fold_map(f, value)` folds
 * under, and therefore the type whose registered Monoid it defaults to.
 * Ill-formed when `IMPL` declares no `element_type`, so that a
 * requires-expression naming it answers `false`.
 */
template <class IMPL, class F>
using fold_result_t = std::remove_cvref_t<
    std::invoke_result_t<F &, const typename IMPL::element_type &>>;

// Named witnesses for the fold family's availability probes, completing
// probe_witness/probe_witness2 with the argument-dependent return shapes
// the derived operations need. A lambda inside a requires-clause mints a
// distinct closure type at every constraint check; MSVC's backend ICEs
// (fatal error C1001, p2) in exactly the translation units that evaluate
// the fold family's clauses, and hoisting the probes into named callables
// is both the workaround and consistent with the concepts, which already
// probe with named witnesses.
//
// Each of the three is constrained on `copy_constructible`. Unlike
// probe_witness, whose body establishes a result type and nothing else,
// these three stand in for derivations that really do copy the element --
// `combine_all` folds with the identity function, `to_vector` collects each
// element into a vector, `find_first` carries a matching element out in an
// optional. That requirement has to be stated rather than discovered in a
// body: probing an operation with a deduced return type instantiates enough
// of it to instantiate the witness call, so an unstated copy would reach
// the compiler as a diagnostic from inside the probe, and a concept that
// diagnoses cannot be used to detect anything. Stated, the probe answers
// `false` for an element type that cannot be copied -- which is the true
// answer, since the derivation could not have run.

/** Identity-shaped probe: returns its argument by value, so the probed
 * `fold_map` sees a callable whose result type is the element type itself
 * -- the shape `combine_all` folds with.
 */
struct identity_probe_witness {
    template <class ARGUMENT>
        requires std::copy_constructible<ARGUMENT>
    constexpr auto operator()(const ARGUMENT &argument) const -> ARGUMENT {
        return argument;
    }
};

/** Collecting probe: returns a one-element vector of its argument -- the
 * shape `to_vector` folds with.
 */
struct vector_probe_witness {
    template <class ARGUMENT>
        requires std::copy_constructible<ARGUMENT>
    constexpr auto operator()(const ARGUMENT &argument) const
        -> std::vector<ARGUMENT> {
        return std::vector<ARGUMENT>{argument};
    }
};

/** Optional-shaped probe: returns an empty optional of its argument's type
 * -- the shape `find_first` folds with.
 */
struct optional_probe_witness {
    template <class ARGUMENT>
        requires std::copy_constructible<ARGUMENT>
    constexpr auto operator()(const ARGUMENT &) const
        -> std::optional<ARGUMENT> {
        return std::optional<ARGUMENT>{};
    }
};

} // namespace beman::transpose::detail

namespace beman::transpose {

// Foldable pattern invariants:
// - Generic entry point is fold_map(F, T, MONOID) via foldable<T>.
//   The monoid is an instance object over the result type of F, passed by
//   value (docs/decisions.md#monoid-selection); the two-argument
//   fold_map(F, T) is derived, defaulting to the registered Monoid of the
//   result type, and exists only where the Impl declares `element_type`
//   and that type has a registration.
// - Instances provide an object with fold_map(F, T, MONOID) and specialize
//   foldable<Concrete>. The instance never chooses a monoid.
// - Derived APIs (length, fold_left, fold_right, combine_all, any_of, all_of,
//   empty, to_vector, find_first) live on the same looked-up object and pass
//   the library's own instances over bare types.
// - Traversal order is instance-defined but must be coherent per instance.

/** Restricted `Impl` concept for Foldable: satisfied when `IMPL` supplies
 * one of the two minimal complete bases the `Foldable` CRTP base admits --
 * `fold_map(f, structure, monoid)` alone, or `fold_right` together with a
 * declared `element_type`. This is the `MINIMAL` pragma to
 * `foldable_object`'s class declaration: `length`, `fold_left`,
 * `combine_all`, `fold`, `any_of`, `all_of`, `empty`, `to_vector` and
 * `find_first` are all derived and belong to `foldable_object` alone.
 */
template <class IMPL, class STRUCTURE>
concept foldable_impl = requires(const IMPL &impl, const STRUCTURE &structure) {
    impl.fold_map(detail::probe_witness<std::size_t>{}, structure,
                  sum_monoid<std::size_t>{});
} || requires(const IMPL &impl, const STRUCTURE &structure) {
    typename IMPL::element_type;
    impl.fold_right(structure, int{}, detail::probe_witness2<int>{});
};

/** CRTP base for Foldable instances.
 * `Impl` must provide either `fold_map(f, container, monoid)` or
 * `fold_right` + `element_type`; all other operations are derived from
 * whichever is the primitive. Declaring `element_type` additionally unlocks
 * the derived operations that must name the element type: the
 * two-argument `fold_map`, `combine_all` and `fold` without an instance,
 * `to_vector` and `find_first`.
 */
template <class Impl>
struct derive_foldable : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No foldable<T> specialization found. "
                  "Specialize beman::transpose::foldable<T> for your "
                  "type T and provide fold_map(F, T, MONOID) or "
                  "fold_right(T, STATE, F) + element_type.");
    // Alternate-core: Impl provides either fold_map or fold_right as
    // primitive. Haskell equivalent: {-# MINIMAL foldMap | foldr #-}
    //
    // fold_map and fold_right are a mutually-derivable pair, done the
    // apply.hpp way: each probes Impl for its own native version first, and
    // each derivation addresses Impl directly (impl_of(self), never self)
    // for the other operation. Before this conversion, both derivations were
    // self-routed, and the only thing standing between that and unbounded
    // mutual template recursion -- fold_map deriving from fold_right
    // deriving from fold_map, each round nesting another fold program --
    // was a Map's using-declaration happening to shadow one side. A Map that
    // omitted it fell into the cycle. Addressing Impl directly closes that
    // structurally: neither derivation can re-enter the other through self,
    // because self is never named. The Maps carry no using-declaration for
    // fold_map at all; the base decides, per instantiation, from whichever
    // basis Impl actually provides.

    // fold_map with an explicit monoid instance: the basis. Native
    // Impl::fold_map preferred; derived from fold_right otherwise.
    // foldMap f = foldr (\x acc -> f x <> acc) mempty
    template <class F, class T, class MONOID>
    auto fold_map(this auto &&self, F &&function, T &&value, MONOID monoid)
        requires requires(const Impl &impl) {
            impl.fold_map(std::forward<F>(function), std::forward<T>(value),
                          monoid);
        } || requires(const Impl &impl) {
            typename Impl::element_type;
            impl.fold_right(std::forward<T>(value), monoid.identity(),
                            detail::probe_witness2<monoid_value_t<MONOID>>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).fold_map(std::forward<F>(function),
                                                 std::forward<T>(value),
                                                 monoid);
                      }) {
            return impl_of(self).fold_map(std::forward<F>(function),
                                          std::forward<T>(value), monoid);
        } else {
            using Result = monoid_value_t<MONOID>;
            // The declaration's second alternative probes with a named
            // witness, never a lambda: a capturing lambda in a
            // requires-clause is rejected by Clang's front end, and the
            // closure types lambdas mint per constraint check are what ICE
            // MSVC's backend (see the witnesses' note in detail). This
            // static_assert is provable redundant given the declaration's
            // constraint already selected this branch -- it is the
            // GHC-MINIMAL-style last-resort message, not load-bearing SFINAE
            // -- so the concept-based condition is exactly as informative.
            static_assert(foldable_impl<Impl, std::remove_cvref_t<T>>,
                          "Foldable Impl must provide at least one basis: "
                          "fold_map(f, container, monoid), or "
                          "fold_right(container, state, f) plus element_type.");
            return impl_of(self).fold_right(
                std::forward<T>(value), monoid.identity(),
                [&function, &monoid](const auto &elem, Result acc) {
                    return monoid.combine(std::invoke(function, elem),
                                          std::move(acc));
                });
        }
    }

    // fold_map without an instance: defaults to the registered Monoid of
    // the result type. Available only where Impl declares element_type (so
    // the result type can be named) and that type has a registration. A
    // fold over a type with several monoids -- int, bool -- is therefore
    // not a candidate here; the caller passes the instance. An Impl may
    // still supply a native two-argument fold_map of its own.
    template <class F, class T>
    auto fold_map(this auto &&self, F &&function, T &&value)
        requires requires(const Impl &impl) {
            impl.fold_map(std::forward<F>(function), std::forward<T>(value));
        } || requires {
            typename Impl::element_type;
            requires detail::has_registered_monoid<
                detail::fold_result_t<Impl, F>>;
            self.fold_map(std::forward<F>(function), std::forward<T>(value),
                          Monoid<detail::fold_result_t<Impl, F>>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).fold_map(std::forward<F>(function),
                                                 std::forward<T>(value));
                      }) {
            return impl_of(self).fold_map(std::forward<F>(function),
                                          std::forward<T>(value));
        } else {
            return self.fold_map(std::forward<F>(function),
                                 std::forward<T>(value),
                                 Monoid<detail::fold_result_t<Impl, F>>{});
        }
    }

    /** Returns the number of elements in the foldable container. */
    template <class T>
    auto length(this auto &&self, T &&value) -> std::size_t
        requires requires(const Impl &impl) {
            impl.length(std::forward<T>(value));
        } || requires {
            // self, not impl: fold_map is itself now a probing member with
            // its own either-basis constraint, so checking availability
            // through self is what correctly admits a fold_right +
            // element_type Impl. Checking impl.fold_map directly would
            // reject that Impl, since it has no fold_map member at all.
            self.fold_map(detail::probe_witness<std::size_t>{},
                          std::forward<T>(value), sum_monoid<std::size_t>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).length(std::forward<T>(value));
                      }) {
            return impl_of(self).length(std::forward<T>(value));
        } else {
            return self.fold_map([](const auto &) { return std::size_t{1}; },
                                 std::forward<T>(value),
                                 sum_monoid<std::size_t>{});
        }
    }

    /** Left-associative fold: applies `function(state, element)` for each
     * element in traversal order, starting from `initial_state`.
     */
    template <class T, class STATE, class F>
    auto fold_left(this auto &&self, T &&value, STATE initial_state,
                   F &&function)
        requires requires(const Impl &impl) {
            impl.fold_left(std::forward<T>(value), initial_state,
                           std::forward<F>(function));
        } || requires {
            // self, not impl -- see length's second alternative. A named
            // witness, not a lambda: fold_map is generic in its callable,
            // so only the return-type shape (the erased fold program)
            // matters for the probe.
            self.fold_map(
                detail::probe_witness<std::function<std::remove_cvref_t<STATE>(
                    std::remove_cvref_t<STATE>)>>{},
                std::forward<T>(value),
                detail::left_compose_monoid<std::remove_cvref_t<STATE>>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).fold_left(std::forward<T>(value),
                                                  initial_state,
                                                  std::forward<F>(function));
                      }) {
            return impl_of(self).fold_left(std::forward<T>(value),
                                           std::move(initial_state),
                                           std::forward<F>(function));
        } else {
            using StateType = std::remove_cvref_t<STATE>;
            using Program = std::function<StateType(StateType)>;
            auto step = std::forward<F>(function);

            const auto program = self.fold_map(
                [&step](const auto &x) -> Program {
                    using ValueType = std::remove_cvref_t<decltype(x)>;
                    return [x_copy = ValueType(x), &step](StateType s) {
                        return std::invoke(step, std::move(s), x_copy);
                    };
                },
                std::forward<T>(value),
                detail::left_compose_monoid<StateType>{});

            return program(StateType(std::move(initial_state)));
        }
    }

    /** Right-associative fold: applies `function(element, state)` for each
     * element in reverse traversal order, starting from `initial_state`.
     */
    template <class T, class STATE, class F>
    auto fold_right(this auto &&self, T &&value, STATE initial_state,
                    F &&function)
        requires requires(const Impl &impl) {
            impl.fold_right(std::forward<T>(value), initial_state,
                            std::forward<F>(function));
        } || requires(const Impl &impl) {
            impl.fold_map(
                detail::probe_witness<std::function<std::remove_cvref_t<STATE>(
                    std::remove_cvref_t<STATE>)>>{},
                std::forward<T>(value),
                detail::right_compose_monoid<std::remove_cvref_t<STATE>>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).fold_right(std::forward<T>(value),
                                                   initial_state,
                                                   std::forward<F>(function));
                      }) {
            return impl_of(self).fold_right(std::forward<T>(value),
                                            std::move(initial_state),
                                            std::forward<F>(function));
        } else {
            using StateType = std::remove_cvref_t<STATE>;
            using Program = std::function<StateType(StateType)>;
            auto step = std::forward<F>(function);

            // See fold_map's static_assert for why the concept-based
            // condition is provably redundant here rather than load-bearing.
            static_assert(foldable_impl<Impl, std::remove_cvref_t<T>>,
                          "Foldable Impl must provide at least one basis: "
                          "fold_map(f, container, monoid), or "
                          "fold_right(container, state, f) plus element_type.");
            const auto program = impl_of(self).fold_map(
                [&step](const auto &x) -> Program {
                    using ValueType = std::remove_cvref_t<decltype(x)>;
                    return [x_copy = ValueType(x), &step](StateType s) {
                        return std::invoke(step, x_copy, std::move(s));
                    };
                },
                std::forward<T>(value),
                detail::right_compose_monoid<StateType>{});

            return program(StateType(std::move(initial_state)));
        }
    }

    /** Combines all elements under `monoid`, an instance over the element
     * type.
     */
    template <class T, class MONOID>
    auto combine_all(this auto &&self, T &&value, MONOID monoid)
        requires requires(const Impl &impl) {
            impl.combine_all(std::forward<T>(value), monoid);
        } || requires {
            // self, not impl -- see length's second alternative.
            self.fold_map(detail::identity_probe_witness{},
                          std::forward<T>(value), monoid);
        }
    {
        if constexpr (requires {
                          impl_of(self).combine_all(std::forward<T>(value),
                                                    monoid);
                      }) {
            return impl_of(self).combine_all(std::forward<T>(value), monoid);
        } else {
            return self.fold_map([](const auto &x) { return x; },
                                 std::forward<T>(value), monoid);
        }
    }

    /** Combines all elements using the registered Monoid of the element
     * type. Available only where that registration exists; an element type
     * with several monoids takes the two-argument form.
     */
    template <class T>
    auto combine_all(this auto &&self, T &&value)
        requires requires(const Impl &impl) {
            impl.combine_all(std::forward<T>(value));
        } || requires {
            // self, not impl -- see length's second alternative. The
            // two-argument fold_map carries the element_type and
            // registration requirements.
            self.fold_map(detail::identity_probe_witness{},
                          std::forward<T>(value));
        }
    {
        if constexpr (requires {
                          impl_of(self).combine_all(std::forward<T>(value));
                      }) {
            return impl_of(self).combine_all(std::forward<T>(value));
        } else {
            return self.fold_map([](const auto &x) { return x; },
                                 std::forward<T>(value));
        }
    }

    /** Alias for `combine_all`. Its second alternative names what it
     * actually calls -- self.combine_all, which is itself a probing member
     * -- rather than fold_map. Naming fold_map here would reject an Impl
     * that provides a native combine_all directly but no fold_map at all.
     */
    template <class T, class MONOID>
    auto fold(this auto &&self, T &&value, MONOID monoid)
        requires requires(const Impl &impl) {
            impl.fold(std::forward<T>(value), monoid);
        } || requires { self.combine_all(std::forward<T>(value), monoid); }
    {
        if constexpr (requires {
                          impl_of(self).fold(std::forward<T>(value), monoid);
                      }) {
            return impl_of(self).fold(std::forward<T>(value), monoid);
        } else {
            return self.combine_all(std::forward<T>(value), monoid);
        }
    }

    template <class T>
    auto fold(this auto &&self, T &&value)
        requires requires(const Impl &impl) {
            impl.fold(std::forward<T>(value));
        } || requires { self.combine_all(std::forward<T>(value)); }
    {
        if constexpr (requires {
                          impl_of(self).fold(std::forward<T>(value));
                      }) {
            return impl_of(self).fold(std::forward<T>(value));
        } else {
            return self.combine_all(std::forward<T>(value));
        }
    }

    /** Returns `true` if any element satisfies `predicate`. */
    template <class T, class PREDICATE>
    auto any_of(this auto &&self, T &&value, PREDICATE &&predicate) -> bool
        requires requires(const Impl &impl) {
            impl.any_of(std::forward<T>(value),
                        std::forward<PREDICATE>(predicate));
        } || requires {
            // self, not impl -- see length's second alternative. A named
            // witness, not a lambda: only the bool-shaped return matters
            // for the probe; the instance says which bool monoid.
            self.fold_map(detail::probe_witness<bool>{}, std::forward<T>(value),
                          any_monoid{});
        }
    {
        if constexpr (requires {
                          impl_of(self).any_of(
                              std::forward<T>(value),
                              std::forward<PREDICATE>(predicate));
                      }) {
            return impl_of(self).any_of(std::forward<T>(value),
                                        std::forward<PREDICATE>(predicate));
        } else {
            return self.fold_map(
                [&predicate](const auto &x) {
                    return static_cast<bool>(std::invoke(predicate, x));
                },
                std::forward<T>(value), any_monoid{});
        }
    }

    /** Returns `true` if all elements satisfy `predicate`. */
    template <class T, class PREDICATE>
    auto all_of(this auto &&self, T &&value, PREDICATE &&predicate) -> bool
        requires requires(const Impl &impl) {
            impl.all_of(std::forward<T>(value),
                        std::forward<PREDICATE>(predicate));
        } || requires {
            // self, not impl; see any_of's second alternative.
            self.fold_map(detail::probe_witness<bool>{}, std::forward<T>(value),
                          all_monoid{});
        }
    {
        if constexpr (requires {
                          impl_of(self).all_of(
                              std::forward<T>(value),
                              std::forward<PREDICATE>(predicate));
                      }) {
            return impl_of(self).all_of(std::forward<T>(value),
                                        std::forward<PREDICATE>(predicate));
        } else {
            return self.fold_map(
                [&predicate](const auto &x) {
                    return static_cast<bool>(std::invoke(predicate, x));
                },
                std::forward<T>(value), all_monoid{});
        }
    }

    /** Returns `true` if the container holds no elements. Its second
     * alternative names self.any_of for the same reason `fold`'s names
     * self.combine_all: an Impl may provide a native any_of directly, with
     * no fold_map at all.
     */
    template <class T>
    auto empty(this auto &&self, T &&value) -> bool
        requires requires(const Impl &impl) {
            impl.empty(std::forward<T>(value));
        } || requires {
            self.any_of(std::forward<T>(value), detail::probe_witness<bool>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).empty(std::forward<T>(value));
                      }) {
            return impl_of(self).empty(std::forward<T>(value));
        } else {
            return !self.any_of(std::forward<T>(value),
                                [](const auto &) { return true; });
        }
    }

    /** Collects all elements into a `std::vector` in traversal order. The
     * derivation folds under the registered vector monoid, so it needs the
     * element type: it is available where Impl declares `element_type`.
     */
    template <class T>
    auto to_vector(this auto &&self, T &&value)
        requires requires(const Impl &impl) {
            impl.to_vector(std::forward<T>(value));
        } || requires {
            // self, not impl -- see length's second alternative.
            typename Impl::element_type;
            self.fold_map(detail::vector_probe_witness{},
                          std::forward<T>(value),
                          Monoid<std::vector<typename Impl::element_type>>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).to_vector(std::forward<T>(value));
                      }) {
            return impl_of(self).to_vector(std::forward<T>(value));
        } else {
            using Element = typename Impl::element_type;
            return self.fold_map(
                [](const auto &x) { return std::vector<Element>{x}; },
                std::forward<T>(value), Monoid<std::vector<Element>>{});
        }
    }

    /** Returns the first element satisfying `predicate`, or an empty
     * optional. The derivation folds under `first_monoid` over the element
     * type, so like `to_vector` it needs Impl to declare `element_type`.
     */
    template <class T, class PREDICATE>
    auto find_first(this auto &&self, T &&value, PREDICATE &&predicate)
        requires requires(const Impl &impl) {
            impl.find_first(std::forward<T>(value),
                            std::forward<PREDICATE>(predicate));
        } || requires {
            // self, not impl; see any_of's second alternative.
            typename Impl::element_type;
            self.fold_map(detail::optional_probe_witness{},
                          std::forward<T>(value),
                          first_monoid<typename Impl::element_type>{});
        }
    {
        if constexpr (requires {
                          impl_of(self).find_first(
                              std::forward<T>(value),
                              std::forward<PREDICATE>(predicate));
                      }) {
            return impl_of(self).find_first(std::forward<T>(value),
                                            std::forward<PREDICATE>(predicate));
        } else {
            using Element = typename Impl::element_type;
            return self.fold_map(
                [&predicate](const auto &x) -> std::optional<Element> {
                    if (std::invoke(predicate, x)) {
                        return std::optional<Element>{x};
                    }
                    return std::optional<Element>{};
                },
                std::forward<T>(value), first_monoid<Element>{});
        }
    }

  private:
    //! \omit
    template <class SELF>
    static constexpr decltype(auto) impl_of(SELF &&self) {
        return static_cast<impl_ref_t<Impl, SELF>>(self);
    }
};

/** Typeclass lookup variable for Foldable; specialize for each container type.
 */
template <class T>
inline constexpr auto foldable = std::false_type{};

// -- Operation objects --
//
// See functor.hpp for the shape, the motivation, and why every typeclass
// operation shares one namespace. Foldable is the widest of the typeclasses
// -- one primitive and nine derived operations -- and it supplies most of the
// names with namespace-std counterparts: `empty`, `length`, `to_vector`,
// `any_of` and `all_of`, of which `empty` against `std::empty` is the worst.
//
// `empty` stays the predicate here, which is the C++ reading (std::empty,
// std::ranges::empty, container .empty()) and the reverse of the FP one --
// Haskell and PureScript call this `null`, Cats calls it `isEmpty`, and all
// three reserve `empty` for Alternative's identity. See the note in
// functor.hpp for why the host language wins that one.
//
// Every operation gets one, primitive and derived alike. Which operations are
// primitive is a fact about the instance author's obligations, and asking a
// caller to know it before they can find `length` would be the wrong tax.
//
// `fold` (the alias of combine_all) gets no object: an alias does not need a
// second name.

/** Operation object for Foldable's `fold_map` primitive.
 *
 * The container type keys the lookup and is deduced from the second
 * argument. The monoid instance is the trailing argument, as it is on the
 * instance (docs/decisions.md#monoid-selection); without it the call
 * defaults to the registered Monoid of the result type, and where that type
 * has no registration the two-argument form is not a candidate.
 */
struct fold_map_fn {
    /** Maps each element into `monoid` and combines the images in the
     * instance's documented traversal order.
     *
     * @tparam TC  The Foldable instance, from the lookup for the keying
     * argument. Not for callers to supply; pin with mode 2.
     */
    template <class F, class T, class MONOID,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.fold_map(std::declval<F>(), std::declval<T>(),
                        std::declval<MONOID>());
        }
    constexpr auto operator()(F &&function, T &&value, MONOID monoid) const {
        return TC.fold_map(std::forward<F>(function), std::forward<T>(value),
                           std::move(monoid));
    }

    /** As above, with the registered Monoid of the result type. */
    template <class F, class T,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires { TC.fold_map(std::declval<F>(), std::declval<T>()); }
    constexpr auto operator()(F &&function, T &&value) const {
        return TC.fold_map(std::forward<F>(function), std::forward<T>(value));
    }
};

/** The semantic centre of Foldable: `fold_map(f, container[, monoid])`. */
inline constexpr fold_map_fn fold_map{};

/** Operation object for the derived `fold_left`. */
struct fold_left_fn {
    /** Left-associative fold in the instance's traversal order. */
    template <class T, class STATE, class F,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.fold_left(std::declval<T>(), std::declval<STATE>(),
                         std::declval<F>());
        }
    constexpr auto operator()(T &&value, STATE initial_state,
                              F &&function) const {
        return TC.fold_left(std::forward<T>(value), std::move(initial_state),
                            std::forward<F>(function));
    }
};

/** Left-associative fold: `fold_left(container, init, f)`. */
inline constexpr fold_left_fn fold_left{};

/** Operation object for the derived `fold_right`. */
struct fold_right_fn {
    /** Right-associative fold in reverse traversal order. */
    template <class T, class STATE, class F,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.fold_right(std::declval<T>(), std::declval<STATE>(),
                          std::declval<F>());
        }
    constexpr auto operator()(T &&value, STATE initial_state,
                              F &&function) const {
        return TC.fold_right(std::forward<T>(value), std::move(initial_state),
                             std::forward<F>(function));
    }
};

/** Right-associative fold: `fold_right(container, init, f)`. */
inline constexpr fold_right_fn fold_right{};

/** Operation object for the derived `length`. */
struct length_fn {
    /** Number of elements in the container. */
    template <class T, const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires { TC.length(std::declval<T>()); }
    constexpr auto operator()(T &&value) const -> std::size_t {
        return TC.length(std::forward<T>(value));
    }
};

/** Number of elements: `length(container)`. */
inline constexpr length_fn length{};

/** Operation object for the derived `combine_all`. */
struct combine_all_fn {
    /** Combines all elements under `monoid`. */
    template <class T, class MONOID,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.combine_all(std::declval<T>(), std::declval<MONOID>());
        }
    constexpr auto operator()(T &&value, MONOID monoid) const {
        return TC.combine_all(std::forward<T>(value), std::move(monoid));
    }

    /** Combines all elements under the registered Monoid of the element
     * type; not a candidate where the element type has no registration. */
    template <class T, const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires { TC.combine_all(std::declval<T>()); }
    constexpr auto operator()(T &&value) const {
        return TC.combine_all(std::forward<T>(value));
    }
};

/** Monoidal summary of every element: `combine_all(container[, monoid])`. */
inline constexpr combine_all_fn combine_all{};

/** Operation object for the derived `any_of`. */
struct any_of_fn {
    /** True when any element satisfies `predicate`. */
    template <class T, class PREDICATE,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.any_of(std::declval<T>(), std::declval<PREDICATE>());
        }
    constexpr auto operator()(T &&value, PREDICATE &&predicate) const -> bool {
        return TC.any_of(std::forward<T>(value),
                         std::forward<PREDICATE>(predicate));
    }
};

/** True when any element satisfies a predicate. */
inline constexpr any_of_fn any_of{};

/** Operation object for the derived `all_of`. */
struct all_of_fn {
    /** True when every element satisfies `predicate`. */
    template <class T, class PREDICATE,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.all_of(std::declval<T>(), std::declval<PREDICATE>());
        }
    constexpr auto operator()(T &&value, PREDICATE &&predicate) const -> bool {
        return TC.all_of(std::forward<T>(value),
                         std::forward<PREDICATE>(predicate));
    }
};

/** True when every element satisfies a predicate. */
inline constexpr all_of_fn all_of{};

/** Operation object for the derived `empty`. See the naming-hazard
 * note above: this name is the one genuine collision risk in the set.
 */
struct empty_fn {
    /** True when the container holds no elements. */
    template <class T, const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires { TC.empty(std::declval<T>()); }
    constexpr auto operator()(T &&value) const -> bool {
        return TC.empty(std::forward<T>(value));
    }
};

/** True when the container holds no elements. */
inline constexpr empty_fn empty{};

/** Operation object for the derived `to_vector`. */
struct to_vector_fn {
    /** Collects every element into a `std::vector` in traversal order. */
    template <class T, const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires { TC.to_vector(std::declval<T>()); }
    constexpr auto operator()(T &&value) const {
        return TC.to_vector(std::forward<T>(value));
    }
};

/** Elements in traversal order: `to_vector(container)`. */
inline constexpr to_vector_fn to_vector{};

/** Operation object for the derived `find_first`. */
struct find_first_fn {
    /** First element satisfying `predicate`, or an empty optional. */
    template <class T, class PREDICATE,
              const auto &TC = foldable<std::remove_cvref_t<T>>>
        requires requires {
            TC.find_first(std::declval<T>(), std::declval<PREDICATE>());
        }
    constexpr auto operator()(T &&value, PREDICATE &&predicate) const {
        return TC.find_first(std::forward<T>(value),
                             std::forward<PREDICATE>(predicate));
    }
};

/** First matching element, or empty: `find_first(container, pred)`. */
inline constexpr find_first_fn find_first{};

/** Deep object concept for a Foldable object over `STRUCTURE`: satisfied
 * when `OBJ` provides the full object surface -- `fold_map` with an
 * instance, `length`, `fold_left`, `fold_right`, `any_of`, `all_of`,
 * `empty`, `to_vector` and `find_first`, each probed with a representative
 * witness callable. `combine_all` and `fold` without an instance are
 * required only where `STRUCTURE`'s element type has a registered `Monoid`
 * -- see `detail::has_registered_monoid` -- since both default to that
 * registration. Foldable is evidence, not proposed wording; this concept
 * carries no wording either.
 */
template <class OBJ, class STRUCTURE>
concept foldable_object =
    requires(const OBJ &obj, const STRUCTURE &structure) {
        obj.fold_map(detail::probe_witness<std::size_t>{}, structure,
                     sum_monoid<std::size_t>{});
        obj.length(structure);
        obj.fold_left(structure, int{}, detail::probe_witness2<int>{});
        obj.fold_right(structure, int{}, detail::probe_witness2<int>{});
        obj.any_of(structure, detail::probe_witness<bool>{});
        obj.all_of(structure, detail::probe_witness<bool>{});
        obj.empty(structure);
        obj.to_vector(structure);
        obj.find_first(structure, detail::probe_witness<bool>{});
    } && (!detail::has_registered_monoid<applicative_value_t<STRUCTURE>> ||
          requires(const OBJ &obj, const STRUCTURE &structure) {
              obj.combine_all(structure);
              obj.fold(structure);
          });

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_FOLD_HPP
