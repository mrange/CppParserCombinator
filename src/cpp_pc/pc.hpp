#pragma once

#include <algorithm>
#include <string>
#include <type_traits>

#include "common.hpp"
#include "opt.hpp"

namespace cpp_pc
{
  CPP_PC__PRELUDE int EOS = 0xFF000001;

  struct sub_string
  {
    CPP_PC__PRELUDE sub_string (char const * begin, char const * end)
      : begin   (begin)
      , end     (end)
    {
    }

    sub_string ()                                 = delete ;
    sub_string (sub_string const &)               = default;
    sub_string (sub_string &&)                    = default;
    sub_string & operator = (sub_string const &)  = delete ;
    sub_string & operator = (sub_string &&)       = delete ;
    ~sub_string ()                                = default;

    CPP_PC__INLINE std::string str () const
    {
      return std::string (begin, end);
    }

    CPP_PC__PRELUDE std::size_t size () const
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

    state (char const * begin, char const * end) noexcept
      : begin   (begin)
      , end     (end)
      , current (begin)
    {
      CPP_PC__ASSERT(begin <= end);
    }

    CPP_PC__INLINE int peek () noexcept
    {
      CPP_PC__ASSERT(current <= end);
      return
          current < end
        ? *current
        : EOS
        ;
    }

    CPP_PC__INLINE std::size_t remaining () const noexcept
    {
      CPP_PC__ASSERT(current <= end);
      return end - current;
    }

    CPP_PC__INLINE void advance () noexcept
    {
      CPP_PC__ASSERT(current <= end);
      if (current < end)
      {
        ++current;
      }
    }

    template<typename TSatisfyFunction>
    CPP_PC__INLINE sub_string satisfy (std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function) noexcept
    {
      CPP_PC__ASSERT(current <= end);

      auto rem = remaining ();

      if (rem < at_least)
      {
        return sub_string (current, current);
      }

      auto start  = current;
      auto last   = start + std::min (rem, at_most);

      for (
        ; current < last && satisfy_function (static_cast<std::size_t> (current - start), *current)
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

    CPP_PC__PRELUDE result (empty_result_type)
    {
    }

    CPP_PC__PRELUDE explicit result (T const & o)
      : value (o)
    {
    }

    CPP_PC__PRELUDE explicit result (T && o) noexcept
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
  CPP_PC__PRELUDE auto pbind (TParser && t, TParserGenerator && fu);

  template<typename TParser, typename TOtherParser>
  CPP_PC__PRELUDE auto pleft (TParser && t, TOtherParser && u);

  template<typename TParser, typename TOtherParser>
  CPP_PC__PRELUDE auto pright (TParser && t, TOtherParser && u);

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
    CPP_PC__PRELUDE parser (parser_function_type const & parser_function)
      : parser_function (parser_function)
    {
    }

    CPP_PC__PRELUDE parser (parser_function_type && parser_function)
      : parser_function (std::move (parser_function))
    {
    }

    CPP_PC__PRELUDE result<value_type> operator () (state & s) const
    {
      return parser_function (s);
    }

    template<typename TBindFunction>
    CPP_PC__PRELUDE auto operator >= (TBindFunction && bind_function) const
    {
      return pbind (*this, std::forward<TBindFunction> (bind_function));
    };


    template<typename TParser>
    CPP_PC__PRELUDE auto operator > (TParser && parser) const
    {
      return pleft (*this, std::forward<TParser> (parser));
    };

    template<typename TParser>
    CPP_PC__PRELUDE auto operator < (TParser && parser) const
    {
      return pright (*this, std::forward<TParser> (parser));
    };

  };

  namespace detail
  {
    template<typename TParserFunction>
    CPP_PC__PRELUDE auto adapt_parser_function (TParserFunction && parser_function)
    {
      using parser_function_type  = strip_type_t<TParserFunction>                     ;
      using parser_result_type    = std::result_of_t<parser_function_type (state &)>  ;
      using value_type            = typename parser_result_type::value_type           ;

      return parser<value_type, parser_function_type> (std::forward<TParserFunction> (parser_function));
    }
  }

  template<typename T>
  CPP_PC__PRELUDE auto success (T && v)
  {
    return result<detail::strip_type_t<T>> (std::forward<T> (v));
  }

  template<typename T>
  CPP_PC__PRELUDE auto failure ()
  {
    return result<T> ();
  }

  template<typename TValueType, typename TParserFunction>
  CPP_PC__INLINE auto parse (parser<TValueType, TParserFunction> const & p, std::string const & i)
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
  CPP_PC__PRELUDE auto preturn (TValue && v)
  {
    return detail::adapt_parser_function (
      [v = std::forward<TValue> (v)] (state & s)
      {
        return success (v);
      });
  }

  template<typename TParser, typename TParserGenerator>
  CPP_PC__PRELUDE auto pbind (TParser && t, TParserGenerator && fu)
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
  CPP_PC__PRELUDE auto pleft (TParser && t, TOtherParser && u)
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
  CPP_PC__PRELUDE auto pright (TParser && t, TOtherParser && u)
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
  CPP_PC__PRELUDE auto pright (TParser && t, TOtherParser && u);

  template<typename TSatisfyFunction>
  CPP_PC__PRELUDE auto psatisfy (std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state & s)
      {
        auto result = s.satisfy (at_least, at_most, satisfy_function);

        if (result.size () < at_least)
        {
          return failure<sub_string> ();
        }

        return success (std::move (result));
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__PRELUDE auto pskip_satisfy (std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state & s)
      {
        s.satisfy (at_least, at_most, satisfy_function);
        return success (unit);
      });
  }

  CPP_PC__INLINE auto pskip_char (char ch)
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

  CPP_PC__INLINE auto pskip_ws ()
  {
    return pskip_satisfy (0U, SIZE_MAX, satisfy_whitespace);
  }

  CPP_PC__INLINE auto pint ()
  {
    return detail::adapt_parser_function (
      [] (state & s)
      {
        auto ss = s.satisfy (1U, 10U, satisfy_digit);
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
}