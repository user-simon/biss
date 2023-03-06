#include "rule.h"
#include "utility.h"

using namespace engine::rule;

Rule::Rule(Predicate predicate, Result result) : predicate(std::move(predicate)), result(std::move(result))
{
    
}

ast::Ast Rule::apply(ast::Ast expr) const
{
    assert(false);
}
