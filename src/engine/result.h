#pragma once
#include "ast/ast.h"

#include <functional>
#include <type_traits>
#include <vector>
#include <cassert>

namespace engine::result
{
    struct Result;

    struct Tag
    {
        std::uint8_t value;
    };

    struct Ast
    {
        ast::Ast value;
    };

    struct Call
    {
        const ast::function::Function* fn;
        std::vector<Result> args;

        template<class... Args>
        Call(std::string_view identifier, Args&&... args);
        Call(const ast::function::Function* fn, std::vector<Result> args);
    };

    struct Result : Variant<Tag, Ast, Call>
    {
        using Variant<Tag, Ast, Call>::Variant;

        // implicitly delete copy constructor to forbid accidental copies
        Result(Result&&) = default;
        Result& operator=(Result&&) = default;

        template<class T> requires(std::is_convertible_v<T, ast::Ast>)
        Result(T expr) : Result(Ast{ std::move(expr) }) {}
    };

    inline Call::Call(const ast::function::Function* fn, std::vector<Result> args)
        : fn(fn), args(std::move(args))
    {
        assert(fn);
    }

    template<class... Args>
    inline Call::Call(std::string_view identifier, Args&&... args) : Call
    (
        ast::function::get(identifier, sizeof...(args)),
        move_to_vector<Result>(std::forward<Args>(args)...)
    ) {}

    namespace prelude
    {
        using result::Tag;
        using result::Ast;
        using result::Call;
        using result::Result;
    }
}
