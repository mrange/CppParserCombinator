#include "stdafx.h"

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>

#include "cpp_pc/pc.hpp"

#define TEST_EQ(expected, actual) test_eq (__FILE__, __LINE__, __FUNCTION__, #expected, expected, #actual, actual)

namespace test_parser
{
  using namespace cpp_pc;

  std::ostream & operator << (std::ostream & o, unit_type const &)
  {
    o
      << "unit"
      ;
    return o;
  }

  namespace detail
  {
    template<std::size_t i, std::size_t sz>
    struct print_tuple
    {
      template<typename... TArgs>
      static void print (std::ostream & o, std::tuple<TArgs...> const & v)
      {
        o
          << ", "
          << std::get<i> (v)
          ;

        print_tuple<i + 1, sz>::print (o, v);
      }
    };

    template<std::size_t sz>
    struct print_tuple<sz, sz>
    {
      template<typename... TArgs>
      static void print (std::ostream &, std::tuple<TArgs...> const &)
      {
      }
    };
  }

  std::ostream & operator << (std::ostream & o, std::tuple<> const &)
  {
    o
      << "()"
      ;
    return o;
  }

  template<typename... TArgs>
  std::ostream & operator << (std::ostream & o, std::tuple<TArgs...> const & v)
  {
    o
      << "("
      << std::get<0> (v)
      ;

    detail::print_tuple<1, sizeof... (TArgs)>::print (o, v);

    o
      << ")"
      ;
    return o;
  }

  template<typename TValue>
  std::ostream & operator << (std::ostream & o, opt<TValue> const & v)
  {
    if (v)
    {
      o
        << "Some "
        << v.get ()
        ;
    }
    else
    {
      o
        << "None"
        ;
    }
    return o;
  }

  template<typename TValue>
  std::ostream & operator << (std::ostream & o, result<TValue> const & v)
  {
    o
      << "{result: "
      << v.position
      << ", "
      << v.value
      << "}"
      ;
    return o;
  }

  template<typename TExpected, typename TActual>
  bool test_eq (
      char const *      file
    , int               line_no
    , char const *      // func
    , char const *      expected_name
    , TExpected const & expected
    , char const *      actual_name
    , TActual const &   actual
    )
  {
    if (!(expected == actual))
    {
      std::cout
        << file << "(" << line_no << "): EQ - "
        << expected_name << "(" << expected << ")"
        << " == "
        << actual_name << "(" << actual << ")"
        << std::endl
        ;
      return false;
    }

    return true;
  }

  template<typename T>
  void test_opt (T const & one, T const & two)
  {
    {
      opt<T> empty;
      TEST_EQ (true, empty.is_empty ());
    }


    {
      opt<T> empty = empty_opt;
      TEST_EQ (true, empty.is_empty ());
    }

    {
      opt<T> o { one };
      if (TEST_EQ (false, o.is_empty ()))
      {
        TEST_EQ (one, o.get ());
      }
    }

    {
      opt<T> o { T (one) };
      if (TEST_EQ (false, o.is_empty ()))
      {
        TEST_EQ (one, o.get ());
      }
    }

    {
      opt<T> o;
      opt<T> c = o;
      TEST_EQ (true, o.is_empty ());
      TEST_EQ (true, c.is_empty ());
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o;
      opt<T> c = std::move (o);
      TEST_EQ (true, o.is_empty ());
      TEST_EQ (true, c.is_empty ());
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o;
      opt<T> c;
      c = o;
      TEST_EQ (true, o.is_empty ());
      TEST_EQ (true, c.is_empty ());
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o;
      opt<T> c;
      c = std::move (o);
      TEST_EQ (true, o.is_empty ());
      TEST_EQ (true, c.is_empty ());
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o { one };
      opt<T> c = o;
      if (TEST_EQ (false, o.is_empty ()))
      {
        TEST_EQ (one, o.get ());
      }
      if (TEST_EQ (false, c.is_empty ()))
      {
        TEST_EQ (one, c.get ());
      }
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o { one };
      opt<T> c = std::move (o);
      TEST_EQ (true, o.is_empty ());
      if (TEST_EQ (false, c.is_empty ()))
      {
        TEST_EQ (one, c.get ());
      }
      TEST_EQ (true, o != c);
    }

    {
      opt<T> o { one };
      opt<T> c;
      c = o;
      if (TEST_EQ (false, o.is_empty ()))
      {
        TEST_EQ (one, o.get ());
      }
      if (TEST_EQ (false, c.is_empty ()))
      {
        TEST_EQ (one, c.get ());
      }
      TEST_EQ (true, o == c);
    }

    {
      opt<T> o { one };
      opt<T> c;
      c = std::move (o);
      TEST_EQ (true, o.is_empty ());
      if (TEST_EQ (false, c.is_empty ()))
      {
        TEST_EQ (one, c.get ());
      }
      TEST_EQ (true, o != c);
    }
  }

  void test_parser ()
  {
    std::string input = "1234 + 5678";
    auto error = std::make_shared<expected_error> ("expected");

    {
      auto p =
            preturn (3)
        ;
      result<int> expected  = success (0, 3);
      result<int> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            preturn (3)
        >=  [] (int v) { return preturn (std::make_tuple (v, 4)); }
        ;
      result<std::tuple<int,int>> expected  = success (0, std::make_tuple (3, 4));
      result<std::tuple<int,int>> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            psatisfy ("digits", 1U, 10U, satisfy_digit)
        >=  [] (auto && v) { return preturn (v.str ()); }
        ;
      result<std::string> expected  = success (4, std::string ("1234"));
      result<std::string> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        ;
      result<unit_type> expected  = success (1, unit);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('2')
        ;
      result<unit_type> expected  = failure<unit_type> (0, error);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        >   pskip_char ('2')
        ;
      result<unit_type> expected  = success (2, unit);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        >   pskip_char ('1')
        ;
      result<unit_type> expected  = failure<unit_type> (1, error);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('2')
        ;
      result<unit_type> expected  = success (2, unit);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('1')
        ;
      result<unit_type> expected  = failure<unit_type> (1, error);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pint ()
        ;
      result<int> expected  = success (4, 1234);
      result<int> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_ws ()
        ;
      result<unit_type> expected  = success (0, unit);
      result<unit_type> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pint ()
        >   pskip_ws ()
        >   pskip_char ('+')
        >   pskip_ws ()
        >=  [] (int v) { return pint () >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> expected  = success (11, std::make_tuple (1234, 5678));
      result<std::tuple<int,int>> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pint ()
        >   pskip_ws ()
        >   pskip_char ('-')
        >   pskip_ws ()
        >=  [] (int v) { return pint () >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> expected  = failure<std::tuple<int,int>> (5, error);
      result<std::tuple<int,int>> actual    = parse (p, input);
      TEST_EQ (expected, actual);
    }

    // TODO:
    // pchoice
    // psatisfy_char
    // ptrampoline
    // pbreakpoint
    // pbetween
    // psep
    // peos
  }
}

namespace calculator
{
  using namespace cpp_pc;

  using variables = std::map<std::string, int>;

  struct expr
  {
    using ptr = std::shared_ptr<expr>;

    expr ()                         = default;
    expr (expr const &)             = delete ;
    expr (expr &&)                  = delete ;
    expr & operator = (expr const &)= delete ;
    expr & operator = (expr &&)     = delete ;
    virtual ~expr ()                = default;

    virtual void  build_string (std::ostream & o) const = 0;
    virtual int   eval (variables const & vs) const     = 0;
  };

  struct int_expr : expr
  {
    int_expr (int v)
      : value (std::move (v))
    {
    }

    void build_string (std::ostream & o) const override
    {
      o << value;
    }

    int eval (variables const & vs) const override
    {
      return value;
    }

    int value;

    static expr::ptr create (int v)
    {
      return std::make_shared<int_expr> (v);
    }

  };

  struct identifier_expr : expr
  {
    identifier_expr (std::string id)
      : id (std::move (id))
    {
    }

    void build_string (std::ostream & o) const override
    {
      o << id;
    }

    std::string id;

    int eval (variables const & vs) const override
    {
      auto find = vs.find (id);
      if (find != vs.end ())
      {
        return find->second;
      }
      else
      {
        // TODO: Error handler
        return 0;
      }
    }

    static expr::ptr create (std::string id)
    {
      return std::make_shared<identifier_expr> (std::move (id));
    }

  };

  struct binary_expr : expr
  {
    binary_expr (expr::ptr left, char op, expr::ptr right)
      : left  (std::move (left))
      , op    (std::move (op))
      , right (std::move (right))
    {
    }

    void build_string (std::ostream & o) const override
    {
      o
        << '('
        ;

      left->build_string (o);

      o
        << ' '
        << op
        << ' '
        ;

      right->build_string (o);

      o
        << ')'
        ;
    }

    int eval (variables const & vs) const override
    {
      auto l = left->eval (vs);
      auto r = right->eval (vs);
      switch (op)
      {
      default:
        // Error handling
        return 0;
      case '+':
        return l + r;
      case '-':
        return l - r;
      case '*':
        return l * r;
      case '/':
        return l / r;
      case '%':
        return l % r;
      }
    }

    expr::ptr left  ;
    char      op    ;
    expr::ptr right ;

    static expr::ptr create (expr::ptr left, char op, expr::ptr right)
    {
      return std::make_shared<binary_expr> (std::move (left), std::move (op), std::move (right));
    }

  };

  auto satisfy_identifier = [] (std::size_t pos, char ch)
    {
      return
            (ch >= 'A' && ch <= 'Z'               )
        ||  (ch >= 'a' && ch <= 'z'               )
        ||  ((pos > 0) && ch >= '0' && ch <= '9'  )
        ;
    };

  auto satisfy_mullike_op = [] (std::size_t, char ch)
    {
      return
            ch == '*'
        ||  ch == '/'
        ||  ch == '%'
        ;
    };

  auto satisfy_addlike_op = [] (std::size_t, char ch)
    {
      return
            ch == '+'
        ||  ch == '-'
        ;
    };

  auto pfull_trampoline = create_trampoline_payload<expr::ptr> ();
  auto pfull_expr       = ptrampoline<expr::ptr> (pfull_trampoline);

  auto psub_expr        = pbetween (pskip_char ('(') > pskip_ws (), pfull_expr, pskip_char (')'));
  auto pint_expr        =
        pint ()
    >=  [] (int v) { return preturn (int_expr::create (v)); }
    ;
  auto pidentifier_expr =
        psatisfy ("identifier", 1, SIZE_MAX, satisfy_identifier)
    >=  [] (sub_string ss) { return preturn (identifier_expr::create (ss.str ())); }
    ;
  auto pmullike_op =
        psatisfy_char ("'*', '/'", satisfy_mullike_op)
    >   pskip_ws ()
    ;
  auto paddlike_op =
        psatisfy_char ("'+', '-'", satisfy_addlike_op)
    >   pskip_ws ()
    ;
  auto pvalue_expr      = pchoice (pint_expr, pidentifier_expr, psub_expr) > pskip_ws ();
  auto pop0_expr        = psep (pvalue_expr , pmullike_op, binary_expr::create);
  auto pop1_expr        = psep (pop0_expr   , paddlike_op, binary_expr::create);

  struct init_parser
  {
    init_parser ()
    {
      pfull_trampoline->trampoline = pop1_expr.parser_function;
    }
  };

  auto pcalculator_expr = pfull_expr > peos ();

  init_parser init;

  variables const vars 
  {
    {"x"  , 3},
    {"y"  , 5},
  };

  void parse_and_print (const std::string & input)
  {
    auto r = parse (pcalculator_expr, input);
    if (r.value)
    {
      auto expr = r.value.get ();
      auto v = expr->eval (vars);

      std::stringstream o;
      expr->build_string (o);

      std::cout
        << "Parsed: " << input << std::endl
        << "  as  : " << o.str () << std::endl
        << "  eval: " << v << std::endl
        ;
    }
    else
    {
      std::cout
        << "Failed to parse: "
        << input
        << std::endl
        ;
    }
  }

  void test_calculator ()
  {
    std::cout << "Variables:" << std::endl;
    for (auto && kv : vars)
    {
      std::cout << "  " << kv.first << " = " << kv.second << std::endl;
    }
    parse_and_print ("1234");
    parse_and_print ("abc");
    parse_and_print ("(0 + 3) * x + 4*y - 1");

    std::cout << "Input expression (blank to exit)" << std::endl;
    std::string line;
    while (std::getline (std::cin, line) && !line.empty ())
    {
      parse_and_print (line);
    }

  }

}

int main()
{
  std::cout << "Running tests..." << std::endl;
  test_parser::test_opt<std::string> ("1234", "5678");
  test_parser::test_opt<int> (1,3);
  test_parser::test_parser ();
  calculator::test_calculator ();
  std::cout << "Done!" << std::endl;
  return 0;
}

