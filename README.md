# CppParserCombinator

CppParserCombinator is a parser combinator library for C++14.

1. CppParserCombinator is inspired by [FParsec](http://www.quanttec.com/fparsec/)
1. Which in turn is inspired by [Parsec](https://wiki.haskell.org/Parsec)
1. Which in turn was inspired by the classic paper [Monadic Parser Combinators](http://www.cs.nott.ac.uk/~gmh/monparsing.pdf)

Parser combinators simplifies writing parsers for simple to semi-complex grammars.
Parser Combinators have similarities to RegEx but have the following benefits over RegEx:

1. More readable (once you understand them)
1. Produces values of the desired type (ie an int parser returns an integer)
1. Supports more complex grammars, for instance sub-expressions and operator precedence is simple to implement
1. Produces human-readable error messages "for free"

Example, a calculator grammar support able to parse expressions like: (x+y)*3 + z

```c++
// AST left out
auto satisfy_identifier = [] (std::size_t pos, char ch)
  {
    return
          (ch >= 'A' && ch <= 'Z'               )
      ||  (ch >= 'a' && ch <= 'z'               )
      ||  ((pos > 0) && ch >= '0' && ch <= '9'  )
      ;
  };

auto pexpr_trampoline = create_trampoline<expr::ptr> ();
auto pexpr            = ptrampoline<expr::ptr> (pexpr_trampoline);

auto psub_expr        = pbetween (pskip_char ('(') > pskip_ws, pexpr, pskip_char (')'));
auto pint_expr        =
      pint
  >=  [] (int v) { return preturn (int_expr::create (v)); }
  ;
auto pidentifier_expr =
      psatisfy ("identifier", 1, SIZE_MAX, satisfy_identifier)
  >=  [] (sub_string ss) { return preturn (identifier_expr::create (ss.str ())); }
  ;
auto pvalue_expr      = pchoice (pint_expr, pidentifier_expr, psub_expr) > pskip_ws;
auto p0_op =
      pany_of ("*/%")
  >   pskip_ws
  ;
auto pop0_expr        = psep (pvalue_expr , p0_op , binary_expr::create);
auto p1_op =
      pany_of ("+-")
  >   pskip_ws
  ;
auto pop1_expr        = psep (pop0_expr   , p1_op , binary_expr::create);

auto pcalculator_expr = [] ()
{
  pexpr_trampoline->trampoline = pop1_expr.parser_function;
  return pskip_ws < pexpr > peos;
} ();

```
