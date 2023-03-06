#pragma once
#include "ast/precedence.h"
#include "ast/function.h"
#include "utility.h"

#include <vector>
#include <string>
#include <format>
#include <cassert>

namespace ast
{
    // forward declaration
    struct Ast;

    // represents a literal (constant) numerical value
    struct Literal
    {
        double value;

        constexpr Literal(double value) : value(value) {}
    };

    // represents an unknown value. note that you currently cannot bind a value to variables
    struct Variable
    {
        std::string identifier;

        constexpr Variable(std::string identifier) : identifier(std::move(identifier)) {}
    };

    // represents a function call, which may be an operator (e.g. `+`) or a routine (e.g. `min`)
    struct Call
    {
        const function::Function* fn;
        std::vector<Ast> args;

        Call(const function::Function* fn, std::vector<Ast> args);
        template<class... Args>
        Call(const function::Function* fn, Args&&... args);
        
        // these have to overloaded to utilize the explicit `Ast::copy` ctor wrapper
        Call(const Call&);
        Call& operator=(const Call&);
    };

    // sum type between all different kinds of AST-nodes. represents an entire expression tree
    struct Ast : Variant<Literal, Variable, Call>
    {
        using Variant<Literal, Variable, Call>::Variant;

        Ast(Ast&&) = default;
        Ast& operator=(Ast&&) = default;
        
        precedence::Precedence precedence() const;
        std::string to_string() const;

        // force all copies of AST to be explicit, and in user code. you'd be surprised how many sneaky
        // copies C++ introduces
        Ast copy() const;
    
    private:
        Ast(const Ast&) = default;
    };
    
    bool operator==(const Ast& a, const Ast& b);

    inline Call::Call(const function::Function* fn, std::vector<Ast> args) : fn(fn), args(std::move(args))
    {
        assert(fn);
    }
    
    template<class... Args>
    inline Call::Call(const function::Function* fn, Args&&... args)
        : Call(fn, move_to_vector<Ast>(std::forward<Args>(args)...)) {}
    
    namespace prelude
    {
        using ast::Literal;
        using ast::Variable;
        using ast::Call;
        using ast::Ast;
        using ast::precedence::Precedence;

        namespace function
        {
            using namespace ast::function;
        }

        namespace precedence
        {
            using namespace ast::precedence;
        }
    }
}
