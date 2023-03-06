#include "lexer.h"
#include "ast/function.h"

#include <algorithm>
#include <cctype>
#include <utility>
using namespace parser;

namespace impl
{
    enum struct Category : std::uint8_t
    {
        ALPHA,
        DIGIT,
        SYMBOL,
        WHITESPACE,
    };

    static Category categorize(char c)
    {
        using enum Category;

        if (std::isalpha(c) || c == '_')
            return ALPHA;
        if (std::isdigit(c) || c == '.')
            return DIGIT;
        if (std::isspace(c))
            return WHITESPACE;
        return SYMBOL;
    }

    using Segments = std::vector<std::pair<Category, std::string_view>>;

    // splits string on change of symbol category; first step of tokenization
    static Segments categorize(std::string_view str)
    {
        Segments segments;
        Category prev_cat;
        std::size_t start = 0;

        for (std::size_t i = 0; i < str.length(); i++)
        {
            Category current_cat = categorize(str[i]);

            if (i && prev_cat != current_cat)
            {
                std::string_view segment = str.substr(start, i - start);
                segments.emplace_back(prev_cat, segment);
                start = i;
            }
            prev_cat = current_cat;
        }
        segments.emplace_back(prev_cat, str.substr(start));
        return segments;
    }
}

Lexer::Lexer(std::string_view str, const std::unordered_set<std::string_view>& fn_identifiers) : _cursor(0)
{
    const impl::Segments segments = impl::categorize(str);
    const std::size_t min_tokens = segments.size() + 1;

    _tokens.reserve(min_tokens);
    _offsets.reserve(min_tokens);

    std::size_t current_offset = 0;
    const auto handle_segment = [&](std::string_view segment, std::optional<Token> token)
    {
        if (token)
        {
            _tokens.push_back(std::move(*token));
            _offsets.push_back(current_offset);
        }
        current_offset += segment.size();
    };

    for (const auto& [category, segment] : segments)
    {
        using enum impl::Category;

        switch (category)
        {
        case ALPHA:
        {
            const Token token = fn_identifiers.contains(segment)
                ? Token{ Identifier{ segment } }
                : Token{ segment };
            handle_segment(segment, std::move(token));
            break;
        }
        case DIGIT: 
        {
            const double value = std::stod(std::string(segment)); // ugh
            handle_segment(segment, Token{ value });
            break;
        }
        case SYMBOL:
        {
            // add each char seperately unless it forms part of a function identifiers, such that e.g. "+==="
            // is parsed as { "+", "===", "=" } provided "==" appears in the list of function identifiers

            std::size_t start = 0;

            while (start < segment.size())
            {
                std::string_view substr = segment.substr(start);
                std::optional<Token> token;
                while (!token)
                {
                    if (fn_identifiers.contains(substr))
                        token = Token { Identifier{ substr } };
                    else if (substr.size() == 1)
                        token = Token { substr[0] };
                    else
                        substr.remove_suffix(1);
                }
                handle_segment(substr, token);
                start += substr.size();
            }
            break;
        }
        case WHITESPACE:
        {
            handle_segment(segment, {});
            break;
        }
        }
    }
    handle_segment(" ", EOL{});
}

const Token& Lexer::peek() const
{
    return _tokens[_cursor];
}

const Token& Lexer::read()
{
    const Token& out = peek();
    discard();
    return out;
}

void Lexer::discard()
{
    const std::size_t new_cursor = _cursor + 1;
    if (new_cursor < _tokens.size())
        _cursor = new_cursor;
}

std::size_t Lexer::last_token_start() const
{
    if (_cursor == 0)
        return _offsets[0];
    else
        return _offsets[_cursor - 1];
}
