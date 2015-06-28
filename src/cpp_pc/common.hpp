#pragma once

#include <cassert>
#include <type_traits>

#define CPP_PC__PRELUDE constexpr
#define CPP_PC__INLINE  inline
#define CPP_PC__ASSERT(expr) assert(expr)

#define CPP_PC__NO_COPY_MOVE(cls)           \
  cls (cls const &)               = delete ;\
  cls (cls &&)                    = delete ;\
  cls & operator = (cls const &)  = delete ;\
  cls & operator = (cls &&)       = delete ;

#define CPP_PC__COPY_MOVE(cls)              \
  cls (cls const &)               = default;\
  cls (cls &&)                    = default;\
  cls & operator = (cls const &)  = default;\
  cls & operator = (cls &&)       = default;


namespace cpp_pc
{
  namespace detail
  {
    template<typename T>
    using strip_type_t = std::remove_cv_t<std::remove_reference_t<T>>;
  }

  struct unit_type
  {
    CPP_PC__PRELUDE unit_type ()
    {
    }

    CPP_PC__PRELUDE bool operator == (unit_type) const
    {
      return true;
    }
  };

  CPP_PC__PRELUDE unit_type const unit;
}