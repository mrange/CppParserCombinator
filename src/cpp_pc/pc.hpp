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
#include <tuple>
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

    CPP_PC__INLINE sub_string (char const * begin, char const * end)
      : begin   (begin)
      , end     (end)
    {
      CPP_PC__ASSERT (begin <= end);
    }

    CPP_PC__INLINE std::string str () const
    {
      return std::string (begin, end);
    }

    CPP_PC__INLINE std::size_t size () const
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
    auto make_expected (std::string e)
    {
      return std::make_shared<expected_error> (std::move (e));
    }

    auto make_unexpected (std::string ue)
    {
      return std::make_shared<unexpected_error> (std::move (ue));
    }


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
      case '\f':
      case '\n':
      case '\r':
      case '\t':
        return true;
      default:
        return false;
      }
    };

  struct state
  {
    CPP_PC__NO_COPY_MOVE (state);

    state ()                              = delete ;

    state (std::size_t error_position, char const * begin, char const * end) noexcept
      : error_position(error_position)
      , begin         (std::min (begin, end))
      , end           (std::max (begin, end))
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
      CPP_PC__ASSERT (begin <= end);

      std::string prelude   = "Parse failure: ";

      auto input_sz         = static_cast<std::size_t> (end - begin);
      auto wsize            = std::min (input_sz, 80U - prelude.size ());
      auto hwsize           = wsize / 2;
      auto desired_err_pos  = std::min (error_position, input_sz);
      auto err_pos          = static_cast<std::size_t> (0);

      std::string input;

      if (input_sz > wsize)
      {
        auto abegin           = (desired_err_pos < hwsize)            ? 0U      : desired_err_pos - hwsize;
        auto aend             = (desired_err_pos + hwsize > input_sz) ? input_sz: desired_err_pos + hwsize;

        err_pos               = aend == 0U ? desired_err_pos : hwsize;
        std::string (begin + abegin, begin + aend).swap (input);
      }
      else
      {
        err_pos               = desired_err_pos;
        std::string (begin, end).swap (input);
      }

      for (auto && ch : input)
      {
        if (satisfy_whitespace (0, ch))
        {
          ch = ' ';
        }
      }
      auto err_ch   = err_pos < input.size () ? input[err_pos] : ' ';

      // TODO: Handle multiline

      std::string indicator (prelude.size () + err_pos, ' ');
      indicator += '^';

      std::stringstream o;
      o
        << prelude << input << std::endl
        << indicator
        << std::endl
        << "  Found '"
        << err_ch
        << "', position: "
        << desired_err_pos
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
            o << " nor ";
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

    CPP_PC__PRELUDE explicit result (std::size_t position)
      : position  (position)
    {
    }

    CPP_PC__PRELUDE explicit result (std::size_t position, T o)
      : position  (position)
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

    CPP_PC__PRELUDE static auto success (std::size_t pos, T && v)
    {
      return result<T> (pos, std::move (v));
    }

    CPP_PC__PRELUDE static auto success (std::size_t pos, T const & v)
    {
      return result<T> (pos, v);
    }

    CPP_PC__PRELUDE static auto failure (std::size_t pos)
    {
      return result<T> (pos);
    }

    std::size_t     position  ;
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
    using result_type           = detail::strip_type_t<std::result_of_t<parser_function_type (state const &, std::size_t)>>;

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
    CPP_PC__PRELUDE auto operator > (TParser && t) const
    {
      CPP_PC__CHECK_PARSER (t);
      return pleft (*this, std::forward<TParser> (t));
    };

    template<typename TParser>
    CPP_PC__PRELUDE auto operator < (TParser && t) const
    {
      CPP_PC__CHECK_PARSER (t);
      return pright (*this, std::forward<TParser> (t));
    };

  };

  namespace detail
  {
    template<typename TParserFunction>
    CPP_PC__PRELUDE auto adapt_parser_function (TParserFunction && parser_function)
    {
      using parser_function_type  = strip_type_t<TParserFunction>                                                     ;
      using parser_result_type    = strip_type_t<std::result_of_t<parser_function_type (state const &, std::size_t)>> ;
      using value_type            = typename parser_result_type::value_type                                           ;

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
        return parse_result<TValueType> (v.position, std::move (v.value), std::string ());
      }
      else
      {
        state es (v.position, begin, end);
        auto ev = p (es, 0);

        CPP_PC__ASSERT (v.position == ev.position);
        CPP_PC__ASSERT (!ev.value);

        return parse_result<TValueType> (ev.position, empty_opt, es.error_description ());
      }
    }
  }

  // parser<'T> = state -> result<'T>

  template<typename TValue>
  CPP_PC__PRELUDE auto preturn (TValue && v)
  {
    return detail::adapt_parser_function (
      [v = std::forward<TValue> (v)] (state const &, std::size_t position)
      {
        using result_type = result<detail::strip_type_t<decltype (v)>>;

        return result_type::success (position, v);
      });
  }

  auto const punit =
    detail::adapt_parser_function (
      [] (state const &, std::size_t position)
      {
        using result_type = result<unit_type>;

        return result_type::success (position, unit);
      });

  template<typename TParser, typename TParserGenerator>
  CPP_PC__PRELUDE auto pbind (TParser && t, TParserGenerator && fu)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t), fu = std::forward<TParserGenerator> (fu)] (state const & s, std::size_t position)
      {
        auto tv = t (s, position);

        using result_type = detail::strip_type_t<decltype (fu (std::move (tv.value.get ())) (s, 0))>;

        if (tv.value)
        {
          auto tu = fu (std::move (tv.value.get ())) (s, tv.position);

          return tu
            .reposition (tu.position)
            ;
        }
        else
        {
          return result_type::failure (tv.position);
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
        using result_type = detail::strip_type_t<decltype (t (s, 0))>;

        auto tv = t (s, position);
        if (tv.value)
        {
          auto tu = u (s, tv.position);
          if (tu.value)
          {
            return tv
              .reposition (tu.position)
              ;
          }
          else
          {
            return result_type::failure (tu.position);
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
        using result_type = detail::strip_type_t<decltype (u (s, 0))>;

        auto tv = t (s, position);
        if (tv.value)
        {
          auto tu = u (s, tv.position);
          return tu
            .reposition (tu.position)
            ;
        }
        else
        {
          return result_type::failure (tv.position);
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

        using mvalue_type = detail::strip_type_t<decltype (m (std::move (tv.value.get())))>;
        using result_type = result<mvalue_type>                       ;

        if (tv.value)
        {
          return result_type::success (tv.position, m (std::move (tv.value.get())));
        }
        else
        {
          return result_type::failure (tv.position);
        }
      });
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto popt (TParser && t)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t)] (state const & s, std::size_t position)
      {
        using tresult_type= detail::strip_type_t<decltype (t (s, 0))> ;
        using tvalue_type = typename tresult_type::value_type         ;
        using result_type = result<opt<tvalue_type>>                  ;

        auto tv = t (s, position);

        if (tv.value)
        {
          return result_type::success (tv.position, std::move (tv.value));
        }
        else
        {
          return result_type::success (position, empty_opt);
        }
      });
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany (std::size_t at_least, std::size_t at_most, TParser && t)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [at_least, at_most, t = std::forward<TParser> (t)] (state const & s, std::size_t position)
      {
        using tresult_type = detail::strip_type_t<decltype (t (s, 0))>  ;
        using tvalue_type  = typename tresult_type::value_type          ;
        using result_type  = result<std::vector<tvalue_type>>           ;

        std::vector<tvalue_type> values;
        values.reserve (at_least);

        auto current = position;

        auto cont = true;

        while (cont)
        {
          if (values.size () >= at_most)
          {
            cont = false;
            continue;
          }

          auto tv = t (s, current);
          if (!tv.value)
          {
            cont = false;
            continue;
          }

          values.push_back (std::move (tv.value.get ()));

          current = tv.position;
        }

        if (values.size () >= at_least)
        {
          return result_type::success (current, std::move (values));
        }
        else
        {
          return result_type::failure (current);
        }
      });
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany (TParser && t)
  {
    return pmany (0, SIZE_MAX, std::forward<TParser> (t));
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany1 (TParser && t)
  {
    return pmany (1, SIZE_MAX, std::forward<TParser> (t));
  }

  template<typename TParser, typename TSepParser>
  CPP_PC__PRELUDE auto pmany_sepby (
      std::size_t   at_least
    , std::size_t   at_most
    , bool          allow_trailing_sep
    , TParser &&    t
    , TSepParser && sep_parser
    )
  {
    CPP_PC__CHECK_PARSER (t);
    CPP_PC__CHECK_PARSER (sep_parser);

    return detail::adapt_parser_function (
      [at_least, at_most, allow_trailing_sep, t = std::forward<TParser> (t), sep_parser = std::forward<TSepParser> (sep_parser)] (state const & s, std::size_t position)
      {
        using tresult_type = detail::strip_type_t<decltype (t (s, 0))>          ;
        using tvalue_type  = typename tresult_type::value_type                  ;

        using sresult_type = detail::strip_type_t<decltype (sep_parser (s, 0))> ;
        using svalue_type  = typename sresult_type::value_type                  ;

        using result_type  = result<std::vector<tvalue_type>>                   ;

        static_assert (
            std::is_same<unit_type, svalue_type>::value
          , "sep_parser must return result<unit_type>"
          );

        std::vector<tvalue_type> values;
        values.reserve (at_least);

        auto current = position;

        {
          auto tv = t (s, current);
          if (!tv.value)
          {
            return result_type::success (current, std::move (values));
          }

          values.push_back (std::move (tv.value.get ()));

          current = tv.position;
        }

        auto cont = true;

        while (cont)
        {
          if (values.size () >= at_most)
          {
            cont = false;
            continue;
          }

          auto sv = sep_parser (s, current);
          if (!sv.value)
          {
            cont = false;
            continue;
          }

          current = sv.position;

          auto tv = t (s, current);
          if (!tv.value)
          {
            if (allow_trailing_sep)
            {
              cont = false;
              continue;
            }
            else
            {
              return result_type::failure (tv.position);
            }
          }

          values.push_back (std::move (tv.value.get ()));

          current = tv.position;
        }

        if (values.size () >= at_least)
        {
          return result_type::success (current, std::move (values));
        }
        else
        {
          return result_type::failure (current);
        }
      });
  }

  template<typename TParser, typename TSepParser>
  CPP_PC__PRELUDE auto pmany_sepby (
      bool          allow_trailing_sep
    , TParser &&    t
    , TSepParser && sep_parser
    )
  {
    return pmany_sepby (0, SIZE_MAX, allow_trailing_sep, std::forward<TParser> (t), std::forward<TSepParser> (sep_parser));
  }

  template<typename TParser, typename TSepParser>
  CPP_PC__PRELUDE auto pmany_sepby1 (
      bool          allow_trailing_sep
    , TParser &&    t
    , TSepParser && sep_parser
    )
  {
    return pmany_sepby (1, SIZE_MAX, allow_trailing_sep, std::forward<TParser> (t), std::forward<TSepParser> (sep_parser));
  }

  template<typename TParser, typename TSepParser>
  CPP_PC__PRELUDE auto pmany_sepby (
      TParser &&    t
    , TSepParser && sep_parser
    )
  {
    return pmany_sepby (0, SIZE_MAX, false, std::forward<TParser> (t), std::forward<TSepParser> (sep_parser));
  }

  template<typename TParser, typename TSepParser>
  CPP_PC__PRELUDE auto pmany_sepby1 (
      TParser &&    t
    , TSepParser && sep_parser
    )
  {
    return pmany_sepby (1, SIZE_MAX, false, std::forward<TParser> (t), std::forward<TSepParser> (sep_parser));
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany_char (std::size_t at_least, std::size_t at_most, TParser && t)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [at_least, at_most, t = std::forward<TParser> (t)] (state const & s, std::size_t position)
      {
        using tresult_type  = detail::strip_type_t<decltype (t (s, 0))> ;
        using tvalue_type   = typename tresult_type::value_type         ;
        using result_type   = result<std::string>                       ;

        static_assert (std::is_same<char, tvalue_type>::value, "Parser passed to pmany_chars must return value of type char");

        std::string values;
        values.reserve (at_least);

        auto current = position;

        auto cont = true;

        while (cont)
        {
          if (values.size () >= at_most)
          {
            cont = false;
            continue;
          }

          auto tv = t (s, current);
          if (!tv.value)
          {
            cont = false;
            continue;
          }

          values.push_back (std::move (tv.value.get ()));

          current = tv.position;
        }

        if (values.size () >= at_least)
        {
          return result_type::success (current, std::move (values));
        }
        else
        {
          return result_type::failure (current);
        }
      });
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany_char (TParser && t)
  {
    return pmany_char (0, SIZE_MAX, std::forward<TParser> (t));
  }

  template<typename TParser>
  CPP_PC__PRELUDE auto pmany_char1 (TParser && t)
  {
    return pmany_char (1, SIZE_MAX, std::forward<TParser> (t));
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
  CPP_PC__INLINE auto pbreakpoint (TParser && t)
  {
    CPP_PC__CHECK_PARSER (t);

    return detail::adapt_parser_function (
      [t = std::forward<TParser> (t)] (state const & s, std::size_t position)
      {
//        CPP_PC__ASSERT ("pbreakpoint" && false);
        auto tv = t (s, position);
        return tv;
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
        CPP_PC__CHECK_PARSER (head);
      }

      CPP_PC__INLINE result<TValue> parse (state const & s, std::size_t position, std::size_t & right_most) const
      {
        auto hv = head (s, position);
        right_most = std::max (hv.position, right_most);

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

      // TODO: Perfect forward
      CPP_PC__PRELUDE pchoice_impl (THead const & head, TTail const &... tail)
        : base_type (tail...)
        , head (head)
      {
        CPP_PC__CHECK_PARSER (head);
      }

      CPP_PC__INLINE result<TValue> parse (state const & s, std::size_t position, std::size_t & right_most) const
      {
        auto hv = head (s, position);
        right_most = std::max (hv.position, right_most);

        if (hv.value)
        {
          if (s.error_position == position)
          {
            // In order to collect error info
            base_type::parse (s, position, right_most);
          }
          return hv;
        }
        else
        {
          return base_type::parse (s, position, right_most);
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
        std::size_t right_most = 0;
        auto cv = impl.parse (s, position, right_most);

        if (!cv.value)
        {
          // This is in order to report the error on the furthest position on the right
          cv.reposition (right_most);
        }

        return cv;
      });
  }

  namespace detail
  {
    template<typename ...TParsers>
    struct ptuple_impl;

    template<>
    struct ptuple_impl<>
    {
      ptuple_impl () = default;

      CPP_PC__CTOR_COPY_MOVE (ptuple_impl);

      template<typename ...TTypes>
      CPP_PC__PRELUDE auto fail (std::size_t position) const
      {
        return result<std::tuple<TTypes...>>::failure (position);
      }

      template<typename ...TTypes>
      CPP_PC__PRELUDE auto parse (state const &, std::size_t position, TTypes const &... values) const
      {
        // TODO: Perfect forward
        return result<std::tuple<TTypes...>>::success (position, std::tuple<TTypes...> (values...));
      }

    };

    template<typename THead, typename... TTail>
    struct ptuple_impl<THead, TTail...> : ptuple_impl<TTail...>
    {
      using base_type   = ptuple_impl<TTail...>;
      using value_type  = detail::parser_value_type_t<THead>;

      CPP_PC__CTOR_COPY_MOVE (ptuple_impl);

      // TODO: Perfect forward
      CPP_PC__PRELUDE ptuple_impl (THead const & head, TTail const &... tail)
        : base_type (tail...)
        , head (head)
      {
        CPP_PC__CHECK_PARSER (head);
      }

      template<typename ...TTypes>
      CPP_PC__PRELUDE auto fail (std::size_t position) const
      {
        return base_type::template fail<TTypes..., value_type> (position);
      }

      template<typename ...TTypes>
      CPP_PC__INLINE auto parse (state const & s, std::size_t position, TTypes const &... values) const
      {
        auto hv = head (s, position);
        if (hv.value)
        {
          // TODO: Perfect forward
          return base_type::template parse<TTypes..., value_type> (s, hv.position, values..., hv.value.get ());
        }
        else
        {
          return fail<TTypes...> (position);
        }
      }

      THead head;
    };

  }

  template<typename ...TParsers>
  CPP_PC__INLINE auto ptuple (TParsers && ...parsers)
  {
    detail::ptuple_impl<TParsers...> impl (
        // TODO: Perfect forward
        parsers...
      );

    return detail::adapt_parser_function (
      [impl = std::move (impl)] (state const & s, std::size_t position)
      {
        return impl.parse (s, position);
      });
  }

  template<typename TBeginParser, typename TParser, typename TEndParser>
  CPP_PC__PRELUDE auto pbetween (
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
        using tvalue_type = detail::parser_value_type_t<TParser>;
        using result_type = result<tvalue_type>                 ;

        auto bv = begin_parser (s, position);
        if (!bv.value)
        {
          return result_type::failure (bv.position);
        }

        auto v = parser (s, bv.position);
        if (!v.value)
        {
          return v;
        }

        auto ev = end_parser (s, v.position);
        if (!ev.value)
        {
          return result_type::failure (ev.position);
        }

        return v
          .reposition (ev.position)
          ;
      });
  }

  template<typename TParser, typename TSepParser, typename TCombiner>
  CPP_PC__PRELUDE auto psep (
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
        auto v = parser (s, position);

        if (!v.value)
        {
          return v;
        }

        auto cont = true;

        while (cont)
        {
          auto sv = sep_parser (s, v.position);
          if (!sv.value)
          {
            cont = false;
            continue;
          }

          auto ov = parser (s, sv.position);
          if (!ov.value)
          {
            return ov
              ;
          }
          v.reposition (ov.position);

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
    return detail::adapt_parser_function (
      [error = detail::make_expected (std::move (expected)), at_least, at_most, satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state const & s, std::size_t position)
      {
        using result_type = result<sub_string>  ;

        auto result = s.satisfy (position, at_most, satisfy_function);
        auto consumed = result.size ();

        s.append_error (position + consumed, error);

        if (consumed < at_least)
        {
          return result_type::failure (position + consumed);
        }

        return result_type::success (position + consumed, std::move (result));
      });
  }

  template<typename TSatisfyFunction>
  CPP_PC__INLINE auto psatisfy_char (std::string expected, TSatisfyFunction && satisfy_function)
  {
    return detail::adapt_parser_function (
      [error = detail::make_expected (std::move (expected)), satisfy_function = std::forward<TSatisfyFunction> (satisfy_function)] (state const & s, std::size_t position)
      {
        using result_type = result<char>  ;

        s.append_error (position, error);

        auto peek = s.peek (position);
        if (peek == EOS)
        {
          return result_type::failure (position);
        }

        auto result = static_cast<char> (peek);

        if (!satisfy_function (0, result))
        {
          return result_type::failure (position);
        }

        return result_type::success (position + 1, result);
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
      return detail::make_expected (detail::char_to_string (ch));
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
        using result_type = result<char>  ;

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
          return result_type::failure (position);
        }

        auto find = expected.find (static_cast<char> (peek));
        if (find == std::string::npos)
        {
          return result_type::failure (position);
        }

        return result_type::success (position + 1, static_cast<char> (peek));
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
        using result_type = result<unit_type> ;

        s.append_error (position, error);

        auto peek = s.peek (position);
        if (peek == ch)
        {
          return result_type::success (position + 1, unit);
        }
        else
        {
          return result_type::failure (position);
        }
      });
  }

  CPP_PC__INLINE auto pskip_string (std::string str)
  {
    // TODO: Fix error position by doing full implementation
    auto sz = str.size ();
    auto expected =  '"' + str + '"';
    auto satisfy = [str = std::move (str)] (std::size_t pos, char ch)
      {
        CPP_PC__ASSERT (pos < str.size ());
        return ch == str[pos];
      };
    return pskip_satisfy (std::move (expected), sz, sz, std::move (satisfy));
  }

  auto const pskip_ws = pskip_satisfy ("whitespace", 0U, SIZE_MAX, satisfy_whitespace);

  namespace detail
  {
    auto const peos_error = detail::make_expected ("EOS");
  }

  auto const peos =
    detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        using result_type = result<unit_type> ;

        s.append_error (position, detail::peos_error);

        auto peek = s.peek (position);
        if (peek == EOS)
        {
          return result_type::success (position, unit);
        }
        else
        {
          return result_type::failure (position);
        }
      });

  namespace detail
  {
    auto const pdigit_error   = detail::make_expected ("digit");
    auto const pinteger_error = detail::make_expected ("integer");
  }

  auto const praw_uint64 =
    detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        using result_type = result<std::tuple<std::uint64_t, std::size_t>>;

        auto ss = s.satisfy (position, 20U, satisfy_digit);
        auto consumed = ss.size ();

        s.append_error (position + consumed, detail::pdigit_error);

        if (consumed == 0)
        {
          return result_type::failure (position + consumed);
        }

        std::uint64_t i = 0;
        for (auto iter = ss.begin; iter != ss.end; ++iter)
        {
          i = 10*i + static_cast<std::uint64_t> (*iter - '0');
        }

        return result_type::success (position + consumed, std::make_tuple (i, consumed));
      });

  auto const pint64 =
    detail::adapt_parser_function (
      [] (state const & s, std::size_t position)
      {
        using result_type = result<std::int64_t>;

        s.append_error (position, detail::pinteger_error);

        auto sign = 1         ;
        auto pos  = position  ;

        auto peek = s.peek (pos) ;
        switch (peek)
        {
        case EOS:
          return result_type::failure (position);
        case '+':
          ++pos;
          break;
        case '-':
          sign = -1;
          ++pos;
          break;
        default:

          break;
        }

        auto ss = s.satisfy (pos, 20U, satisfy_digit);
        auto consumed = ss.size ();

        if (consumed == 0)
        {
          return result_type::failure (pos + consumed);
        }

        std::int64_t i = 0;
        for (auto iter = ss.begin; iter != ss.end; ++iter)
        {
          i = 10*i + static_cast<std::int64_t> (*iter - '0');
        }

        i *= sign;

        return result_type::success (pos + consumed, i);
      });

  auto const puint64  = pmap (praw_uint64, [] (auto && v) { return std::get<0> (v); });
  auto const puint32  = pmap (praw_uint64, [] (auto && v) { return static_cast<std::uint32_t> (std::get<0> (v)); });

  auto const pint32   = pmap (pint64, [] (auto && v) { return static_cast<std::int32_t> (v); });
  auto const pint     = pint32;
}
// ----------------------------------------------------------------------------
