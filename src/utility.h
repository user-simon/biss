#pragma once
#include <variant>
#include <vector>

// utility to combine several callable types
template<class... Overloads>
struct Overload : Overloads...
{
    using Overloads::operator()...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

// wrapper for std::variant to provide commonly used utilities with a terser syntax
template<class... Types>
struct Variant : std::variant<Types...>
{
    using std::variant<Types...>::variant;

    template<class T>
    constexpr T& get()
    {
        return std::get<T>(*this);
    }

    template<class T>
    constexpr const T& get() const
    {
        return std::get<T>(*this);
    }

    template<class T>
    constexpr bool has() const
    {
        return std::holds_alternative<T>(*this);
    }

    template<class T>
    constexpr bool has(const T& value) const
    {
        return has_predicate<T>([&](auto& actual) { return actual == value; });
    }

    template<class T, class Predicate>
    constexpr bool has_predicate(Predicate pred) const
    {
        return has<T>() && pred(get<T>());
    }

    template<class... Overloads>
    constexpr auto visit(Overloads&& ...overloads) const
    {
        return std::visit(Overload { overloads... }, *this);
    }
};

// you'd think this wouldn't be necessary, but as it turns out, std::vector makes copies of the elements&&
// you give it's constructor. 
template<class T, class... Args>
constexpr std::vector<T> move_to_vector(Args&&... args)
{
    T init[] = { std::forward<Args>(args)... };
    return
    {
        std::make_move_iterator(std::begin(init)),
        std::make_move_iterator(std::end(init))
    };
}

// determines if two floating-point numbers are equivalent
// https://stackoverflow.com/a/35252979
constexpr bool double_equality(double a, double b)
{
    constexpr double EPSILON = 1e-10; // arbitrarily chosen
    return std::abs(a - b) <= EPSILON * std::max(std::abs(a), std::abs(b));
}
