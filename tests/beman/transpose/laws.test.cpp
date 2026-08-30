// tests/beman/transpose/laws.test.cpp                                -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "laws.hpp"

#include <beman/transpose/error_set.hpp>
#include <beman/transpose/grade.hpp>

#include <catch2/catch_test_macros.hpp>

#include <future>
#include <memory>
#include <optional>
#include <system_error>
#include <type_traits>

namespace bt = beman::transpose;
namespace laws = beman::transpose::test::laws;

// =========================================================================
// GRADE PROBES: three distinct named std enum types.
//
// Inside the interim type-ordering fallback's restrictions BY CONSTRUCTION
// -- named, external linkage, stable spelling -- so the harness never
// depends on the P2830 stand-in behaving outside what error_set.hpp
// documents. Three generators give a free semilattice of eight elements,
// which is the smallest sample that exercises every law shape: the unit at
// ∅, idempotence on the diagonal, commutativity off it, and associativity
// on triples that are not all comparable.
// =========================================================================

using empty_grade = bt::error_set<>;
using e_c = bt::error_set<std::errc>;
using e_i = bt::error_set<std::io_errc>;
using e_f = bt::error_set<std::future_errc>;
using e_ci = bt::error_set<std::errc, std::io_errc>;
using e_cf = bt::error_set<std::errc, std::future_errc>;
using e_if = bt::error_set<std::io_errc, std::future_errc>;
using e_cif = bt::error_set<std::errc, std::io_errc, std::future_errc>;

// The eight elements really are eight, not fewer: the interim ordering
// separates the three generators, so no two subsets collapsed into one type.
static_assert(!std::is_same_v<e_c, e_i>);
static_assert(!std::is_same_v<e_c, e_f>);
static_assert(!std::is_same_v<e_i, e_f>);
static_assert(!std::is_same_v<e_ci, e_cf>);
static_assert(!std::is_same_v<e_ci, e_if>);
static_assert(!std::is_same_v<e_cf, e_if>);
static_assert(!std::is_same_v<e_ci, e_cif>);

/** The shipped model, as a harness subject. */
struct error_set_model {
    using grades =
        laws::grade_sample<empty_grade, e_c, e_i, e_f, e_ci, e_cf, e_if, e_cif>;
    using values =
        laws::value_sample<int, std::optional<int>, std::unique_ptr<int>>;
};

static_assert(laws::check_graded_laws<error_set_model>());

// =========================================================================
// THE SECOND MODEL: the Boolean semilattice {⊥, ⊤}.
//
// TEST-ONLY. Where it lives long-term -- here, or shipped as std::optional's
// official grade registration -- is the still-open question
// docs/decisions.md#optional-grade-model, and this stage does not settle it.
// That is why the carrier below is a purpose-built `Fallible`, not
// std::optional: registering std::optional as a graded carrier would answer
// the open question by fiat, and would contradict the ∅-default assertions
// tests/beman/transpose/grade.test.cpp pins on it.
//
// Its job here is to be a LEAK DETECTOR, per
// docs/decisions.md#grade-generality: an abstraction with one model is
// renamed, not generic. Every place this fails to slide in cleanly is a
// framework/model leak and gets logged as a divergence -- never patched
// around, and never smoothed by special-casing the framework to accommodate
// it.
//
// It is also the smallest possible non-trivial grade algebra: two elements,
// no parameters, no type ordering, no canonicalization. Anything the
// framework demands of it is something the framework demands of every
// algebra, which is exactly what makes it diagnostic.
// =========================================================================

namespace beman_transpose_boolean_grade {

/** ⊥: this computation cannot fail. */
struct never_fails {};

/** ⊤: this computation may fail. The whole algebra is {⊥, ⊤} with ⊥ ⊑ ⊤. */
struct may_fail {};

/** The ⊤ carrier: a value, or nothing. Deliberately a distinct type rather
 * than std::optional -- see the section comment. Impoverished on purpose:
 * the harness needs construction from a value, copy/move, and equality, and
 * nothing else. */
template <class VALUE>
class Fallible {
  public:
    constexpr Fallible() = default;

    // NOLINTNEXTLINE(*-explicit-constructor) -- promotion is a subsumption
    // coercion and must be implicit, exactly as expected's from-T is.
    constexpr Fallible(VALUE value) : d_value(std::move(value)) {}

    constexpr auto has_value() const noexcept -> bool {
        return d_value.has_value();
    }

    friend auto operator==(const Fallible &, const Fallible &)
        -> bool = default;

  private:
    std::optional<VALUE> d_value;
};

} // namespace beman_transpose_boolean_grade

namespace boolean_grade = beman_transpose_boolean_grade;

using boolean_grade::Fallible;
using boolean_grade::may_fail;
using boolean_grade::never_fails;

namespace beman::transpose {

// -- The algebra: four joins, two bottoms, four order facts. This part is
// -- exactly what a grade algebra should cost, and it slides in cleanly.

template <>
struct grade_join<never_fails, never_fails> {
    using type = never_fails;
};
template <>
struct grade_join<never_fails, may_fail> {
    using type = may_fail;
};
template <>
struct grade_join<may_fail, never_fails> {
    using type = may_fail;
};
template <>
struct grade_join<may_fail, may_fail> {
    using type = may_fail;
};

template <>
struct grade_bottom<never_fails> {
    using type = never_fails;
};
template <>
struct grade_bottom<may_fail> {
    using type = never_fails;
};

template <>
struct grade_subsumes<never_fails, never_fails> : std::true_type {};
template <>
struct grade_subsumes<never_fails, may_fail> : std::true_type {};
template <>
struct grade_subsumes<may_fail, never_fails> : std::false_type {};
template <>
struct grade_subsumes<may_fail, may_fail> : std::true_type {};

// -- The carrier registration. This part does NOT slide in cleanly: every
// -- specialization below is a line-for-line transliteration of one in
// -- error_set.hpp, with `Fallible` where `std::expected` stood. See the
// -- divergence logged at docs/decisions.md#bottom-carrier-ownership.

namespace detail {

template <class T>
inline constexpr bool is_fallible_v = false;

template <class VALUE>
inline constexpr bool is_fallible_v<Fallible<VALUE>> = true;

} // namespace detail

template <class VALUE>
struct grade_of<Fallible<VALUE>> {
    using type = may_fail;
};

/** Promotion: a bare value re-indexed at ⊤ acquires the carrier. Needs the
 * negative constraint for the same reason error_set.hpp's does -- so that
 * re-indexing a carrier does not nest one inside another. */
template <class VALUE>
    requires(!detail::is_fallible_v<VALUE>)
struct rebind_grade<VALUE, may_fail> {
    using type = Fallible<VALUE>;
};

template <class VALUE>
struct rebind_grade<Fallible<VALUE>, may_fail> {
    using type = Fallible<VALUE>;
};

/** Re-indexing at the model's OWN ∅ yields the bare value. The framework
 * already says this for `unit_grade`, and says nothing for a model's bottom,
 * so every model writes it again. */
template <class VALUE>
    requires(!detail::is_fallible_v<VALUE>)
struct rebind_grade<VALUE, never_fails> {
    using type = VALUE;
};

template <class VALUE>
struct rebind_grade<Fallible<VALUE>, never_fails> {
    using type = VALUE;
};

} // namespace beman::transpose

/** The Boolean semilattice, as a harness subject. Two elements is the whole
 * algebra, so the sample is exhaustive rather than representative. */
struct boolean_model {
    using grades = laws::grade_sample<never_fails, may_fail>;
    using values =
        laws::value_sample<int, std::optional<int>, std::unique_ptr<int>>;
};

static_assert(laws::check_graded_laws<boolean_model>());

TEST_CASE("laws: the shipped model satisfies the graded laws") {
    // Compile-time only; reaching here means the static_assert above held.
    SUCCEED("error_set is a bounded join-semilattice with coherent carriers");
}

TEST_CASE("laws: a second, unrelated model satisfies the same laws") {
    SUCCEED("the Boolean semilattice passes the harness written against the "
            "concept");
}
