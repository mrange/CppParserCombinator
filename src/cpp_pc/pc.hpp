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
#pragma once
// ----------------------------------------------------------------------------
#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
// ----------------------------------------------------------------------------
#include "common.hpp"
#include "opt.hpp"
// ----------------------------------------------------------------------------
#define CPP_PC__CHECK_PARSER(p) static_assert (::cpp_pc::detail::is_parser<decltype (p)>::value, "Must be a valid parser")
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
namespace cpp_pc
{
  CPP_PC__PRELUDE int EOS = 0x70000001;

  struct sub_string
  {
    CPP_PC__CTOR_COPY_MOVE (sub_string);

    CPP_PC__PRELUDE sub_string (char const * begin, char const * end)
      : begin   (begin)
      , end     (end)
    {
    }

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

  struct expected_error   ;
  struct unexpected_error ;

  struct error_visitor
  {
    CPP_PC__NO_COPY_MOVE (error_visitor);

    error_visitor ()           = default;
    virtual ~error_visitor ()  = default;

    virtual void visit (expected_error &  ) = 0;
    virtual void visit (unexpected_error &) = 0;
  };

  struct base_error
  {
    using ptr = std::shared_ptr<base_error>;

    CPP_PC__NO_COPY_MOVE (base_error);

    base_error ()           = default;
    virtual ~base_error ()  = default;

    virtual void apply (error_visitor &) = 0;
  };

  using base_errors = std::vector<base_error::ptr>;

  struct expected_error : base_error
  {
    expected_error (std::string expected)
      : expected (std::move (expected))
    {
    }

    void apply (error_visitor & v) override
    {
      v.visit (*this);
    }

    std::string expected ;
  };

  struct unexpected_error : base_error
  {
    unexpected_error (std::string unexpected)
      : unexpected (std::move (unexpected))
    {
    }

    void apply (error_visitor & v) override
    {
      v.visit (*this);
    }

    std::string unexpected ;
  };

  namespace detail
  {
    struct collect_error_visitor : error_visitor
    {
      std::vector<std::string>  expected  ;
      std::vector<std::string>  unexpected;

      void visit (expected_error & e) override
      {
        expected.push_back (e.expected);
      }

      void visit (unexpected_error & e) override
      {
        unexpected.push_back (e.unexpected);
      }
    };
  }

  struct state
  {
    CPP_PC__NO_COPY_MOVE (state);

    state ()                              = delete ;

    state (std::size_t error_position, char const * begin, char const * end) noexcept
      : error_position(error_position)
      , begin         (begin)
      , end           (end)
    {
      CPP_PC__ASSERT (begin <= end);
    }

    CPP_PC__INLINE int peek (std::size_t position) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT (current <= end);
      return
          current < end
        ? *current
        : EOS
        ;
    }

    CPP_PC__INLINE std::size_t remaining (std::size_t position) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT (current <= end);
      return end - current;
    }

    template<typename TSatisfyFunction>
    CPP_PC__INLINE sub_string satisfy (
        std::size_t position
      , std::size_t at_most
      , TSatisfyFunction && satisfy_function
      ) const noexcept
    {
      auto current = begin + position;
      CPP_PC__ASSERT (current <= end);

      auto rem = remaining (position);

      auto start  = current;
      auto last   = start + std::min (rem, at_most);

      for (
        ; current < last && satisfy_function (static_cast<std::size_t> (current - start), *current)
        ; ++current
        )
        ;

      return sub_string (start, current);
    }

    CPP_PC__INLINE void append_error (std::size_t position, base_error::ptr const & error) const
    {
      if (position == error_position && error)
      {
        errors.push_back (error);
      }
    }

    std::string error_description () const
    {
      std::string prelude   = "Parse failure: ";
      std::string indicator (prelude.size () + error_position, ' ');
      indicator += '^';

      std::stringstream o;
      o
        << prelude << std::string (begin, end) << std::endl
        << indicator
        ;

      detail::collect_error_visitor visitor;
      for (auto && error : errors)
      {
        error->apply (visitor);
      }

      if (!visitor.expected.empty ())
      {
        auto & vs = visitor.expected;
        std::sort (vs.begin (), vs.end ());
        vs.erase (std::unique (vs.begin (), vs.end ()), vs.end ());

        o
          << std::endl
          << "  Expected"
          ;

        auto sz = vs.size ();
        for (auto iter = 0U; iter < sz; ++iter)
        {
          auto & e = vs[iter];

          if (iter == 0)
          {
            o << ' ';
          }
          else if (iter + 1 == sz && sz > 1)
          {
            o << " or ";
          }
          else
          {
            o << ", ";
          }

          o << e;
        }
      }

      if (!visitor.unexpected.empty ())
      {
        auto & vs = visitor.expected;
        std::sort (vs.begin (), vs.end ());
        vs.erase (std::unique (vs.begin (), vs.end ()), vs.end ());

        o
          << std::endl
          << "  Unexpected"
          ;

        auto sz = vs.size ();
        for (auto iter = 0U; iter < sz; ++iter)
        {
          auto & ue = vs[iter];

          if (iter == 0)
          {
            o << ' ';
          }
          else if (iter + 1 == sz && sz > 1)
          {
            o << " and ";
          }
          else
          {
            o << ", ";
          }

          o << ue;
        }
      }

      return o.str ();
    }


    std::size_t const   error_position;
    char const * const  begin         ;
    char const * const  end           ;

    base_errors mutable errors        ;
  };

  template<typename T>
  struct result
  {
    using value_type = T;

    CPP_PC__COPY_MOVE (result);

    result ()                             = delete ;

    CPP_PC__INLINE explicit result (std::size_t position)
      : end       (position)
    {
    }

    CPP_PC__INLINE explicit result (std::size_t position, T o)
      : end       (position)
      , value     (std::move (o))
    {
    }

    CPP_PC__PRELUDE bool operator == (result const & o) const
    {
      return
            end   == o.end
        &&  value == o.value
        ;
    }

    CPP_PC__INLINE result<value_type> & reposition (std::size_t e)
    {
      end = e;
      return *this;
    }

    template<typename TOther>
    CPP_PC__PRELUDE result<TOther> fail_as () const
    {
      return result<TOther> (end);
    }

    std::size_t     end       ;
    opt<T>          value     ;
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
  CPP_PC__PRELUDE auto success (std::size_t end, T && v)
  {
    return result<detail::strip_type_t<T>> (end, std::forward<T> (v));
  }

  template<typename T>
  CPP_PC__INLINE auto failure (std::size_t position)
  {
    return result<T> (position);
  }

  template<typename TValueType, typename TParserFunction>
  CPP_PC__INLINE auto plain_parse (parser<TValueType, TParserFunction> const & p, std::string const & i)
  {
    auto begin  = i.c_str ();
    auto end    = begin + i.size ();

    state s (SIZE_MAX, begin, end);
    return p (s, 0);
  }

  template<typename T>
  struct parse_result
  {
    CPP_PC__COPY_MOVE (parse_result);

    parse_result (std::size_t consumed, opt<T> value, std::string message)
      : consumed(consumed)
      , value   (std::move (value))
      , message (std::move (message))
    {
    }

    std::size_t consumed;
    opt<T>      value   ;
    std::string message ;
  };

  template<typename TValueType, typename TParserFunction>
  CPP_PC__INLINE auto parse (parser<TValueType, TParserFunction> const & p, std::string const & i)
  {
    auto begin  = i.c_str ();
    auto end    = begin + i.size ();

    {
      state s (SIZE_MAX, begin, end);
      auto v = p (s, 0);
      if (v.value)
      {
        return parse_result<TValueType> (v.end, std::move (v.value), std::string ());
      }
      else
      {
        state es (v.end, begin, end);
        auto ev = p (es, 0);

        CPP_PC__ASSERT (v.end == ev.end);
        CPP_PC__ASSERT (!ev.value);

        return parse_result<TValueType> (ev.end, empty_opt, es.error_description ());
      }
    }
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
      [v = std::forward<TValue> (v)] (state const &, std::size_t position)
      {
        return success (position, v);
      });
  }

  auto const punit =
    detail::adapt_parser_function (
      [] (state const &, std::size_t position)
      {
        return success (position, unit);
      });

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
          auto tu = fu (std::move (tv.value.get ())) (s, tv.end);

          return tu
            .reposition (tu.end)
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
          auto tu = u (s, tv.end);
          if (tu.value)
          {
            return tv
              .reposition (tu.end)
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
              .reposition (tu.end)
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
          auto tu = u (s, tv.end);
          return tu
            .reposition (tu.end)
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

  template<typename TParser, typename TMapper>
  CPP_PC__PRELUDE auto pmap (TParser && t, TMapper && m)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), m = std::forward<TMapper> (m)] (state const & s, std::size_t position)
      {

        auto tv = t (s, position);

        using value_type  = decltype (m (std::move (tv.value.get()))) ;

        if (tv.value)
        {
          return success (tv.end, m (std::move (tv.value.get())));
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
    template<typename TValue>
    struct ptrampoline_payload
    {
      using ptr           = std::shared_ptr<ptrampoline_payload>;
      using trampoline_f  = std::function<result<TValue> (state const & s, std::size_t position)>;

      CPP_PC__PRELUDE ptrampoline_payload () = default;

      CPP_PC__NO_COPY_MOVE (ptrampoline_payload);

      trampoline_f trampoline;
    };
  }


  template<typename TValue>
  CPP_PC__INLINE auto create_trampoline ()
  {
    return std::make_shared<detail::ptrampoline_payload<TValue>> ();
  }

  template<typename TValue>
  CPP_PC__INLINE auto ptrampoline (typename detail::ptrampoline_payload<TValue>::ptr payload)
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
          return result<TValue> (position);
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

      CPP_PC__CTOR_COPY_MOVE (pchoice_impl);

      CPP_PC__PRELUDE pchoice_impl (THead const & head)
        : head (head)
      {
      }

      CPP_PC__INLINE result<TValue> parse (state const & s, std::size_t position) const
      {
        auto hv = head (s, position);
        return hv;
      }

      THead head;
    };

    template<typename TValue, typename THead, typename... TTail>
    struct pchoice_impl<TValue, THead, TTail...> : pchoice_impl<TValue, TTail...>
    {
      using base_type = pchoice_impl<TValue, TTail...>;
      using value_type = detail::parser_value_type_t<THead>;

      static_assert (std::is_same<TValue, value_type>::value, "All pchoice parsers must produce values of the same value_type");

      CPP_PC__CTOR_COPY_MOVE (pchoice_impl);

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
          if (s.error_position == position)
          {
            // In order to collect error info
            base_type::parse (s, position);
          }
          return hv;
        }
        else
        {
          return base_type::parse (s, position);
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
        if (s.error_position == position)
        {
          auto tv = impl.parse (s, position);
          return tv;
        }
        else
        {
          return impl.parse (s, position);
        }
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

        auto v = parser (s, bv.end);
        if (!v.value)
        {
          return v;
        }

        auto ev = end_parser (s, v.end);
        if (!ev.value)
        {
          return ev
#ifdef _MSC_VER
            .fail_as<value_type> ()
#else
            .template fail_as<value_type> ()
#endif
            ;
        }

        return v
          .reposition (ev.end)
          ;
      });
  }

  template<typename TParser, typename TSepParser, typename TCombiner>
  CPP_PC__INLINE auto psep (
      TParser       && parser
    , TSepParser    && sep_parser
    , TCombiner     && combiner
    )
  {
    return detail::adapt_parser_function (
      [
          parser      = std::forward<TParser> (parser)
        , sep_parser  = std::forward<TSepParser> (sep_parser)
        , combiner    = std::forward<TCombiner> (combiner)
      ] (state const & s, std::size_t position)
      {
        using value_type = detail::parser_value_type_t<TParser>;

        auto v = parser (s, position);

        if (!v.value)
        {
          return v;
        }

        auto cont = true;

        while (cont)
        {
          auto sv = sep_parser (s, v.end);
          if (!sv.value)
          {
            cont = false;
            continue;
          }

          auto ov = parser (s, sv.end);
          if (!ov.value)
          {
            return ov
              ;
          }
          v.reposition (ov.end);

          CPP_PC__ASSERT (v.value);
          CPP_PC__ASSERT (ov.value);
          v.value = make_opt (combiner (std::move (v.value.get ()), std::move (sv.value.get ()), std::move (ov.value.get())));
        }
        return v;
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__INLINE auto psatisfy (std::string expected, std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    auto error = std::make_shared<expected_error> (std::move (expected));

    return detail::adapt_parser_function (
      [error = std::move (error), at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state const & s, std::size_t position)
      {
        s.append_error (position, error);

        auto result = s.satisfy (position, at_most, satisfy_function);

        auto consumed = result.size ();
        if (consumed < at_least)
        {
          return failure<sub_string> (position + consumed);
        }

        return success (position + consumed, std::move (result));
      });
  }

  namespace detail
  {
    std::string char_to_string (char ch)
    {
      char s[] = {'\'', ch, '\'', 0};
      return s;
    }

    base_error::ptr char_to_expected (char ch)
    {
      return std::make_shared<expected_error> (detail::char_to_string (ch));
    }
  }

  CPP_PC__INLINE auto pany_of (std::string expected)
  {
    base_errors errors;
    for (auto ch : expected)
    {
      errors.push_back (detail::char_to_expected (ch));
    }

    return detail::adapt_parser_function (
      [expected = std::move (expected), errors = std::move (errors)] (state const & s, std::size_t position)
      {
        if (position == s.error_position)
        {
          for (auto && error : errors)
          {
            s.append_error (position, error);
          }
        }

        auto peek = s.peek (position);
        if (peek == EOS)
        {
          return failure<char> (position);
        }

        auto find = expected.find (static_cast<char> (peek));
        if (find == std::string::npos)
        {
          return failure<char> (position);
        }

        return success (position + 1, static_cast<char> (peek));
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__INLINE auto pskip_satisfy (std::string expected, std::size_t at_least, std::size_t at_most, TSatisfyFunction && satisfy_function)
  {
    return
          psatisfy (std::move (expected), at_least, at_most, std::forward<TSatisfyFunction> (satisfy_function))
      <   punit
      ;
  }

  CPP_PC__INLINE auto pskip_char (char ch)
  {
    return detail::adapt_parser_function (
      [ch, error = detail::char_to_expected (ch)] (state const & s, std::size_t position)
      {
        s.append_error (position, error);

        auto peek = s.peek (position);
        if (peek == ch)
        {
          return success (position + 1, unit);
        }
        else
        {
          return failure<unit_type> (position);
        }
      });
  }

  auto const pskip_ws = pskip_satisfy ("whitespace", 0U, SIZE_MAX, satisfy_whitespace);

  namespace detail
  {
    auto const peos_error = std::make_shared<expected_error> ("EOS");
  }

  auto const peos =
    detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        s.append_error (position, detail::peos_error);

        auto peek = s.peek (position);
        if (peek == EOS)
        {
          return success (position, unit);
        }
        else
        {
          return failure<unit_type> (position);
        }
      });

  namespace detail
  {
    auto const pint_error = std::make_shared<expected_error> ("integer");
  }

  auto const pint =
    detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        s.append_error (position, detail::pint_error);

        auto ss = s.satisfy (position, 10U, satisfy_digit);
        auto consumed = ss.size ();
        if (consumed == 0)
        {
          return failure<int> (position + consumed);
        }

        auto i = 0;
        for (auto iter = ss.begin; iter != ss.end; ++iter)
        {
          i = 10*i + (*iter - '0');
        }

        return success (position + consumed, i);
      });
}
// ----------------------------------------------------------------------------
