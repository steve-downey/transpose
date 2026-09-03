// include/beman/transpose/traverse.hpp                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef BEMAN_TRANSPOSE_TRAVERSE_HPP
#define BEMAN_TRANSPOSE_TRAVERSE_HPP

#include <beman/transpose/apply.hpp>
#include <beman/transpose/detail/typeclass_base.hpp>
#include <beman/transpose/functor.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace beman::transpose {

// Traversable pattern invariants:
// - Instances are single lookup objects that provide traverse(F, T).
// - transpose is a derived object operation implemented from
//   traverse(identity).
// - Dispatch happens through a provided object or
//   traversable_typeclass<Concrete>.
// - Traversal must preserve container shape while transposing structure and
//   context.

/** CRTP base for Traversable instances.
 * `Impl` must provide `traverse(applicative, f, container)` and declare
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
struct Traversable : protected Impl {
    static_assert(!std::is_same_v<Impl, std::false_type>,
                  "No traversable_typeclass<T> specialization found. "
                  "Specialize beman::transpose::traversable_typeclass<T> for "
                  "your type T, "
                  "provide traverse(applicative, F, T), and declare 'using "
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
        const auto &applicative = applicative_typeclass<Context>;
        return self.traverse(applicative, std::forward<F>(function),
                             std::forward<T>(value));
    }

    /** Transposes a structure of effectful values into one outer effect
     * containing the structure. The element type must itself be an
     * applicative context.
     */
    template <class T>
    auto transpose(this auto &&self, T &&value) {
        using Context = element_type;
        const auto &applicative = applicative_typeclass<Context>;
        return self.traverse(
            applicative, [](auto &&x) { return std::forward<decltype(x)>(x); },
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
        const auto &applicative = applicative_typeclass<Context>;
        return traversable_map.traverse(applicative, std::forward<F>(function),
                                        std::forward<T>(value));
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
inline constexpr auto traversable_typeclass = std::false_type{};

namespace detail {

/** The applicative context `traverse(function, value)` would infer: the
 * return type of `function` applied to one element of `value`'s traversable
 * structure. Factored out so both the POLICY default and its constraint can
 * name it without repeating the computation.
 */
template <class F, class T>
using traverse_context_t = remove_cvref_t<std::invoke_result_t<
    F,
    const typename remove_cvref_t<
        decltype(traversable_typeclass<remove_cvref_t<T>>)>::element_type &>>;

} // namespace detail

/** True when POLICY is usable as the trailing traverse policy for CONTEXT: it
 * must provide `pure`, returning exactly CONTEXT, from CONTEXT's element
 * type -- the minimal signature every registered applicative object has.
 *
 * This is the enforcement hook of docs/decisions.md#traverse-policy-surface:
 * it is what makes a stray third argument (someone's extra container, say)
 * fail loudly instead of being silently accepted as a policy, and it is
 * where the framework would refuse an accumulating object's absent bind at
 * the traverse boundary rather than deeper inside the call.
 */
template <class POLICY, class CONTEXT>
concept applicative_object_for = requires(const POLICY &policy) {
    {
        policy.pure(std::declval<applicative_value_t<CONTEXT>>())
    } -> std::same_as<CONTEXT>;
};

/** @brief Maps `function` over `value`, traverses effects left-to-right,
 *         and preserves container shape.
 *
 * @param function  A callable returning an applicative effect for each element.
 * @param value     The traversable container to process.
 * @param policy    The applicative object selecting the composition
 *                  discipline (docs/decisions.md#traverse-policy-surface).
 *                  Defaults to the monad-derived (short-circuit) object
 *                  looked up for the inferred context, which is exactly
 *                  today's behavior; pass
 *                  `accumulating_applicative_typeclass<Context>` explicitly
 *                  to collect every raised error instead of the first.
 * @return          The container shape transposed into the applicative effect.
 */
template <
    class F, class T,
    class POLICY = remove_cvref_t<
        decltype(applicative_typeclass<detail::traverse_context_t<F, T>>)>>
    requires applicative_object_for<POLICY, detail::traverse_context_t<F, T>>
auto traverse(F &&function, T &&value, POLICY policy = POLICY{}) {
    const auto &map = traversable_typeclass<remove_cvref_t<T>>;
    return map.traverse(policy, std::forward<F>(function),
                        std::forward<T>(value));
}

} // namespace beman::transpose

#endif // BEMAN_TRANSPOSE_TRAVERSE_HPP
