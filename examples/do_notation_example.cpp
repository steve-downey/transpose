// examples/do_notation_example.cpp                                   -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// A Haskell `do` block, desugared by hand and carried over to the Monad
// typeclass. The source program is the closing example of the wikibooks
// do-notation chapter (https://en.wikibooks.org/wiki/Haskell/do_notation),
// chosen there because every desugaring rule fires at least once:
//
//   nameReturnAndCarryOn = do putStr "What is your first name? "
//                             first <- getLine
//                             putStr "And your last name? "
//                             last <- getLine
//                             let full = first++" "++last
//                             putStrLn ("Pleased to meet you, "++full++"!")
//                             return full
//                             putStrLn "I am not finished yet!"
//
// The Haskell 2010 report (section 3.14) rewrites a do block one statement
// at a time:
//
//   do {e}              =  e
//   do {e; stmts}       =  e >> do {stmts}
//   do {v <- e; stmts}  =  e >>= \v -> do {stmts}    -- v a plain variable,
//                                                    -- so the match cannot
//                                                    -- fail and no MonadFail
//                                                    -- alternative is needed
//   do {let ds; stmts}  =  let ds in do {stmts}
//
// Applying them until no `do` remains:
//
//   nameReturnAndCarryOn =
//     putStr "What is your first name? " >>
//     (getLine >>= \first ->
//      putStr "And your last name? " >>
//      (getLine >>= \last ->
//       let full = first ++ " " ++ last
//       in putStrLn ("Pleased to meet you, " ++ full ++ "!") >>
//          (return full >>
//           putStrLn "I am not finished yet!")))
//
// Everything left standing is Monad, not Applicative: the continuations
// handed to >>= use the values bound by earlier statements, which is
// exactly the dependency Applicative cannot express. So the translation
// below targets monad_typeclass, and the pieces map as
//
//   >>=            bind        (the typeclass basis; mbind at call sites)
//   >>             then        (m >> k  =  m >>= \_ -> k, its default
//                               definition in the Monad class)
//   return         pure        (the operation's modern name, and `return`
//                               is spoken for in C++)
//   \v -> e        a by-value lambda
//   let v = e in   a local variable in the continuation
//
// IO here is the teaching model of IO: a function that is given the world
// and hands back a value. The world is only what the demo needs to be
// deterministic -- the lines the user has yet to type.

#include <beman/transpose/monad.hpp>

#include <deque>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace {

/// Haskell's unit type, ().
struct Unit {};

/// What is left of RealWorld once the session is scripted: the lines the
/// user is going to type. Output goes straight to stdout, as it does there.
struct World {
    std::deque<std::string> input;
};

/// IO a: a computation that, given the world, produces an `a`.
template <class VALUE_TYPE>
struct IO {
    std::function<VALUE_TYPE(World &)> run;
};

/// Monad instance for IO: pure produces the value and leaves the world
/// alone; bind runs `ma`, then runs the action the continuation builds
/// from its result, threading the one world through both.
template <class VALUE_TYPE>
struct IOMonadImpl {
    using element_type = VALUE_TYPE;

    template <class VALUE>
    auto pure(this auto &&, VALUE &&value) -> IO<std::remove_cvref_t<VALUE>> {
        return {
            [value = std::forward<VALUE>(value)](World &) { return value; }};
    }

    template <class A, class F>
    auto bind(this auto &&, IO<A> ma, F &&f) -> std::invoke_result_t<F &, A> {
        using MB = std::invoke_result_t<F &, A>;
        return MB{[ma = std::move(ma), f = std::forward<F>(f)](World &world) {
            return std::invoke(f, ma.run(world)).run(world);
        }};
    }
};

template <class VALUE_TYPE>
struct IOMonadMap : beman::transpose::Monad<IOMonadImpl<VALUE_TYPE>> {
    using IOMonadImpl<VALUE_TYPE>::bind;
    using IOMonadImpl<VALUE_TYPE>::pure;
};

} // namespace

namespace beman::transpose {
/// Register the instance so mbind and the generic machinery find it.
template <class VALUE_TYPE>
inline constexpr auto monad_typeclass<IO<VALUE_TYPE>> =
    IOMonadMap<VALUE_TYPE>{};
} // namespace beman::transpose

namespace {

namespace bt = beman::transpose;

// -- The Prelude, one name at a time --

IO<Unit> putStr(std::string text) {
    return {[text = std::move(text)](World &) {
        std::cout << text;
        return Unit{};
    }};
}

IO<Unit> putStrLn(std::string line) {
    return {[line = std::move(line)](World &) {
        std::cout << line << '\n';
        return Unit{};
    }};
}

/// Consumes the next scripted line, echoing it as the terminal would.
IO<std::string> getLine() {
    return {[](World &world) {
        std::string line = std::move(world.input.front());
        world.input.pop_front();
        std::cout << line << '\n';
        return line;
    }};
}

/// Haskell's return.
template <class VALUE>
IO<VALUE> pure(VALUE value) {
    return bt::monad_typeclass<IO<VALUE>>.pure(std::move(value));
}

/// (>>): sequence two actions, discarding the first result.
/// m >> k  =  m >>= \_ -> k
template <class A, class B>
IO<B> then(IO<A> action, IO<B> continuation) {
    return bt::mbind(std::move(action),
                     [continuation = std::move(continuation)](const A &) {
                         return continuation;
                     });
}

// -- The desugared block, call for call --
//
// The type is IO<Unit>, not IO<std::string>: a do block has the type of its
// last statement, and `return full` is just one more statement. It neither
// ends the computation nor sets its result.

IO<Unit> nameReturnAndCarryOn() {
    return then(
        putStr("What is your first name? "),
        bt::mbind(getLine(), [](std::string first) {
            return then(
                putStr("And your last name? "),
                bt::mbind(getLine(), [first](std::string last) {
                    auto full = first + " " + last;
                    return then(
                        putStrLn("Pleased to meet you, " + full + "!"),
                        then(pure(full), putStrLn("I am not finished yet!")));
                }));
        }));
}

} // namespace

int main() {
    World world{.input = {"John", "Smith"}};
    nameReturnAndCarryOn().run(world);
    return 0;
}
