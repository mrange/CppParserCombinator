#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "common.hpp"
#include "opt.hpp"

#define CPP_PC__CHECK_PARSER(p) static_assert (::cpp_pc::detail::is_parser<decltype (p)>::value, "Must be a valid parser")

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
    CPP_PC__NO_COPY_MOVE (state);

    state ()                              = delete ;

    state (char const * begin, char const * end) noexcept
      : begin   (begin)
      , end     (end)
    {
      CPP_PC__ASSERT(begin <= end);
    }

    CPP_PC__INLINE int peek (std::size_t position) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT(current <= end);
      return
          current < end
        ? *current
        : EOS
        ;
    }

    CPP_PC__INLINE std::size_t remaining (std::size_t position) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT(current <= end);
      return end - current;
    }

    template<typename TSatisfyFunction>
    CPP_PC__INLINE sub_string satisfy (
        std::size_t position
      , std::size_t at_least
      , std::size_t at_most
      , TSatisfyFunction && satisfy_function
      ) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT(current <= end);

      auto rem = remaining (position);

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
  };

  struct base_error
  {
    using ptr = std::shared_ptr<base_error>;

    CPP_PC__NO_COPY_MOVE (base_error);

    base_error ()           = default;
    virtual ~base_error ()  = default;
  };

  struct expected_error : base_error
  {
    expected_error (std::string expected)
      : expected (std::move (expected))
    {
    }

    std::string expected ;
  };

  struct unexpected_error : base_error
  {
    unexpected_error (std::string unexpected)
      : unexpected (std::move (unexpected))
    {
    }

    std::string unexpected ;
  };

  struct fork_error : base_error
  {
    fork_error (base_error::ptr left, base_error::ptr right)
      : left  (std::move (left))
      , right (std::move (right))
    {
    }

    base_error::ptr left ;
    base_error::ptr right;
  };

  template<typename T>
  struct result
  {
    using value_type = T;

    CPP_PC__COPY_MOVE (result);

    result ()                             = delete ;

    CPP_PC__INLINE explicit result (std::size_t position, base_error::ptr error)
      : position  (std::move (position))
      , error     (std::move (error))
    {
    }

    CPP_PC__PRELUDE explicit result (std::size_t position, T o)
      : position  (std::move (position))
      , value     (std::move (o))
    {
    }

    CPP_PC__PRELUDE bool operator == (result const & o) const
    {
      return
            position  == o.position
        &&  value     == o.value
        ;
    }

    CPP_PC__INLINE result<value_type> & reposition (std::size_t p)
    {
      position = p;
      return *this;
    }

    template<typename TOther>
    CPP_PC__PRELUDE result<TOther> fail_as () const
    {
      return result<TOther> (position, error);
    }

    template<typename TOther>
    CPP_PC__INLINE result<value_type> & merge_with (result<TOther> const & o)
    {
      // TODO: How should merging work on different positions
      /*
      if (position < o.position)
      {
      }
      else if (o.position < position)
      {
        position = o.position;
        error = o.error;
      }
      */
      if (error && o.error)
      {
        error = std::make_shared<fork_error> (std::move (error), o.error);
      }
      else if (o.error)
      {
        error = o.error;
      }
      /*
      else if (error)
      {
      }
      else
      {
      }
      */

      return *this;
    }

    std::size_t     position  ;
    opt<T>          value     ;
    base_error::ptr error     ;
  };

  template<typename TParser, typename TParserGenerator>
  CPP_PC__PRELUDE auto pbind (TParser && t, TParserGenerator && fu);

  template<typename TParser, typename TOtherParser>
  CPP_PC__PRELUDE auto pleft (TParser && t, TOtherParser && u);

  template<typename TParser, typename TOtherParser>
  CPP_PC__PRELUDE auto pright (TParser && t, TOtherParser && u);

  namespace detail
  {
    template<typename T>
    struct is_parser;
  }

  template<typename TValue, typename TParserFunction>
  struct parser
  {
    using parser_function_type  = TParserFunction;
    using value_type            = TValue;
    using result_type           = std::result_of_t<parser_function_type (state const &, std::size_t)>;

    parser_function_type parser_function;

    static_assert (
        std::is_same<result<value_type>, result_type>::value
      , "parser_function_type must return result<_>"
      );

    CPP_PC__PRELUDE parser (parser_function_type const & parser_function)
      : parser_function (parser_function)
    {
    }

    CPP_PC__PRELUDE parser (parser_function_type && parser_function)
      : parser_function (std::move (parser_function))
    {
    }

    CPP_PC__PRELUDE result<value_type> operator () (state const & s, std::size_t position) const
    {
      return parser_function (s, position);
    }

    template<typename TBindFunction>
    CPP_PC__PRELUDE auto operator >= (TBindFunction && bind_function) const
    {
      return pbind (*this, std::forward<TBindFunction> (bind_function));
    };


    template<typename TParser>
    CPP_PC__PRELUDE auto operator > (TParser && parser) const
    {
      CPP_PC__CHECK_PARSER (parser);
      return pleft (*this, std::forward<TParser> (parser));
    };

    template<typename TParser>
    CPP_PC__PRELUDE auto operator < (TParser && parser) const
    {
      CPP_PC__CHECK_PARSER (parser);
      return pright (*this, std::forward<TParser> (parser));
    };

  };

  namespace detail
  {
    template<typename TParserFunction>
    CPP_PC__PRELUDE auto adapt_parser_function (TParserFunction && parser_function)
    {
      using parser_function_type  = strip_type_t<TParserFunction>                                       ;
      using parser_result_type    = std::result_of_t<parser_function_type (state const &, std::size_t)> ;
      using value_type            = typename parser_result_type::value_type                             ;

      return parser<value_type, parser_function_type> (std::forward<TParserFunction> (parser_function));
    }

    template<typename T>
    struct is_parser_impl
    {
      enum
      {
        value = false,
      };
    };

    template<typename TValue, typename TParserFunction>
    struct is_parser_impl<parser<TValue, TParserFunction>>
    {
      enum
      {
        value = true,
      };
    };

    template<typename T>
    struct is_parser
    {
      enum
      {
        value = is_parser_impl<strip_type_t<T>>::value,
      };
    };

    template<typename T>
    struct parser_value_type_impl;

    template<typename TValue, typename TParserFunction>
    struct parser_value_type_impl<parser<TValue, TParserFunction>>
    {
      using type = TValue ;
    };

    template<typename T>
    struct parser_value_type
    {
      using type = typename parser_value_type_impl<strip_type_t<T>>::type;
    };

    template<typename T>
    using parser_value_type_t = typename parser_value_type<T>::type;

  }

  template<typename T>
  CPP_PC__PRELUDE auto success (std::size_t position, T && v)
  {
    return result<detail::strip_type_t<T>> (position, std::forward<T> (v));
  }

  template<typename T>
  CPP_PC__INLINE auto failure (std::size_t position, base_error::ptr error)
  {
    return result<T> (std::move (position), std::move (error));
  }

  template<typename TValueType, typename TParserFunction>
  CPP_PC__INLINE auto parse (parser<TValueType, TParserFunction> const & p, std::string const & i)
  {
    auto begin  = i.c_str ();
    auto end    = begin + i.size ();
    state s (begin, end);

    return p (s, 0);
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
      [v = std::forward<TValue> (v)] (state const & s, std::size_t position)
      {
        return success (position, v);
      });
  }

  template<typename TParser, typename TParserGenerator>
  CPP_PC__PRELUDE auto pbind (TParser && t, TParserGenerator && fu)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), fu = std::forward<TParserGenerator> (fu)] (state const & s, std::size_t position)
      {
        auto tv = t (s, position);

        using result_type = decltype (fu (std::move (tv.value.get ())) (s, 0));
        using value_type  = typename result_type::value_type                  ;

        if (tv.value)
        {
          return fu (std::move (tv.value.get ())) (s, tv.position)
            .merge_with (tv)
            ;
        }
        else
        {
          return tv
#ifdef _MSC_VER
            .fail_as<value_type> ()
#else
            .template fail_as<value_type> ()
#endif
            ;
        }
      });
  }

  template<typename TParser, typename TOtherParser>
  CPP_PC__PRELUDE auto pleft (TParser && t, TOtherParser && u)
  {
    CPP_PC__CHECK_PARSER (t);
    CPP_PC__CHECK_PARSER (u);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), u = std::forward<TOtherParser> (u)] (state const & s, std::size_t position)
      {
        using result_type = decltype (t (s, 0))               ;
        using value_type  = typename result_type::value_type  ;

        auto tv = t (s, position);
        if (tv.value)
        {
          auto tu = u (s, tv.position);
          if (tu.value)
          {
            return tv
              .merge_with (tu)
              .reposition (tu.position)
              ;
          }
          else
          {
            return tu
#ifdef _MSC_VER
              .fail_as<value_type> ()
#else
              .template fail_as<value_type> ()
#endif
              .merge_with (tv)
              ;
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
    CPP_PC__CHECK_PARSER (t);
    CPP_PC__CHECK_PARSER (u);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), u = std::forward<TOtherParser> (u)] (state const & s, std::size_t position)
      {
        using result_type = decltype (u (s, 0))               ;
        using value_type  = typename result_type::value_type  ;

        auto tv = t (s, position);
        if (tv.value)
        {
          return u (s, tv.position)
            .merge_with (tv)
            ;
        }
        else
        {
          return tv
#ifdef _MSC_VER
            .fail_as<value_type> ()
#else
            .template fail_as<value_type> ()
#endif
            ;
        }
      });
  }

  namespace detail
  {
    auto const ptrampoline_error = std::make_shared<unexpected_error> ("empty trampoline");
  }

  template<typename TValue>
  struct ptrampoline_payload
  {
    using ptr           = std::shared_ptr<ptrampoline_payload>;
    using trampoline_f  = std::function<result<TValue> (state const & s, std::size_t position)>;

    CPP_PC__PRELUDE ptrampoline_payload () = default;

    CPP_PC__NO_COPY_MOVE (ptrampoline_payload);

    trampoline_f trampoline;
  };

  template<typename TValue>
  CPP_PC__INLINE auto create_trampoline_payload ()
  {
    return std::make_shared<ptrampoline_payload<TValue>> ();
  }

  template<typename TValue>
  CPP_PC__INLINE auto ptrampoline (typename ptrampoline_payload<TValue>::ptr payload)
  {
    CPP_PC__ASSERT ("empty payload" && payload);

    return detail::adapt_parser_function (
      [payload = std::move (payload)] (state const & s, std::size_t position)
      {
        CPP_PC__ASSERT ("empty trampoline" && payload->trampoline);
        if (payload->trampoline)
        {
          return payload->trampoline (s, position);
        }
        else
        {
          return result<TValue> (position, detail::ptrampoline_error);
        }
      });
  }

  template<typename TParser>
  CPP_PC__INLINE auto pbreakpoint (TParser && parser)
  {
    CPP_PC__CHECK_PARSER (parser);

    return detail::adapt_parser_function (
      [parser = std::forward<TParser> (parser)] (state const & s, std::size_t position)
      {
        CPP_PC__ASSERT ("pbreakpoint" && false);
        return parser (s, position);
      });
  }

  namespace detail
  {
    template<typename TValue, typename ...TParsers>
    struct pchoice_impl;

    template<typename TValue, typename THead>
    struct pchoice_impl<TValue, THead>
    {
      using value_type = detail::parser_value_type_t<THead>;

      static_assert (std::is_same<TValue, value_type>::value, "All pchoice parsers must produce values of the same value_type");

      CPP_PC__COPY_MOVE (pchoice_impl);

      CPP_PC__PRELUDE pchoice_impl (THead const & head)
        : head (head)
      {
      }

      CPP_PC__PRELUDE result<TValue> parse (state const & s, std::size_t position) const
      {
        return head (s, position);
      }

      THead head;
    };

    template<typename TValue, typename THead, typename... TTail>
    struct pchoice_impl<TValue, THead, TTail...> : pchoice_impl<TValue, TTail...>
    {
      using base_type = pchoice_impl<TValue, TTail...>;
      using value_type = detail::parser_value_type_t<THead>;

      static_assert (std::is_same<TValue, value_type>::value, "All pchoice parsers must produce values of the same value_type");

      CPP_PC__COPY_MOVE (pchoice_impl);

      // TODO: Forward parsers
      CPP_PC__PRELUDE pchoice_impl (THead const & head, TTail const &... tail)
        : base_type (tail...)
        , head (head)
      {
      }

      CPP_PC__INLINE result<TValue> parse (state const & s, std::size_t position) const
      {
        auto hv = head (s, position);
        if (hv.value)
        {
          return hv;  // TODO: Should continue to pickup error info
        }
        else
        {
          auto tv = base_type::parse (s, position);

          return tv
            .merge_with (hv)
            ;
        }
      }

      THead head;
    };

  }

  template<typename TParser, typename ...TParsers>
  CPP_PC__INLINE auto pchoice (TParser && parser, TParsers && ...parsers)
  {
    using value_type = detail::parser_value_type_t<TParser>;

    detail::pchoice_impl<value_type, TParser, TParsers...> impl (
      // TODO: Perfect forward
        parser
      , parsers...
      );

    return detail::adapt_parser_function (
      [impl = std::move (impl)] (state const & s, std::size_t position)
      {
        return impl.parse (s, position);
      });
  }

  template<typename TBeginParser, typename TParser, typename TEndParser>
  CPP_PC__INLINE auto pbetween (
      TBeginParser  && begin_parser
    , TParser       && parser
    , TEndParser    && end_parser
    )
  {
    return detail::adapt_parser_function (
      [
          begin_parser  = std::forward<TBeginParser> (begin_parser)
        , parser        = std::forward<TParser> (parser)
        , end_parser    = std::forward<TEndParser> (end_parser)
      ] (state const & s, std::size_t position)
      {
        using value_type = detail::parser_value_type_t<TParser>;

        auto bv = begin_parser (s, position);
        if (!bv.value)
        {
          return bv
#ifdef _MSC_VER
            .fail_as<value_type> ()
#else
            .template fail_as<value_type> ()
#endif
            ;
        }

        auto v = parser (s, bv.position);
        if (!v.value)
        {
          return v
            .merge_with (bv)
            ;
        }

        auto ev = end_parser (s, v.position);
        if (!ev.value)
        {
          return ev
#ifdef _MSC_VER
            .fail_as<value_type> ()
#else
            .template fail_as<value_type> ()
#endif
            .merge_with (v)
            .merge_with (bv)
            ;
        }

        return v
          .merge_with (bv)
          .merge_with (ev)
          .reposition (ev.position)
          ;
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__INLINE auto psatisfy (std::string expected, std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    auto error = std::make_shared<expected_error> (std::move (expected));

    return detail::adapt_parser_function (
      [error = std::move (error), at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state const & s, std::size_t position)
      {
        auto result = s.satisfy (position, at_least, at_most, satisfy_function);

        auto consumed = result.size ();
        if (consumed < at_least)
        {
          return failure<sub_string> (position, error);
        }

        return success (position + consumed, std::move (result));
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__PRELUDE auto pskip_satisfy (std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state const & s, std::size_t position)
      {
        auto ss = s.satisfy (position, at_least, at_most, satisfy_function);
        return success (position + ss.size (), unit);
      });
  }

  CPP_PC__INLINE auto pskip_char (char ch)
  {
    char expected[] = {'\'', ch, '\'', 0};
    auto error = std::make_shared<expected_error> (expected);
    return detail::adapt_parser_function (
      [ch, error = std::move (error)] (state const & s, std::size_t position)
      {
        auto peek = s.peek (position);
        if (peek == ch)
        {
          return success (position + 1, unit);
        }
        else
        {
          return failure<unit_type> (position, error);
        }
      });
  }

  CPP_PC__INLINE auto pskip_ws ()
  {
    return pskip_satisfy (0U, SIZE_MAX, satisfy_whitespace);
  }

  namespace detail
  {
    auto const pint_error = std::make_shared<expected_error> ("integer");
  }

  CPP_PC__INLINE auto pint ()
  {
    return detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        auto ss = s.satisfy (position, 1U, 10U, satisfy_digit);
        auto consumed = ss.size ();
        if (consumed > 0)
        {
          auto i = 0;
          for (auto iter = ss.begin; iter != ss.end; ++iter)
          {
            i = 10*i + (*iter - '0');
          }
          return success (position + consumed, i);
        }
        else
        {
          return failure<int> (position, detail::pint_error);
        }
      });
  }
}