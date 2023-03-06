<h1 align="center" width="100%">biss</h1>

⚠️ HEAVY WORK-IN-PROGRESS ⚠️

Currently, only the AST, parser, and general framework for rewrite rules are implemented. The next steps include implementing an algorithm for accurately determining whether an expression matches a predicate. This is much more difficult than you might think, read more [here](#predicate-matching-and-what-still-needs-to-be-done). 

---

**biss** is (or, will be) an arithmetic expression evaluation and simplification engine. This is achieved using [rules](#rules) which provide a declarative method of specifying mathematical rules that can be applied to expressions.

The overall purpose of this project is to be able to perform evaluations and at least rudimentary simplifications on arithmetic expressions supplied as strings. [main(.cpp)](src/main.cpp) defines a command-line UI for this. These examples demonstrate the eventual goal of the capabilities of the system:

```
> 1 + 2(3**2 * max(2 , 3))
55
```

```
> 1 + 2(3 + x)
2x + 7
```

```
> x**2 * 0
0
```

```
> (x + y)(z - w) / (x + y)
z - w
```

```
> 1 || (a**b)
true
```

```
> 0 ^^ sqrt(a)
bool(sqrt(a))
```

```
> 1 ^^ min(1, a)
!min(1, a)
```

```
> variable**0
1
```


# The AST

There are three nodes supported by the AST:

* Literal: contains a literal numerical value.
* Variable: contains only the variable name.
* Function call: represents a mathematical operation to be performed on sub-expressions.

Code-wise, these are implemented simply as structs containing only the necessary data [(ast.h)](src/ast/ast.h):

```cpp
struct Literal
{
    double value;
};

struct Variable
{
    std::string identifier;
};

struct Call
{
    const function::Function* fn;
    std::vector<Ast> args;
};
```

To achieve runtime polymorphism between these types, `std::variant` is used as a sum-type and replaces traditional inheritance:

```cpp
using Ast = std::variant<Literal, Variable, Call>;
```

Functionality that acts on specific AST types are therefore represented as visitors invoked with `std::visit`, instead of as virtual functions. See for the example `Ast::operator==`, which utilizes the lambda overload pattern described [here](https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/):

```cpp
bool operator==(const Ast& a, const Ast& b)
{
    return std::visit(Overload
    {
        [](const Call& a, const Call& b) -> bool
        {
            return a.fn == b.fn && a.args == b.args;
        },
        [](const Literal& a, const Literal& b) -> bool
        {
            return a.value == b.value;
        },
        [](const Variable& a, const Variable& b) -> bool
        {
            return a.identifier == b.identifier;
        },
        [](const auto&, const auto&) -> bool
        {
            return false;
        },
    }, a, b);
}
```


# Rules

*Rules* are a declarative method of encoding mathematical concepts in code. They have two constituent parts: a [*predicate*(.h)](src/engine/predicate.h), and a [*result*(.h)](src/engine/result.h). Both are generic expression patterns that may represent any number of actual expressions. The predicate describes the general "shape" an expression must have for the rule to be applicable. If so, the result describes how to rewrite the expression. 

More concretely, the predicate tree may contain the following nodes:

- `Any` matches any expression.
- `Literal` matches `ast::Literal`, and optionally matches against the value contained within. 
- `Variable` matches `ast::Variable`.
- `Call` matches `ast::Call` and contains nested predicates for each of the arguments. 

Additionally, any such node may be *tagged* for reuse in the result. Consider the rule `1 * Any → [whatever 'Any' matched to]` for an illustration as to why this is needed. In this case, we would tag `Any` with an identifier that we can refer to in the result, for example `0`: `1 * Any[0] → [0]`. 

Ultimately, rules will be used both to perform simplifications and to numerically evaluate expressions. 

# Predicate Matching (and what still needs to be done)

Matching a predicate with an expression involves determining if the expression holds the general shape required, and extracting the tagged sub-expressions if so. For the following reasons, this is not as trivial as it might seem:

1. We cannot naively match nested predicates in a `predicate::Call` against the nested expressions in an `ast::Call` 1:1, since the order of the arguments may change according to the function's commutativity. Consider once more the rule `1 * Any[0] → [0]`, now applied to the expression `x * 1`; though all sub-predicates have a corresponding expression, they are not in the same position. The matcher therefore has to be able to consider all possible permutations of arguments in a commutative function call. 
2. Decisions made locally affect the global ability to match the predicate against the expression. Consider the rule `(Any[0] * Any[1]) / Any[0] → [1]` matched against the expression `(a * b) / b`. Simply performing a recursive match and greedily tagging expressions as they are encountered would erroneously bind the tag `0` to `a`, causing the match of the denominator to fail. 
3. Arbitrary nesting of repeated operations makes it difficult to consider the entire expression. Consider the expression `1 + (2 + x)`, which we would ideally want to simplify as `3 + x`. Since the literal constituents of the addition are not immediately next to each other in the tree, it makes it extraordinarily difficult to design a rule-set to handle this. 

The solutions to these problems are two-fold; though only the first of which have been implemented in code as of yet. 

Problem #3 can be solved by allowing certain function calls to have arguments exceeding its arity, and to then collect all arguments of nested function calls of the same function inside the top-level function call, thereby *flattening* it (see [engine.cpp → flatten()](src/engine/engine.cpp#L17) for an implementation of this algorithm). For example: `1 + (2 + x)` may flattened to a single function call of addition containing three arguments: `+(1, 2, x)`. 

As for the first two problems, an algorithm I've been playing around with consists of the following steps:

1. Record which expressions match against each predicate in a function call; these are the *candidates* for the predicates. These candidates can be represented as bit-sets, where a bit set at index `i` denotes that the expression at index `i` in the arguments matches the predicate.
2. Perform local conflict resolution:
    - If any predicate has `0` matching expressions, the match failed.
    - If any predicate has exactly `1` matching expression, it is marked as successfully matched. 
    - If `n` predicates share the same `n` candidates, each is marked as successfully matched. Note that we should not decide what predicate is paired with what expression in this case, instead we simply declare that it would be possible to make such a pairing successfully. (This step is really a generalization of the previous step, but is included separately for clarity.) 
    - Once a predicate is successfully matched, each of its candidates must be removed as candidates from all not-as-of-yet-matched predicates in the function call. 
3. Once the match passes the local conflict resolution, each tagged predicate should have a list of candidates that it may be matched to. These can then be aggregated globally from all nested tagged predicates, and the intersection between all of them contain the viable candidates for each tag; the ones all tag-instances "agree upon". If the intersection has no candidates for a tag, the match failed. Otherwise, the tag-value may be chosen arbitrarily from each list of candidates. 

There are still some kinks to sort out with this algorithm, but the implementation should not be too difficult once this is done. 


# Parsing

The [parser(.h)](src/parser/parser.h) is a simple parser using the precedence climbing method, as detailed on [Wikipedia](https://en.wikipedia.org/wiki/Operator-precedence_parser). To aid the parser, there is a [lexer(.h)](src/parser/lexer.h) defined, which reads from an input string and separates out sub-strings based on the types of characters contained in them. The tokens produced can then be read in a stream-like manner and further processed into the AST.

The functions and operators recognized by the program are:


| Identifier | Arity | Priority | Description                       |
| -----------| ------| ---------| ----------------------------------|
| `-`        | 1     | L1       | Arithmetic negation operator      |
| `!`        | 1     | L1       | Boolean negation operator         |
| `**`       | 2     | L1       | Power operator                    |
| `*`        | 2     | L2       | Multiplication operator           |
| `/`        | 2     | L2       | Division operator                 |
| `%`        | 2     | L2       | Modulo operator                   |
| `+`        | 2     | L3       | Addition operator                 |
| `-`        | 2     | L3       | Subtraction operator              |
| `==`       | 2     | L4       | Equality operator                 |
| `!=`       | 2     | L4       | Non-equality operator             |
| `<`        | 2     | L4       | Less-than operator                |
| `<=`       | 2     | L4       | Less-than or equal-to operator    |
| `>`        | 2     | L4       | Greater-than operator             |
| `>=`       | 2     | L4       | Greater-than or equal-to operator |
| `&&`       | 2     | L5       | Logical AND operator              |
| `^^`       | 2     | L5       | Logical XOR operator              |
| `\|\|`     | 2     | L6       | Logical OR operator               |
| `sqrt`     | 1     | L1       | Square-root                       |
| `abs`      | 1     | L1       | Absolute value                    |
| `min`      | 2     | L1       | Lesser value                      |
| `max`      | 2     | L1       | Greater value                     |

New functions may be added by first adding it to the array of functions recognized by the parser in [function.h](src/ast/function.h), and then if the function should produce a result, adding an evaluation rule for it (which you cannot do yet, since it's not finished).

The parser supports function/operator overloading, based on the number of arguments, though the only implemented examples of this are the arithmetic negation operator and the subtraction operator which share the identifier `-`. 

Operators and functions are largely equivalent, the only real difference being that operators may be parsed according to their priority whereas functions have the de-facto priority of `L1`. If we were so inclined, we could call an operator with the function syntax, and a function with the operator syntax: 

```
> *(2, 3)
6

> 2 min 4
2
```

There is also syntactic sugar for multiplications defined if one parsing "unit" is placed right after another:

```
> 7x == 7 * x
true

> (x)(y) == x * y
true
```


# Building

CMake. C++20. Note that the project is making use of `std::format`, which may not be readily available in your standard library.
