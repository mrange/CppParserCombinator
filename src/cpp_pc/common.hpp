#pragma once

#include <cassert>
#include <type_traits>

#define CPP_PC__PRELUDE constexpr
#define CPP_PC__INLINE  inline
#define CPP_PC__ASSERT(expr) assert(expr)

namespace cpp_pc
{
  namespace detail
  {
    template<typename T>
    using strip_type_t = std::remove_cv_t<std::remove_reference_t<T>>;
  }

  struct unit_type
  {
    CPP_PC__PRELUDE bool operator == (unit_type) const
    {
      return true;
    }
  };

  unit_type unit;
}