#include <iostream>
#include <iterator>
#include <utility>
#include <variant>

#include "parser/parser.h"
#include "engine/engine.h"

int main()
{
    std::string input;

    while (true)
    {
        std::cout << "> ";
        std::getline(std::cin, input);

        const std::string out = [&]()
        {
            try
            {
                return engine::evaluate_str(input).to_string();
            }
            catch (const parser::Error& e)
            {
                const std::string column_indicator = std::string(e.column, ' ');
                return std::format("{}  ^ {}", column_indicator, e.what());
            }
        }();
        std::cout << out << '\n';
    }
}
