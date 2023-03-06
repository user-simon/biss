#pragma once
#include "predicate.h"
#include "result.h"

namespace engine::rule
{
    using predicate::Predicate;
    using result::Result;

    struct Rule
    {
        Predicate predicate;
        Result result;
        std::vector<std::size_t> pivot_order;

        Rule(Predicate predicate, Result result);

        ast::Ast apply(ast::Ast expr) const;
    };

    inline Rule operator>(predicate::Predicate predicate, result::Result result)
    {
        return Rule{ std::move(predicate), std::move(result) };
    }
}
