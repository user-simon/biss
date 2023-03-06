#include "engine.h"
#include "ast/function.h"
#include "parser/parser.h"
#include <iterator>
#include <vector>
using namespace ast::prelude;
using namespace function::prelude;

namespace impl
{
    // collect the arguments of all nested calls of the same function into the top-level function arguments,
    // provided that the function has a dynamic arity. this is beneficial as it canonicalizes equivalent
    // expressions such as `1 + (2 + 3)` and `(1 + 2) + 3` to a common AST `+(1, 2, 3)`, which helps with
    // predicate-matching against long nested expressions. it is also more cache-efficient since arguments
    // that are semantically "next to" each other (and will therefore be accessed at the same time) are also
    // physically next to each other in memory
    Ast flatten(ast::Ast ast)
    {
        if (!ast.has<Call>())
            return ast;

        Call& call = ast.get<Call>();
        std::vector<Ast> new_args;
        const std::size_t last_i = call.args.size() - 1;

        for (std::size_t i = 0; i < call.args.size(); i++)
        {
            Ast flat_arg = flatten(std::move(call.args[i]));
            
            if (flat_arg.has<Call>())
            {
                Call& sub_call = flat_arg.get<Call>();
                const bool associativity_match = 
                    (call.fn->associativity == Associativity::LEFT && i == 0) ||
                    (call.fn->associativity == Associativity::RIGHT && i == last_i) ||
                    (call.fn->associativity == Associativity::ALL);
                
                if (sub_call.fn == call.fn && associativity_match)
                {
                    new_args.insert
                    (
                        new_args.end(),
                        std::make_move_iterator(sub_call.args.begin()),
                        std::make_move_iterator(sub_call.args.end())
                    );
                    continue;
                }
            }
            // else
            new_args.push_back(std::move(flat_arg));
        }
        return Call{ call.fn, std::move(new_args) };
    }
}

Ast engine::evaluate_str(std::string_view str)
{
    return evaluate_expr(parser::parse(str));
}

Ast engine::evaluate_expr(ast::Ast ast)
{
    return impl::flatten(std::move(ast));
}
