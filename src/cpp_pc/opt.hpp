#pragma once

#include <type_traits>

#include "common.hpp"

namespace cpp_pc
{
  struct empty_opt_type
  {
  };

  empty_opt_type empty_opt;

  template<typename TValue>
  struct opt
  {
    using value_type = TValue;

    CPP_PC__INLINE opt () noexcept
      : has_value (false)
    {
    }

    CPP_PC__INLINE opt (empty_opt_type) noexcept
      : has_value (false)
    {
    }

    CPP_PC__INLINE ~opt () noexcept
    {
      free_opt ();
    }

    CPP_PC__INLINE explicit opt (value_type const & v)
      : has_value (true)
    {
      new (&storage) value_type (v);
    }

    CPP_PC__INLINE explicit opt (value_type && v) noexcept
      : has_value (true)
    {
      new (&storage) value_type (std::move (v));
    }

    CPP_PC__INLINE opt (opt const & o)
      : has_value (o.has_value)
    {
      if (has_value)
      {
        new (&storage) value_type (o.get ());
      }
    }

    CPP_PC__INLINE opt (opt && o) noexcept
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

    CPP_PC__PRELUDE bool operator == (opt const & o) const
    {
      return
          has_value && o.has_value
        ? get () == o.get ()
        : has_value == o.has_value
        ;
    }

    CPP_PC__PRELUDE bool operator != (opt const & o) const
    {
      return !(*this == o);
    }

    CPP_PC__PRELUDE explicit operator bool () const noexcept
    {
      return has_value;
    }

    CPP_PC__PRELUDE bool is_empty () const noexcept
    {
      return !has_value;
    }

    CPP_PC__PRELUDE value_type const & get () const noexcept
    {
      return *get_ptr ();
    }

    CPP_PC__INLINE value_type const & get () noexcept
    {
      return *get_ptr ();
    }

  private:

    CPP_PC__PRELUDE value_type const * get_ptr () const noexcept
    {
      return reinterpret_cast<value_type const *> (&storage);
    }

    CPP_PC__INLINE value_type * get_ptr () noexcept
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
  CPP_PC__PRELUDE auto make_opt (T && v)
  {
    return opt<detail::strip_type_t<T>> (std::forward<T> (v));
  }
}