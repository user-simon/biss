#pragma once
#include "ast/ast.h"

namespace engine
{
    // parses and evaluates string
    ast::Ast evaluate_str(std::string_view str);

    // evaluates expression. this is a massive TODO, currently only flattens the expression
    ast::Ast evaluate_expr(ast::Ast ast);
}
