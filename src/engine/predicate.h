#pragma once
#include "ast/function.h"
#include "utility.h"

#include <memory>
#include <vector>
#include <optional>
#include <type_traits>
#include <span>
#include <cassert>

namespace engine::predicate
{
    struct Predicate;

    template<class Child>
    struct Taggable
    {
        Predicate operator[](std::uint8_t tag);
    };

    struct Any : Taggable<Any>
    {
        Any() {}
    };

    struct Literal : Taggable<Literal>
    {
        std::optional<double> value;

        Literal(std::optional<double> value) : value(value) {}
    };

    struct Tag
    {
        std::unique_ptr<Predicate> nested;
        std::uint8_t tag;
        
        Tag(Predicate nested, std::uint8_t tag);
    };

    struct Variable : Taggable<Variable>
    {
        Variable() {}

        Predicate operator[](std::uint8_t tag);
    };

    struct Call : Taggable<Call>
    {
        const ast::function::Function* fn;
        std::vector<Predicate> args;
        
        template<class... Args>
        Call(std::string_view identifier, Args&&... args);
        Call(const ast::function::Function* fn, std::vector<Predicate> args);

        Predicate operator[](std::uint8_t tag);
    };

    struct Predicate : Variant<Any, Literal, Tag, Variable, Call>
    {
        using Variant<Any, Literal, Tag, Variable, Call>::Variant;

        // implicitly delete copy constructor to forbid accidental copies
        Predicate(Predicate&&) = default;
        Predicate& operator=(Predicate&&) = default;

        Predicate(double value) : Predicate(Literal{ value }) {}
    };
    
    template<class Child>
    Predicate Taggable<Child>::operator[](std::uint8_t tag)
    {
        return Tag{ std::move(*static_cast<Child*>(this)), tag };
    }

    inline Tag::Tag(Predicate nested, std::uint8_t tag)
        : nested(std::make_unique<Predicate>(std::move(nested))), tag(tag) {}
    
    template<class... Args>
    inline Call::Call(std::string_view identifier, Args&&... args) : Call
    (
        ast::function::get(identifier, sizeof...(args)),
        move_to_vector<Predicate>(std::forward<Args>(args)...)
    ) {}

    inline Call::Call(const ast::function::Function* fn, std::vector<Predicate> args)
        : fn(fn), args(std::move(args))
    {
        assert(fn);
    }

    namespace prelude
    {
        using predicate::Any;
        using predicate::Literal;
        using predicate::Tag;
        using predicate::Variable;
        using predicate::Call;
        using predicate::Predicate;
    }
}
