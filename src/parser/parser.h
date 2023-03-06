#pragma once
#include "ast/ast.h"

#include <stdexcept>
#include <string_view>

namespace parser
{
    struct Error : std::runtime_error
    {
        // column of the error
        std::size_t column;

        // error message
        std::string msg;

        Error(std::size_t column, std::string msg) : std::runtime_error(msg.data()), column(column), msg(std::move(msg)) {}
    };

    // parses input string as an expression
    ast::Ast parse(std::string_view input);
}
