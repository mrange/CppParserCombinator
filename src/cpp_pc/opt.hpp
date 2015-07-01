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
#include <type_traits>
// ----------------------------------------------------------------------------
#include "common.hpp"
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
namespace cpp_pc
{
  struct empty_opt_type
  {
    CPP_PC__PRELUDE empty_opt_type ()
    {
    }
  };

  CPP_PC__PRELUDE empty_opt_type const empty_opt;

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
      clear ();
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
      : has_value (o.has_value)
    {
      if (has_value)
      {
        new (&storage) value_type (std::move (o.get ()));
        o.has_value = false;
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

      clear ();

      has_value = o.has_value;

      if (has_value)
      {
        new (&storage) value_type (std::move (o.get ()));
        o.has_value = false;
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
      CPP_PC__ASSERT (has_value);
      return *get_ptr ();
    }

    CPP_PC__INLINE value_type const & get () noexcept
    {
      CPP_PC__ASSERT (has_value);
      return *get_ptr ();
    }

    void clear () noexcept
    {
      if (has_value)
      {
        get ().~value_type ();
        has_value = false;
      }
    }

    TValue const & coalesce (TValue const & v) const noexcept
    {
      if (has_value)
      {
        return get ();
      }
      else
      {
        return v;
      }
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

    alignas (value_type) unsigned char storage[sizeof (value_type)];
    bool has_value;
  };

  template<typename T>
  CPP_PC__PRELUDE auto make_opt (T && v)
  {
    return opt<detail::strip_type_t<T>> (std::forward<T> (v));
  }
}
// ----------------------------------------------------------------------------
