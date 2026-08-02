// include/beman/transpose/traverse.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TRAVERSE_HPP
#define BEMAN_TRANSPOSE_TRAVERSE_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>

#include <type_traits>
#include <utility>

namespace beman::transpose {

// Traversable pattern invariants:
// - Instances are single lookup objects that provide traverse(F, T).
// - transpose is a derived object operation implemented from
//   traverse(identity).
// - Dispatch happens through a provided object or
//   traversable<Concrete>.
// - Traversal must preserve container shape while transposing structure and
//   context.

/** CRTP base for Traversable instances.
 * `Impl` must provide `traverse(applicative_map, f, container)` and declare
 * `element_type`. All other operations (`transpose`, `for_each`,
 * `traverse_with`, `transpose_with`) are derived.
 *
 * DELIBERATE CONSTRAINT: Traversable does not and must not require a
 * Foldable instance (no Haskell-style superclass). `traverse` needs only
 * an Applicative and the walk; the fold family is proposed by the
 * companion recursive-tree-algorithms paper, and requiring it here would
 * put that paper's concept beneath this one after review. foldMapDefault-
 * style derivations (folding via traverse) remain possible as evidence,
 * never a requirement.
 */
template <class Impl>
struct derive_traversable : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No beman::transpose::traversable<T> specialization found. "
                  "Specialize beman::transpose::traversable<T> for "
                  "your type T, "
                  "provide traverse(applicative_map, F, T), and declare 'using "
                  "element_type = T;'.");
    static_assert(
        requires { typename Impl::element_type; },
        "Traversable Impl must declare 'using element_type = T;' "
        "so that transpose() and traverse_with() can deduce the element type.");
    // Alternate-core: Impl::traverse is the primitive; transpose is derived
    // from it. A transpose-primitive Impl would shadow transpose instead.
    using Impl::traverse;
    using element_type = typename Impl::element_type;

    /** Applies `function` to each element and transposes the resulting
     * effects; the applicative is inferred from the return type of `function`.
     */
    template <class T, class F>
    auto for_each(this auto &&self, T &&value, F &&function) {
        using Context =
            remove_cvref_t<std::invoke_result_t<F, const element_type &>>;
        const auto &applicative_map = applicative<Context>;
        return self.traverse(applicative_map, std::forward<F>(function),
                             std::forward<T>(value));
    }

    /** Transposes a structure of effectful values into one outer effect
     * containing the structure. The element type must itself be an
     * applicative context.
     */
    template <class T>
    auto transpose(this auto &&self, T &&value) {
        using Context = element_type;
        const auto &applicative_map = applicative<Context>;
        return self.traverse(
            applicative_map,
            [](auto &&x) { return std::forward<decltype(x)>(x); },
            std::forward<T>(value));
    }

    /** Traverses using a different traversable instance; applicative is
     * inferred from the return type of `function`.
     */
    template <class TRAVERSABLE_MAP, class T, class F>
    auto traverse_with(this auto &&, const TRAVERSABLE_MAP &traversable_map,
                       F &&function, T &&value) {
        using Context = remove_cvref_t<std::invoke_result_t<
            F, const typename remove_cvref_t<TRAVERSABLE_MAP>::element_type &>>;
        const auto &applicative_map = applicative<Context>;
        return traversable_map.traverse(
            applicative_map, std::forward<F>(function), std::forward<T>(value));
    }

    /** Traverses using explicit traversable and applicative instances. */
    template <class TRAVERSABLE_MAP, class APPLICATIVE_MAP, class T, class F>
    auto traverse_with(this auto &&, const TRAVERSABLE_MAP &traversable_map,
                       const APPLICATIVE_MAP &applicative_map, F &&function,
                       T &&value) {
        return traversable_map.traverse(
            applicative_map, std::forward<F>(function), std::forward<T>(value));
    }

    /** Transposes using a different traversable instance; applicative is
     * inferred from the container's element type.
     */
    template <class TRAVERSABLE_MAP, class T>
    auto transpose_with(this auto &&self,
                        const TRAVERSABLE_MAP &traversable_map, T &&value) {
        return self.traverse_with(
            traversable_map,
            [](auto &&x) { return std::forward<decltype(x)>(x); },
            std::forward<T>(value));
    }
};

/** Typeclass lookup variable for Traversable; specialize for each container
 * type. */
template <class T>
inline constexpr auto traversable = std::false_type{};

// -- Operation objects --
//
// See functor.hpp for the shape, the motivation, and why the namespace is
// nested. `traverse` was already a free function template doing exactly this
// lookup-and-call; it becomes an object in `typeclass`, which suppresses ADL
// and makes it non-overloadable. Callers move from `traverse(f, v)` to
// `traverse(f, v)`; whether the proposed verbs should pay that is a
// naming-review question, not one this header can settle.

/** Operation object for Traversable's `traverse`.
 *
 * The structure keys the Traversable lookup and is deduced from the second
 * argument; the applicative context is then inferred from what `function`
 * returns for one element, exactly as the derived `for_each` does.
 */
struct traverse_fn {
    /** @brief Maps `function` over `value`, traverses effects left-to-right,
     *         and preserves container shape.
     *
     * @param function  A callable returning an applicative effect for each
     *                  element.
     * @param value     The traversable container to process.
     * @return          The container shape transposed into the applicative
     *                  effect.
     * @tparam TC  The Traversable instance, from the lookup for the keying
     * argument. Not for callers to supply; pin with mode 2.
     */
    template <class F, class T, const auto &TC = traversable<remove_cvref_t<T>>>
    constexpr auto operator()(F &&function, T &&value) const {
        static_assert(
            has_instance_v<decltype(TC)>,
            "No Traversable instance for this type. Specialize "
            "beman::transpose::traversable<T> with an "
            "object providing traverse(applicative_map, f, container) "
            "and 'using element_type = ...;'.");
        return TC.for_each(std::forward<T>(value), std::forward<F>(function));
    }
};

/** Shape-preserving effectful traversal: `traverse(f, structure)`. */
inline constexpr traverse_fn traverse{};

// `for_each` gets no operation object. The derived operation is `traverse`
// with the arguments the other way round, and `for_each` at namespace scope
// would sit next to std::for_each / std::ranges::for_each with a different
// return contract -- one name, two meanings, is worse than one spelling.

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_TRAVERSE_HPP
