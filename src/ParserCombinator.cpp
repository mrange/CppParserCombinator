// ----------------------------------------------------------------------------
// Copyright 2015 Mårten Rånge
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------
#include "stdafx.h"
// ----------------------------------------------------------------------------
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>
// ----------------------------------------------------------------------------
#include "cpp_pc/pc.hpp"
// ----------------------------------------------------------------------------
#define TEST_EQ(expected, actual) test_eq (__FILE__, __LINE__, __FUNCTION__, #expected, expected, #actual, actual)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
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
  std::ostream & operator << (std::ostream & o, std::vector<TValue> const & vs)
  {
    o
      << "[("
      << vs.size ()
      << ")"
      ;

    for (auto && v : vs)
    {
      o
        << ", "
        << v
        ;
    }

    o
      << "]"
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
  void test_opt (T const & one, T const & /*two*/)
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
    std::string const input = "1234 + 5678";

    {
      auto p =
            preturn (3)
        ;
      result<int> expected  = result<int>::success (0, 3);
      result<int> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            preturn (3)
        >=  [] (int v) { return preturn (std::make_tuple (v, 4)); }
        ;
      result<std::tuple<int,int>> expected  = result<std::tuple<int,int>>::success (0, std::make_tuple (3, 4));
      result<std::tuple<int,int>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            preturn (3) < punit
        ;
      result<unit_type> expected  = result<unit_type>::success (0, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            psatisfy ("digits", 1U, 10U, satisfy_digit)
        >=  [] (auto && v) { return preturn (v.str ()); }
        ;
      result<std::string> expected  = result<std::string>::success (4, std::string ("1234"));
      result<std::string> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        ;
      result<unit_type> expected  = result<unit_type>::success (1, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('2')
        ;
      result<unit_type> expected  = result<unit_type>::failure (0);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        >   pskip_char ('2')
        ;
      result<unit_type> expected  = result<unit_type>::success (2, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        >   pskip_char ('1')
        ;
      result<unit_type> expected  = result<unit_type>::failure (1);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('2')
        ;
      result<unit_type> expected  = result<unit_type>::success (2, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('1')
        ;
      result<unit_type> expected  = result<unit_type>::failure (1);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    // Extend pint test to test +/-
    {
      auto p =
            pint
        ;
      result<int> expected  = result<int>::success (4, 1234);
      result<int> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_ws
        ;
      result<unit_type> expected  = result<unit_type>::success (0, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_string ("123")
        ;
      result<unit_type> expected  = result<unit_type>::success (3, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_string ("124")
        ;
      result<unit_type> expected  = result<unit_type>::failure (2);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_string ("123")
        <   preturn (true)
        ;
      result<bool> expected  = result<bool>::success (3, true);
      result<bool> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_string ("124")
        <   preturn (true)
        ;
      result<bool> expected  = result<bool>::failure (2);
      result<bool> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('1')
        ;
      result<unit_type> expected  = result<unit_type>::success (1, unit);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pskip_char ('2')
        ;
      result<unit_type> expected  = result<unit_type>::failure (0);
      result<unit_type> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pmany (1, 5, pany_of ("123"))
        ;
      result<std::vector<char>> expected  = result<std::vector<char>>::success (3, std::vector<char> ({'1','2','3'}));
      result<std::vector<char>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pmany (1, 5, pany_of ("456"))
        ;
      result<std::vector<char>> expected  = result<std::vector<char>>::failure (0);
      result<std::vector<char>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pmany (1, 2, pany_of ("123"))
        ;
      result<std::vector<char>> expected  = result<std::vector<char>>::success (2, std::vector<char> ({'1','2'}));
      result<std::vector<char>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pmany (5, 5, pany_of ("123"))
        ;
      result<std::vector<char>> expected  = result<std::vector<char>>::failure (3);
      result<std::vector<char>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            ptuple (pskip_char ('1'), pskip_char ('2') < preturn (1))
        ;
      result<std::tuple<unit_type, int>> expected = result<std::tuple<unit_type, int>>::success (2, std::make_tuple (unit, 1));
      result<std::tuple<unit_type, int>> actual   = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            ptuple (pskip_char ('2'), pskip_char ('1') < preturn (1))
        ;
      result<std::tuple<unit_type, int>> expected = result<std::tuple<unit_type, int>>::failure (0);
      result<std::tuple<unit_type, int>> actual   = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            ptuple (pskip_char ('1'), pskip_char ('1') < preturn (1))
        ;
      result<std::tuple<unit_type, int>> expected = result<std::tuple<unit_type, int>>::failure (1);
      result<std::tuple<unit_type, int>> actual   = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            popt (pskip_char ('1'))
        ;
      result<opt<unit_type>> expected  = result<opt<unit_type>>::success (1, make_opt (unit));
      result<opt<unit_type>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            popt (pskip_char ('2'))
        ;
      result<opt<unit_type>> expected  = result<opt<unit_type>>::success (0, opt<unit_type> ());
      result<opt<unit_type>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pint
        >   pskip_ws
        >   pskip_char ('+')
        >   pskip_ws
        >=  [] (int v) { return pint >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> expected  = result<std::tuple<int,int>>::success (11, std::make_tuple (1234, 5678));
      result<std::tuple<int,int>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    {
      auto p =
            pint
        >   pskip_ws
        >   pskip_char ('-')
        >   pskip_ws
        >=  [] (int v) { return pint >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> expected  = result<std::tuple<int,int>>::failure (5);
      result<std::tuple<int,int>> actual    = plain_parse (p, input);
      TEST_EQ (expected, actual);
    }

    // TODO:
    // pbreakpoint
    // pchoice
    // ptrampoline
    // pbetween
    // psep
    // peos
    // pskip_satisfy
    // psatisfy_char
  }
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
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
    int value;

    int_expr (int v)
      : value (std::move (v))
    {
    }

    void build_string (std::ostream & o) const override
    {
      o << value;
    }

    int eval (variables const &) const override
    {
      return value;
    }

    static expr::ptr create (int v)
    {
      return std::make_shared<int_expr> (v);
    }
  };

  struct identifier_expr : expr
  {
    std::string id;

    identifier_expr (std::string id)
      : id (std::move (id))
    {
    }

    void build_string (std::ostream & o) const override
    {
      o << id;
    }

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

    static expr::ptr create (sub_string const & s)
    {
      return std::make_shared<identifier_expr> (s.str ());
    }
  };

  struct binary_expr : expr
  {
    expr::ptr left  ;
    char      op    ;
    expr::ptr right ;

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

    static expr::ptr create (expr::ptr left, char op, expr::ptr right)
    {
      return std::make_shared<binary_expr> (std::move (left), std::move (op), std::move (right));
    }
  };

  auto pcalculator_expr = [] ()
  {
    auto satisfy_identifier = [] (std::size_t pos, char ch)
      {
        return
              (ch >= 'A' && ch <= 'Z'               )
          ||  (ch >= 'a' && ch <= 'z'               )
          ||  ((pos > 0) && ch >= '0' && ch <= '9'  )
          ;
      };

    auto pidentifier_expr = pmap (psatisfy ("identifier", 1, SIZE_MAX, satisfy_identifier), identifier_expr::create);

    auto pint_expr        = pmap (pint, int_expr::create);

    auto pexpr_trampoline = create_trampoline<expr::ptr> ();
    auto pexpr            = ptrampoline<expr::ptr> (pexpr_trampoline);
    auto psub_expr        = pbetween (pskip_char ('(') > pskip_ws, pexpr, pskip_char (')'));

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

    pexpr_trampoline->trampoline = pop1_expr.parser_function;
    return pskip_ws < pexpr > peos;
  } ();

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
      std::cout << r.message << std::endl;
    }
  }

  void test_calculator ()
  {
    std::cout << "Variables:" << std::endl;
    for (auto && kv : vars)
    {
      std::cout << "  " << kv.first << " = " << kv.second << std::endl;
    }

    /*
    parse_and_print ("1234");
    parse_and_print ("abc");
    parse_and_print ("(0 + 3) * x + 4*y - 1");
    parse_and_print ("1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+\r\n6+7+8+9+0+1+2+3+4+5+6+7+8+9");
    parse_and_print ("1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+6+7+8+9+1+2+3+4+5+6+7+8+9+0+1+2+3+4+5+\r\n6+7+8+9+0+)1+2+3+4+5+6+7+8");
    */

    std::string line;
    while (std::cout << "Input expression (blank to exit)" << std::endl, std::getline (std::cin, line) && !line.empty ())
    {
      parse_and_print (line);
    }

  }

}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
namespace json
{
  using namespace cpp_pc;

  struct json_ast
  {
    using ptr = std::shared_ptr<json_ast> ;
    json_ast ()                             = default;
    json_ast (json_ast const &)             = delete ;
    json_ast (json_ast &&)                  = delete ;
    json_ast & operator = (json_ast const &)= delete ;
    json_ast & operator = (json_ast &&)     = delete ;
    virtual ~json_ast ()                    = default;

    virtual void build_string (std::ostream & o) const = 0;
    virtual bool is_equal_to (json_ast::ptr const & o) const = 0;
  };

  std::string to_string (json_ast::ptr const & json)
  {
    std::stringstream ss;

    if (json)
    {
      json->build_string (ss);
    }
    else
    {
      ss << "[]";
    }

    return ss.str ();
  }

  struct json_null : json_ast
  {
    json_null () = default;

    void build_string (std::ostream & o) const override
    {
      o << "null";
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_null> (o);

      return !!other;
    }

    static json_ast::ptr create ()
    {
      return std::make_shared<json_null> ();
    }
  };

  struct json_boolean : json_ast
  {
    using value_type = bool;

    json_boolean (value_type v)
      : value (v)
    {
    }

    value_type const value;

    void build_string (std::ostream & o) const override
    {
      o << (value ? "true" : "false");
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_boolean> (o);
      if (!other)
      {
        return false;
      }

      return value == other->value;
    }

    static json_ast::ptr create (value_type v)
    {
      return std::make_shared<json_boolean> (v);
    }
  };

  struct json_number : json_ast
  {
    using value_type = double;

    json_number (value_type v)
      : value (v)
    {
    }

    value_type const value;

    void build_string (std::ostream & o) const override
    {
      o << value;
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_number> (o);
      if (!other)
      {
        return false;
      }

      return value == other->value;
    }

    static json_ast::ptr create (value_type v)
    {
      return std::make_shared<json_number> (v);
    }
  };

  struct json_string : json_ast
  {
    using value_type = std::string;

    json_string (value_type v)
      : value (std::move (v))
    {
    }

    value_type const value;

    void build_string (std::ostream & o) const override
    {
      // TODO: Add escaping
      o << '"' << value << '"';
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_string> (o);
      if (!other)
      {
        return false;
      }

      return value == other->value;
    }

    static json_ast::ptr create (value_type v)
    {
      return std::make_shared<json_string> (std::move (v));
    }
  };

  struct json_array : json_ast
  {
    using value_type = std::vector<json_ast::ptr>;

    json_array (value_type v)
      : value (std::move (v))
    {
    }

    value_type const value;

    void build_string (std::ostream & o) const override
    {
      auto prepend = "";
      o << '[';

      for (auto && v : value)
      {
        o << prepend;
        if (v)
        {
          v->build_string (o);
        }
        else
        {
          o << "null";
        }
        prepend = ", ";
      }

      o << ']';
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_array> (o);
      if (!other)
      {
        return false;
      }

      return
        std::equal (
            value.begin ()
          , value.end ()
          , other->value.begin ()
          , other->value.end ()
          , [] (auto && l, auto && r) -> bool
          {
            if (!l || !r)
            {
              return !l && !r;
            }

            return l->is_equal_to (r);
          });
    }

    static json_ast::ptr create (value_type v)
    {
      return std::make_shared<json_array> (std::move (v));
    }
  };

  struct json_object : json_ast
  {
    using value_type = std::vector<std::tuple<std::string, json_ast::ptr>>;

    json_object (value_type v)
      : value (std::move (v))
    {
    }

    value_type const value;

    void build_string (std::ostream & o) const override
    {
      auto prepend = "";
      o << '{';

      for (auto && kv : value)
      {
        auto & k = std::get<0> (kv);
        auto & v = std::get<1> (kv);

        o << prepend;

        // TODO: Add escaping
        o << '"' << k << '"' << ':';

        if (v)
        {
          v->build_string (o);
        }
        else
        {
          o << "null";
        }

        prepend = ", ";
      }

      o << '}';
    }

    bool is_equal_to (json_ast::ptr const & o) const override
    {
      auto other = std::dynamic_pointer_cast<json_object> (o);
      if (!other)
      {
        return false;
      }

      return
        std::equal (
            value.begin ()
          , value.end ()
          , other->value.begin ()
          , other->value.end ()
          , [] (auto && l, auto && r) -> bool
          {
            auto lk = std::get<0> (l);
            auto rk = std::get<0> (r);
            auto lv = std::get<1> (l);
            auto rv = std::get<1> (r);

            if (lk != rk)
            {
              return false;
            }

            if (!lv || !rv)
            {
              return !lv && !rv;
            }

            return lv->is_equal_to (rv);
          });
    }

    static json_ast::ptr create (value_type v)
    {
      return std::make_shared<json_object> (std::move (v));
    }
  };

  auto satisfy_char = [] (std::size_t, char ch)
    {
      return ch != '"' && ch != '\\';
    };
  auto map_escaped = [] (char ch)
    {
      switch (ch)
      {
      case '"':
        return '"';
      case '\\':
        return '\\';
      case '/':
        return '/';
      case 'b':
        return '\b';
      case 'f':
        return '\f';
      case 'n':
        return '\n';
      case 'r':
        return '\r';
      case 't':
        return '\t';
      default:
        CPP_PC__ASSERT (false);
        return ch;
      };
    };
  auto map_number = [] (auto && v)
    {
      auto calculate_fraction = [] (auto && frac)
        {
          auto i = static_cast<double> (std::get<0> (frac));
          auto s = std::get<1> (frac);
          return i / std::pow (10.0, s);
        };

      auto calculate_exponent = [] (auto && exp)
        {
          auto sign   = (std::get<0> (exp).coalesce ('+') == '+') ? 1.0 : -1.0;
          auto e      = std::get<1> (exp);
          return std::pow (10.0, sign*e);
        };

      auto sign = std::get<0> (v) ? -1.0 : 1.0;
      auto i    = static_cast<double> (std::get<1> (v));
      auto ofrac= std::get<2> (v);
      auto frac =
          ofrac
        ? calculate_fraction (ofrac.get ())
        : 0.0
        ;
      auto oexp = std::get<3> (v);
      auto exp =
          oexp
        ? calculate_exponent (oexp.get ())
        : 1.0
        ;

      auto result = sign * (i + frac) * exp;

      return json_number::create (result);
    };

  auto json_null_value  = json_null::create ();
  auto json_true_value  = json_boolean::create (true);
  auto json_false_value = json_boolean::create (false);

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

  void parse_and_print (const std::string & input)
  {
    auto r = parse (pjson, input);
    if (r.value)
    {

      auto v = to_string (r.value.get ());
      //auto s = std::string (v.begin (), v.end ());
      std::cout
        << input
        << " : "
        << v
        << std::endl
        ;
    }
    else
    {
      std::cout << r.message << std::endl;
    }
  }

  int next (std::mt19937 & random, int from, int to)
  {
    std::uniform_int_distribution<int> d (from, to);
    return d (random);
  }

  auto string_size  = 10;
  auto array_size   = 10;
  auto object_size  = 10;
  auto max_level    = 4;

  std::string generate_string (std::mt19937 & random)
  {
    std::string result;

    auto sz = next (random, 0, string_size);
    result.reserve (sz);
    for (auto iter = 0; iter < sz; ++iter)
    {
      result.push_back (static_cast<char> (next (random, 65, 90)));
    }

    return result;
  };

  json_ast::ptr generate_ast (std::mt19937 & random, int level)
  {
    auto min = 0;
    auto max = 10;

    if (level == 0)
    {
      // Only allows array/object
      max = 1;
    }
    else if (level > max_level)
    {
      // Disallows array/object
      min = 2;
    }

    switch (next (random, min, max))
    {
    case 0:
      {
        json_array::value_type result;

        auto sz = next (random, 0, array_size);
        result.reserve (sz);
        for (auto iter = 0; iter < sz; ++iter)
        {
          result.push_back (generate_ast (random, level + 1));
        }

        return json_array::create (std::move (result));
      }
    case 1:
      {
        json_object::value_type result;

        auto sz = next (random, 0, object_size);
        result.reserve (sz);
        for (auto iter = 0; iter < sz; ++iter)
        {
          result.push_back (std::make_tuple (generate_string (random), generate_ast (random, level + 1)));
        }

        return json_object::create (std::move (result));
      }
    case 2:
      return json_null_value;
    case 3:
    case 4:
      return
        next (random, 0, 1) == 0
          ? json_true_value
          : json_false_value
          ;
    case 5:
    case 6:
    case 7:
      return json_number::create (next (random, -1000, 1000) / 4.0);
    case 8:
    case 9:
    case 10:
      return json_string::create (generate_string (random));
    default:
      return json_null_value;
    }
  }

  void test_json ()
  {
    std::mt19937 random (19740531);

    auto random_testcases = 100;

    std::cout << "Running " << random_testcases << " JSON testcases..." << std::endl;

    for (auto iter = 0; iter < random_testcases; ++iter)
    {
      auto gen    = generate_ast (random, 0);
      auto sgen   = to_string (gen);

//      std::cout << "-- " << std::endl << sgen << std::endl;

      auto r = parse (pjson, sgen);
      if (r.value)
      {
        auto parsed   = r.value.get ();
        auto sparsed  = to_string (parsed);

        if (!gen->is_equal_to (parsed))
        {
          std::cout

            << "ERROR: Parsed JSON not parsed correctly" << std::endl
            << "Original: " << sgen << std::endl
            << "Parsed  : " << sparsed << std::endl
            ;
        }
      }
      else
      {
        std::cout
          << "ERROR: Failed to parse '" << sgen  << "' with message: " << std::endl
          << r.message << std::endl
          ;
      }
    }

    std::cout << "Done!" << std::endl;

    /*
    parse_and_print ("[1.0g32]");
    parse_and_print ("[2,1.0g32]");
    parse_and_print ("2");
    parse_and_print ("[2.171828]");
    parse_and_print ("[]");
    parse_and_print ("[null]");
    parse_and_print ("{}");
    parse_and_print ("{,}");
    parse_and_print (R"({"x":3, "y":null})");
    */
  };
}
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
int main()
{
  std::cout << "Running unit tests..." << std::endl;
  test_parser::test_opt<std::string> ("1234", "5678");
  test_parser::test_opt<int> (1,3);
  test_parser::test_parser ();
  std::cout << "Unit tests complete" << std::endl;

  std::cout << "Running json tests..." << std::endl;
  json::test_json ();

  std::cout << "Running calculator tests..." << std::endl;
  calculator::test_calculator ();

  std::make_tuple(1,2);

  std::cout << "Done!" << std::endl;
  return 0;
}
// ----------------------------------------------------------------------------
