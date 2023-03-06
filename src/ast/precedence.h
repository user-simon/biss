#pragma once
#include <cstdint>

namespace ast::precedence
{
    // defines what levels of precedence are available. based on operator precedence in C++
    enum class Precedence : std::uint8_t
    {
        L6, // ||
        L5, // && ^^
        L4, // == != < <= > >=
        L3, // + -
        L2, // * /
        L1, // **, functions, unary operators, variables, literals
    };

    // returns the next level of precedence
    constexpr Precedence next(Precedence p)
    {
        if (p == Precedence::L1)
            return Precedence::L1;
        else
            return static_cast<Precedence>(static_cast<std::uint8_t>(p) + 1);
    }

    constexpr Precedence LOWEST = Precedence::L6;
}
