#include "parser.h"
#include "lexer.h"
#include "ast/ast.h"
#include "ast/function.h"
#include "ast/precedence.h"

using namespace parser;
using namespace ast::prelude;
using namespace function::prelude;

namespace impl
{
    // forward declares
    static Ast parse_primary(Lexer& lexer);
    static Ast parse_precedence(Lexer& lexer, Ast lhs, Precedence min, const Function* from);
    static Ast parse_routine_call(Lexer& lexer, std::string_view identifier);
    static Ast parse_shorthand(Lexer& lexer, Ast lhs);

    // utility for throwing a syntax error at the column where the previous token started
    template<class... Args>
    [[noreturn]]
    static void error(Lexer& lexer, std::format_string<Args&...> fmt, Args&&... args)
    {
        throw Error(lexer.last_token_start(), std::format(fmt, args...));
    }

    // attempts to parse the next token as a function identifier, and returns the function object
    static const Function* peek_function(Lexer& lexer, std::uint8_t arity)
    {
        const Token token = lexer.peek();

        if (token.has<Identifier>())
        {
            const std::string_view identifier = token.get<Identifier>().value;
            return function::get(identifier, arity);
        }
        return nullptr;
    }

    // root of the parsing algorithm. based on the precedence climbing method:
    // https://en.wikipedia.org/wiki/Operator-precedence_parser
    static Ast parse_expression(Lexer& lexer)
    {
        Ast lhs = parse_primary(lexer);
        return parse_precedence(lexer, std::move(lhs), precedence::LOWEST, nullptr);
    }

    // parses an expression with a certain minimum precedence, parses infix function calls
    static Ast parse_precedence(Lexer& lexer, Ast lhs, Precedence min, const Function* from)
    {
        const Function* fn = nullptr;

        while ((fn = (from ? from : peek_function(lexer, 2))) && fn->precedence >= min)
        {
            lexer.discard(); // op identifier

            Ast rhs = parse_primary(lexer);
            const Function* sub_fn = nullptr;

            while ((sub_fn = peek_function(lexer, 2)) && (
                 sub_fn->precedence  > fn->precedence ||
                (sub_fn->precedence == fn->precedence && fn->associativity == Associativity::RIGHT)
            )) {
                rhs = parse_precedence
                (
                    lexer,
                    std::move(rhs),
                    precedence::next(min),
                    sub_fn
                );
            }
            lhs = Call { fn, std::move(lhs), std::move(rhs) };
        }
        return lhs;
    }
    
    // parses "unit" expressions like parenthesized expressions, literals, unary functions, or routine
    // function calls
    static Ast parse_primary(Lexer& lexer)
    {
        const Token token = lexer.read();

        if (token.has<EOL>())
            error(lexer, "expected an expression");

        Ast expr = [&]() -> Ast
        {
            if (token.has('(')) // parse nested expression
            {
                Ast nested = parse_expression(lexer);

                if (!lexer.read().has(')'))
                    error(lexer, "expected ')'");
                return nested;
            }
            else if (token.has<std::string_view>()) // parse variable
            {
                return Variable{ std::string(token.get<std::string_view>()) };
            }
            else if (token.has<Identifier>()) // parse unary / routine function call
            {
                const std::string_view identifier = token.get<Identifier>().value;

                if (lexer.peek().has('('))
                {
                    return parse_routine_call(lexer, identifier);
                }
                else
                {
                    const Function* fn = function::get(identifier, 1);

                    if (!fn)
                        error(lexer, "function is not a unary operator '{}'", identifier);
                    Ast arg = parse_primary(lexer);
                    return Call{ fn, std::move(arg) };
                }
            }
            else if (token.has<double>()) // parse literal
            {
                return Literal{ token.get<double>() };
            }
            else // invalid token
            {
                error(lexer, "invalid token '{}'", token);
            }
        }();
        return parse_shorthand(lexer, std::move(expr));
    }
    
    // parses a routine function call
    static Ast parse_routine_call(Lexer& lexer, std::string_view identifier)
    {
        lexer.discard(); // '('
        std::vector<Ast> args;
        bool first = true;

        while (!lexer.peek().has(')'))
        {
            if (first || lexer.read().has(','))
                args.push_back(parse_expression(lexer));
            else
                error(lexer, "expected ',' or ')'");
            first = false;
        }
        lexer.discard(); // ')'
        const std::size_t arity = args.size();
        const Function* fn = function::get(identifier, arity);

        if (!fn)
            error(lexer, "no overload found for '{}' taking {} arguments", identifier, arity);
        return Call{ fn, std::move(args) };
    }

    // if a primary expression immediately follows the current one (`lhs`), interpret as a multiplication.
    // allows e.g. `2(1 + 2)` and `5x`
    static Ast parse_shorthand(Lexer& lexer, Ast lhs)
    {
        const Token next = lexer.peek();
        const bool next_is_primary = next.has<std::string_view>() || next.has<double>() || next.has('(');

        if (next_is_primary)
        {
            constexpr const Function* MULTIPLICATION = function::get("*", 2);
            Ast rhs = parse_primary(lexer);
            return Call{ MULTIPLICATION, std::move(lhs), std::move(rhs) };
        }
        else
        {
            return lhs;
        }
    }
}

Ast parser::parse(std::string_view input)
{
    Lexer lexer{ input, function::IDENTIFIERS };
    Ast expr = impl::parse_expression(lexer);

    if (Token trailing; !(trailing = lexer.read()).has<EOL>())
        impl::error(lexer, "unexpected '{}'", trailing);
    return expr;
}
