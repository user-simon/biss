#pragma once
#include "utility.h"

#include <string_view>
#include <vector>
#include <unordered_set>
#include <format>

namespace parser
{
    // token containing a function identifier. doesn't store a reference to the function object itself since
    // that would require knowing the arity.
    struct Identifier
    {
        std::string_view value;
    };

    // signals the end of the token stream
    struct EOL {};

    // token sum type
    struct Token : Variant<char, double, std::string_view, Identifier, EOL>
    {
        using Variant<char, double, std::string_view, Identifier, EOL>::Variant;

        std::string to_string() const
        {
            return visit
            (
                [](char t)             { return std::format("{}", t);       },
                [](double t)           { return std::format("{}", t);       },
                [](std::string_view t) { return std::format("{}", t);       },
                [](Identifier t)       { return std::format("{}", t.value); },
                [](EOL)                { return std::string("EOL");         }
            );
        }
    };
    
    // tokenizes input string
    struct Lexer
    {
        Lexer(std::string_view str, const std::unordered_set<std::string_view>& fn_identifiers);

        // retrieves a token from the buffer without consuming it
        const Token& peek() const;
        
        // consumes a token from the buffer
        const Token& read();

        // discards a token from the buffer
        void discard();

        // returns the column of start of the previous token, used for error reporting
        std::size_t last_token_start() const;

    private:
        std::vector<Token> _tokens;
        std::vector<std::size_t> _offsets;
        std::size_t _cursor;
    };
}

template<class CharT>
struct std::formatter<parser::Token, CharT> : std::formatter<std::string, CharT>
{
    template<class FormatContext>
    auto format(parser::Token t, FormatContext& fc) const
    {
        return std::formatter<std::string, CharT>::format(t.to_string(), fc);
    }
};
