#pragma once
#include "precedence.h"

#include <string_view>
#include <unordered_set>
#include <array>

namespace ast::function
{
    namespace impl
    {
        // fnv1a from https://create.stephan-brumme.com/fnv-hash/
        constexpr inline std::uint32_t hash(std::string_view data)
        {
            constexpr std::uint32_t SEED  = 0x811C9DC5;
            constexpr std::uint32_t PRIME = 0x01000193;

            std::uint32_t hash = SEED;
            
            while (!data.empty())
            {
                hash = (data[0] ^ hash) * PRIME;
                data.remove_prefix(1);
            }
            return hash;
        }
    }

    // determines how arguments in a function call may be reordered
    enum class Commutativity
    {
        // no arguments may be reordered; `2**3 != 3**2`
        NONE,
        // all arguments may be reordered; `a + b + c == c + b + a`
        ALL,

        // all arguments but the first may be reordered; `a - b - c == a - c - b`
        TAIL
    };

    // determines how nested function calls may be flattened (see `engine.cpp -> flatten`)
    enum class Associativity
    {
        // no arguments may be flattened; used mainly for comparison operators whose input is incompatible
        // with their output.
        NONE,
        // `(a ⚬ b) ⚬ c` is equivalent to `a ⚬ b ⚬ c`
        LEFT,
        // `a ⚬ (b ⚬ c)` is equivalent to `a ⚬ b ⚬ c`
        RIGHT,
        // `(a ⚬ b) ⚬ c` and `a ⚬ (b ⚬ c)` are both equivalent to `a ⚬ b ⚬ c`
        ALL,
    };

    // defines the format of function calls
    enum class Syntax
    {
        // serializes using format `⚬(a, b, c)`; `max(a, b, c)`
        ROUTINE,
        // serializes using format `a ⚬ b ⚬ c`; `a + b + c`
        INFIX,
    };

    // determines what number of arguments are compatible with the function
    enum struct ArityType
    {
        // number of arguments must be exactly specified amount; `sqrt(a)`
        STATIC,
        // number of arguments may be specified amount or more; `min(v0, v1, .., vN)`
        DYNAMIC,
    };
    
    // contains meta-data about all functions recognized by the parser. evaluation of function calls is later
    // performed using rules
    struct Function
    {
        const std::string_view identifier;
        const Syntax syntax;
        const Commutativity commutativity;
        const Associativity associativity;
        const ArityType arity_type;
        const std::uint8_t arity;
        const precedence::Precedence precedence;
        const std::uint32_t identifier_hash;

        // creates a new unary operator function
        constexpr static Function unary(std::string_view identifier)
        {
            return Function
            {
                identifier,
                Syntax::INFIX,
                Commutativity::ALL,
                Associativity::RIGHT,
                ArityType::STATIC,
                1,
                precedence::Precedence::L1,
                impl::hash(identifier),
            };
        }

        // creates a new binary operator function
        constexpr static Function binary(std::string_view identifier, precedence::Precedence precedence, Commutativity commutativity, Associativity associativity)
        {
            return Function
            {
                identifier,
                Syntax::INFIX,
                commutativity,
                associativity,
                ArityType::DYNAMIC,
                2,
                precedence,
                impl::hash(identifier),
            };
        }

        // creates a new routine function
        constexpr static Function routine(std::string_view identifier, Commutativity commutativity, Associativity associativity, ArityType arity_type, std::uint8_t arity)
        {
            return Function
            {
                identifier,
                Syntax::ROUTINE,
                commutativity,
                associativity,
                arity_type,
                arity,
                precedence::Precedence::L1,
                impl::hash(identifier),
            };
        }
    };
    
    inline constexpr auto ARRAY = []()
    {
        using Cm = Commutativity;
        using As = Associativity;
        using enum Syntax;
        using enum ArityType;
        using enum precedence::Precedence;
        
        return std::array
        {
            // arithmetic operators
            Function::unary("-"),
            Function::binary("**", L1, Cm::NONE, As::RIGHT),
            Function::binary("*",  L2, Cm::ALL,  As::ALL),
            Function::binary("/",  L2, Cm::TAIL, As::LEFT),
            Function::binary("%",  L2, Cm::NONE, As::LEFT),
            Function::binary("+",  L3, Cm::ALL,  As::ALL),
            Function::binary("-",  L3, Cm::TAIL, As::LEFT),

            // comparison operators
            Function::binary("==", L4, Cm::ALL,  As::NONE),
            Function::binary("!=", L4, Cm::ALL,  As::NONE),
            Function::binary("<",  L4, Cm::NONE, As::NONE),
            Function::binary("<=", L4, Cm::NONE, As::NONE),
            Function::binary(">",  L4, Cm::NONE, As::NONE),
            Function::binary(">=", L4, Cm::NONE, As::NONE),

            // logical operators
            Function::unary("!"),
            Function::binary("&&", L5, Cm::ALL, As::LEFT),
            Function::binary("^^", L5, Cm::ALL, As::LEFT),
            Function::binary("||", L6, Cm::ALL, As::LEFT),

            // routines
            Function::routine("sqrt", Cm::ALL,  As::NONE, STATIC, 1),
            Function::routine("abs",  Cm::ALL,  As::NONE, STATIC, 1),
            Function::routine("min",  Cm::NONE, As::ALL,  DYNAMIC, 2),
            Function::routine("max",  Cm::NONE, As::ALL,  DYNAMIC, 2),
        };
    }();

    inline const std::unordered_set<std::string_view> IDENTIFIERS = []()
    {
        std::unordered_set<std::string_view> out;

        for (const auto& fn : ARRAY)
            out.emplace(fn.identifier);
        return out;
    }();

    constexpr inline const Function* get(std::string_view identifier, std::uint8_t arity)
    {
        const std::uint32_t hash = impl::hash(identifier);

        for (const auto& fn : ARRAY)
        {
            const bool arity_match = fn.arity_type == ArityType::STATIC
                ? arity == fn.arity
                : arity >= fn.arity;
            if (arity_match && fn.identifier_hash == hash)
                return &fn;
        }
        return nullptr;
    }

    namespace prelude
    {
        using function::Commutativity;
        using function::Associativity;
        using function::Syntax;
        using function::ArityType;
        using function::Function;
    }
}
