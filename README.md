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

A slightly more interesting example is a JSON parser:

```c++
// JSON specification: http://json.org/
auto parray_trampoline  = create_trampoline<json_ast::ptr> ();
auto parray             = ptrampoline<json_ast::ptr> (parray_trampoline);

auto pobject_trampoline = create_trampoline<json_ast::ptr> ();
auto pobject            = ptrampoline<json_ast::ptr> (pobject_trampoline);

auto pnchar   = psatisfy_char ("char", satisfy_char);
auto pescaped = pskip_char ('\\') < pmap (pany_of ("\"\\/bfnrt"), map_escaped);
// TODO: Handle unicode escaping (\u)
auto pchar    = pchoice (pnchar, pescaped);
auto pchars   = pbetween (pskip_char ('"'), pmany_char (pchar), pskip_char ('"'));
auto pstring  = pmap (pchars, json_string::create);

auto pfrac    = popt (pskip_char ('.') < praw_uint64);
auto psign    = popt (pany_of ("+-"));
auto pexp     = popt (pany_of ("eE") < ptuple (psign, pint));
// TODO: Handle that 0123 is not allowed
auto pnumber  = pmap (ptuple (popt (pskip_char ('-')), puint64, pfrac, pexp), map_number);

auto ptrue    = pskip_string ("true")   < preturn (json_true_value);

auto pfalse   = pskip_string ("false")  < preturn (json_false_value);

auto pnull    = pskip_string ("null")   < preturn (json_null_value);

auto pvalue   = pchoice (pstring, pnumber, ptrue, pfalse, pnull, parray, pobject) > pskip_ws;

auto pvalues  = pmany_sepby (pvalue, pskip_char (',') > pskip_ws);
auto parray_  = pmap (pbetween (pskip_char ('[') > pskip_ws, pvalues, pskip_char (']') > pskip_ws), json_array::create);

auto pmember  = ptuple (pchars > pskip_ws > pskip_char (':') > pskip_ws, pvalue);
auto pmembers = pmany_sepby (pmember, pskip_char (',') > pskip_ws);
auto pobject_ = pmap (pbetween (pskip_char ('{') > pskip_ws, pmembers, pskip_char ('}') > pskip_ws), json_object::create);

auto pjson = [] ()
{
  parray_trampoline->trampoline   = parray_.parser_function;
  pobject_trampoline->trampoline  = pobject_.parser_function;
  return pskip_ws < pchoice (parray, pobject) > pskip_ws > peos;
} ();
```
