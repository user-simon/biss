#include "ast.h"
#include <format>
using namespace ast::prelude;
using namespace function::prelude;

namespace impl
{
    // formats function call with the function name between each operand; `1 + 2 + 3`
    static std::string format_infix(const Function* fn, const std::vector<Ast>& args)
    {
        const auto& format_operand = [&](const Ast& arg)
        {
            const std::string arg_out = arg.to_string();

            if (arg.precedence() <= fn->precedence && arg.has<Call>())
                return std::format("({})", arg_out);
            else
                return arg_out;
        };
        if (args.size() == 1)
        {
            return std::format("{}{}", fn->identifier, format_operand(args[0]));
        }
        else
        {
            const std::string delimiter = std::format(" {} ", fn->identifier);
            std::string out;

            for (std::size_t i = 0; i < args.size(); i++)
            {
                if (i)
                    out += delimiter;
                out += format_operand(args[i]);
            }
            return out;
        }
    }

    // formats function call with the function name before all operands; `min(1, 2, 3)`   
    static std::string format_routine(const Function* fn, const std::vector<Ast>& args)
    {
        std::string args_string = "";

        for (std::size_t i = 0; i < args.size(); i++)
        {
            if (i)
                args_string += ", ";
            args_string += args[i].to_string();
        }
        return std::format("{}({})", fn->identifier, args_string);
    }

    // utility to clone args using the `Ast::copy` interface
    static std::vector<Ast> copy_args(const std::vector<Ast>& args)
    {
        std::vector<Ast> cloned_args;
        cloned_args.reserve(args.size());

        for (const auto& arg : args)
            cloned_args.push_back(arg.copy());
        return cloned_args;
    }
}

Call::Call(const Call& other) : Call(other.fn, impl::copy_args(other.args))
{
    
}

Call& Call::operator=(const Call& other)
{
    args = impl::copy_args(other.args);
    fn = other.fn;
    return *this;
}

Precedence Ast::precedence() const
{
    return visit
    (
        [](const Call& e) { return e.fn->precedence; },
        [](const auto&)   { return Precedence::L1;   }
    );
}

std::string Ast::to_string() const
{
    return visit
    (
        [](const Call& e) -> std::string
        {
            if (e.fn->syntax == Syntax::INFIX)
                return impl::format_infix(e.fn, e.args);
            else
                return impl::format_routine(e.fn, e.args);
        },
        [](const Literal& e) -> std::string
        {
            return std::format("{}", e.value);
        },
        [](const Variable& e) -> std::string
        {
            return e.identifier;
        }
    );
}

Ast Ast::copy() const
{
    return (Ast)*this;
}

bool ast::operator==(const Ast& a, const Ast& b)
{
    return std::visit(Overload
    {
        [](const Call& a, const Call& b) -> bool
        {
            return a.fn == b.fn && a.args == b.args;
        },
        [](const Literal& a, const Literal& b) -> bool
        {
            return double_equality(a.value, b.value);
        },
        [](const Variable& a, const Variable& b) -> bool
        {
            return a.identifier == b.identifier;
        },
        [](const auto&, const auto&) -> bool
        {
            return false;
        },
    }, a, b);
}
