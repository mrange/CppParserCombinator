#include "stdafx.h"

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>

#define PARSER_PRELUDE  constexpr
#define PARSER_INLINE   inline

namespace parser
{

  namespace detail
  {
    template<typename T>
    using strip_type_t = std::remove_cv_t<std::remove_reference_t<T>>;
  }

  struct unit_type
  {
    PARSER_PRELUDE bool operator == (unit_type) const
    {
      return true;
    }
  };

  unit_type unit;

  struct empty_opt_type
  {
  };

  empty_opt_type empty_opt;

  template<typename TValue>
  struct opt
  {
    using value_type = TValue;

    PARSER_INLINE opt () noexcept
      : has_value (false)
    {
    }

    PARSER_INLINE opt (empty_opt_type) noexcept
      : has_value (false)
    {
    }

    PARSER_INLINE ~opt () noexcept
    {
      free_opt ();
    }

    PARSER_INLINE explicit opt (value_type const & v)
      : has_value (true)
    {
      new (&storage) value_type (v);
    }

    PARSER_INLINE explicit opt (value_type && v) noexcept
      : has_value (true)
    {
      new (&storage) value_type (std::move (v));
    }

    PARSER_INLINE opt (opt const & o)
      : has_value (o.has_value)
    {
      if (has_value)
      {
        new (&storage) value_type (o.get ());
      }
    }

    PARSER_INLINE opt (opt && o) noexcept
      : has_value (std::move (o.has_value))
    {
      o.has_value = false;
      if (has_value)
      {
        new (&storage) value_type (std::move (o.get ()));
      }
    }

    opt & operator = (opt const & o)
    {
      if (this == &o)
      {
        return *this;
      }

      opt copy (o);

      *this = std::move (copy);

      return *this;
    }

    opt & operator = (opt && o) noexcept
    {
      if (this == &o)
      {
        return *this;
      }

      free_opt ();

      has_value = std::move (o.has_value);
      o.has_value = false;

      if (has_value)
      {
        new (&storage) value_type (std::move (o.get ()));
      }

      return *this;
    }

    PARSER_PRELUDE bool operator == (opt const & o) const
    {
      return
          has_value && o.has_value
        ? get () == o.get ()
        : has_value == o.has_value
        ;
    }

    PARSER_PRELUDE bool operator != (opt const & o) const
    {
      return !(*this == o);
    }

    PARSER_PRELUDE explicit operator bool () const noexcept
    {
      return has_value;
    }

    PARSER_PRELUDE bool is_empty () const noexcept
    {
      return !has_value;
    }

    PARSER_PRELUDE value_type const & get () const noexcept
    {
      return *get_ptr ();
    }

    PARSER_INLINE value_type const & get () noexcept
    {
      return *get_ptr ();
    }

  private:

    PARSER_PRELUDE value_type const * get_ptr () const noexcept
    {
      return reinterpret_cast<value_type const *> (&storage);
    }

    PARSER_INLINE value_type * get_ptr () noexcept
    {
      return reinterpret_cast<value_type *> (&storage);
    }

    void free_opt () noexcept
    {
      if (has_value)
      {
        has_value = false;
        get ().~value_type ();
      }
    }

    alignas(value_type) unsigned char storage[sizeof (value_type)];
    bool has_value;
  };

  template<typename T>
  PARSER_PRELUDE auto make_opt (T && v)
  {
    return opt<detail::strip_type_t<T>> (std::forward<T> (v));
  }

  PARSER_PRELUDE int EOS = 0xFF000001;

  struct sub_string
  {
    PARSER_PRELUDE sub_string (char const * begin, char const * end)
      : begin   (begin)
      , end     (end)
    {
    }

    sub_string ()                                 = delete ;
    sub_string (sub_string const &)               = default;
    sub_string (sub_string &&)                    = default;
    sub_string & operator = (sub_string const &)  = default;
    sub_string & operator = (sub_string &&)       = default;
    ~sub_string ()                                = default;

    PARSER_INLINE std::string str () const
    {
      return std::string (begin, end);
    }

    PARSER_PRELUDE std::size_t size () const
    {
      return static_cast<std::size_t> (end - begin);
    }

    char const * const  begin   ;
    char const * const  end     ;
  };

  struct state
  {
    state ()                              = delete ;
    state (state const &)                 = delete ;
    state (state &&)                      = delete ;
    state & operator = (state const &)    = delete ;
    state & operator = (state &&)         = delete ;
    ~state ()                             = default;

    state (char const * begin, char const * end)
      : begin   (begin)
      , end     (end)
      , current (begin)
    {
    }

    PARSER_INLINE int peek ()
    {
      return
          current < end
        ? *current
        : EOS
        ;
    }

    PARSER_INLINE void advance ()
    {
      ++current;
    }

    template<typename TSatisfyFunction>
    PARSER_INLINE sub_string satisfy (TSatisfyFunction && satisfy_function)
    {
      auto start = current;

      for (
        ; current < end && satisfy_function (static_cast<std::size_t> (current - start), *current)
        ; ++current
        )
        ;

      return sub_string (start, current);
    }

  private:
    char const * const  begin   ;
    char const * const  end     ;
    char const *        current ;
  };

  struct empty_result_type
  {
  };

  empty_result_type empty_result;

  template<typename T>
  struct result
  {
    using value_type = T;

    result ()                             = default;
    result (result const &)               = default;
    result (result &&)                    = default;
    result & operator = (result const &)  = default;
    result & operator = (result &&)       = default;
    ~result ()                            = default;

    PARSER_PRELUDE result (empty_result_type)
    {
    }

    PARSER_PRELUDE explicit result (T const & o)
      : value (o)
    {
    }

    PARSER_PRELUDE explicit result (T && o) noexcept
      : value (std::move (o))
    {
    }

    bool operator == (result const & o) const
    {
      return value == o.value;
    }

    opt<T>  value;
  };

  template<typename TParser, typename TParserGenerator>
  PARSER_PRELUDE auto pbind (TParser && t, TParserGenerator && fu);

  template<typename TParser, typename TOtherParser>
  PARSER_PRELUDE auto pleft (TParser && t, TOtherParser && u);

  template<typename TParser, typename TOtherParser>
  PARSER_PRELUDE auto pright (TParser && t, TOtherParser && u);

  template<typename TValue, typename TParserFunction>
  struct parser
  {
    using parser_function_type  = TParserFunction;
    using value_type            = TValue;

    parser_function_type parser_function;
/*
    static_assert (
        std::is_same<result<value_type>, std::result_of_t<parser_function_type (state &)>::value
      , "Must return result<_>"
      );
*/
    PARSER_PRELUDE parser (parser_function_type const & parser_function)
      : parser_function (parser_function)
    {
    }

    PARSER_PRELUDE parser (parser_function_type && parser_function)
      : parser_function (std::move (parser_function))
    {
    }

    PARSER_PRELUDE result<value_type> operator () (state & s) const
    {
      return parser_function (s);
    }

    template<typename TBindFunction>
    PARSER_PRELUDE auto operator >= (TBindFunction && bind_function) const
    {
      return pbind (*this, std::forward<TBindFunction> (bind_function));
    };


    template<typename TParser>
    PARSER_PRELUDE auto operator > (TParser && parser) const
    {
      return pleft (*this, std::forward<TParser> (parser));
    };

    template<typename TParser>
    PARSER_PRELUDE auto operator < (TParser && parser) const
    {
      return pright (*this, std::forward<TParser> (parser));
    };

  };

  namespace detail
  {
    template<typename TParserFunction>
    PARSER_PRELUDE auto adapt_parser_function (TParserFunction && parser_function)
    {
      using parser_function_type  = strip_type_t<TParserFunction>                     ;
      using parser_result_type    = std::result_of_t<parser_function_type (state &)>  ;
      using value_type            = typename parser_result_type::value_type           ;

      return parser<value_type, parser_function_type> (std::forward<TParserFunction> (parser_function));
    }
  }

  template<typename T>
  PARSER_PRELUDE auto success (T && v)
  {
    return result<detail::strip_type_t<T>> (std::forward<T> (v));
  }

  template<typename T>
  PARSER_PRELUDE auto failure ()
  {
    return result<T> ();
  }

  template<typename TValueType, typename TParserFunction>
  PARSER_INLINE auto parse (parser<TValueType, TParserFunction> const & p, std::string const & i)
  {
    auto begin  = i.c_str ();
    auto end    = begin + i.size ();
    state s (begin, end);

    return p (s);
  }

  auto satisfy_digit = [] (std::size_t, char ch)
    {
      return ch >= '0' && ch <= '9';
    };

  auto satisfy_whitespace = [] (std::size_t, char ch)
    {
      switch (ch)
      {
      case ' ':
      case '\b':
      case '\n':
      case '\r':
      case '\t':
        return true;
      default:
        return false;
      }
    };

  // parser<'T> = state -> result<'T>

  template<typename TValue>
  PARSER_PRELUDE auto preturn (TValue && v)
  {
    return detail::adapt_parser_function (
      [v = std::forward<TValue> (v)] (state & s)
      {
        return success (v);
      });
  }

  template<typename TParser, typename TParserGenerator>
  PARSER_PRELUDE auto pbind (TParser && t, TParserGenerator && fu)
  {
    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), fu = std::forward<TParserGenerator> (fu)] (state & s)
      {
        auto tv = t (s);
        using result_type = decltype (fu (std::move (tv.value.get ())) (s));
        if (tv.value)
        {
          return fu (std::move (tv.value.get ())) (s);
        }
        else
        {
          return result_type ();
        }
      });
  }

  template<typename TParser, typename TOtherParser>
  PARSER_PRELUDE auto pleft (TParser && t, TOtherParser && u)
  {
    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), u = std::forward<TOtherParser> (u)] (state & s)
      {
        using result_type = decltype (t (s));

        auto tv = t (s);
        if (tv.value)
        {
          auto tu = u (s);
          if (tu.value)
          {
            return tv;
          }
          else
          {
            return result_type ();
          }
        }
        else
        {
          return tv;
        }
      });
  }

  template<typename TParser, typename TOtherParser>
  PARSER_PRELUDE auto pright (TParser && t, TOtherParser && u)
  {
    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), u = std::forward<TOtherParser> (u)] (state & s)
      {
        using result_type = decltype (u (s));

        auto tv = t (s);
        if (tv.value)
        {
          return u (s);
        }
        else
        {
          return result_type ();
        }
      });
  }

  template<typename TParser, typename TOtherParser>
  PARSER_PRELUDE auto pright (TParser && t, TOtherParser && u);

  template<typename TSatisfyFunction>
  PARSER_PRELUDE auto psatisfy (TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state & s)
      {
        return success (s.satisfy (satisfy_function));
      });
  }

  template<typename TSatisfyFunction>
  PARSER_PRELUDE auto pskip_satisfy (TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state & s)
      {
        s.satisfy (satisfy_function);
        return success (unit);
      });
  }

  PARSER_INLINE auto pskip_char (char ch)
  {
    return detail::adapt_parser_function (
      [ch] (state & s)
      {
        auto peek = s.peek ();
        if (peek == ch)
        {
          s.advance ();

          return success (unit);
        }
        else
        {
          return failure<unit_type> ();
        }
      });
  }

  PARSER_INLINE auto pskip_ws ()
  {
    return pskip_satisfy (satisfy_whitespace);
  }

  PARSER_INLINE auto pint ()
  {
    return detail::adapt_parser_function (
      [] (state & s)
      {
        auto ss = s.satisfy (satisfy_digit);
        if (ss.size () > 0)
        {
          auto i = 0;
          for (auto iter = ss.begin; iter != ss.end; ++iter)
          {
            i = 10*i + (*iter - '0');
          }
          return success (i);
        }
        else
        {
          return failure<int> ();
        }
      });
  }

  /*
  template<typename... TParsers>
  PARSER_PRELUDE auto ptuple (TParsers... parsers)
  {
    return detail::adapt_parser_function (
      [] (state & s)
      {
        auto results = parsers... (s);
        return failure<int> ();
      });
  }
  */

  // ---------------------------------------------------------------------------

  template<typename TExpected, typename TActual>
  bool test_eq (
      char const *      file
    , int               line_no
    , char const *      func
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

#define TEST_EQ(expected, actual) test_eq (__FILE__, __LINE__, __FUNCTION__, #expected, expected, #actual, actual)

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
      static void print (std::ostream & o, std::tuple<TArgs...> const & v)
      {
      }
    };
  }

  std::ostream & operator << (std::ostream & o, std::tuple<> const & v)
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
      << "result:"
      << v.value
      ;
    return o;
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

    {
      auto p =
            preturn (3)
        ;
      result<int> r = parse (p, input);
      TEST_EQ (success (3), r);
    }

    {
      auto p =
            preturn (3)
        >=  [] (int v) { return preturn (std::make_tuple (v, 4)); }
        ;
      result<std::tuple<int,int>> r = parse (p, input);
      TEST_EQ (success (std::make_tuple (3, 4)), r);
    }

    {
      auto p =
            psatisfy (satisfy_digit)
        >=  [] (auto && v) { return preturn (v.str ()); }
        ;
      result<std::string> r = parse (p, input);
      TEST_EQ (success (std::string ("1234")), r);
    }

    {
      auto p =
            pskip_char ('1')
        ;
      result<unit_type> r = parse (p, input);
      TEST_EQ (success (unit), r);
    }

    {
      auto p =
            pskip_char ('2')
        ;
      result<unit_type> r = parse (p, input);
      TEST_EQ (failure<unit_type> (), r);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('2')
        ;
      result<unit_type> r = parse (p, input);
      TEST_EQ (success (unit), r);
    }

    {
      auto p =
            pskip_char ('1')
        <   pskip_char ('1')
        ;
      result<unit_type> r = parse (p, input);
      TEST_EQ (failure<unit_type> (), r);
    }

    {
      auto p =
            pint ()
        ;
      result<int> r = parse (p, input);
      TEST_EQ (success (1234), r);
    }

    {
      auto p =
            pskip_ws ()
        ;
      result<unit_type> r = parse (p, input);
      TEST_EQ (success (unit), r);
    }

    {
      auto p =
            pint ()
        >   pskip_ws ()
        >   pskip_char ('+')
        >   pskip_ws ()
        >=  [] (int v) { return pint () >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> r = parse (p, input);
      TEST_EQ (success (std::make_tuple (1234, 5678)), r);
    }

    {
      auto p =
            pint ()
        >   pskip_ws ()
        >   pskip_char ('-')
        >   pskip_ws ()
        >=  [] (int v) { return pint () >= [v] (int u) { return preturn (std::make_tuple (v,u)); }; }
        ;
      result<std::tuple<int,int>> r = parse (p, input);
      auto e = failure<std::tuple<int,int>> ();
      TEST_EQ (e, r);
    }

  }

}


int main()
{
  parser::test_opt<std::string> ("1234", "5678");
  parser::test_opt<int> (1,3);
  parser::test_parser ();
	return 0;
}

