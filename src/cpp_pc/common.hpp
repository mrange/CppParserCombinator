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
#include <cassert>
#include <type_traits>
// ----------------------------------------------------------------------------
#define CPP_PC__PRELUDE constexpr
#define CPP_PC__INLINE  inline
#define CPP_PC__ASSERT(expr) assert (expr)

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
#define CPP_PC__CTOR_COPY_MOVE(cls)         \
  cls (cls const &)               = default;\
  cls (cls &&)                    = default;\
  cls & operator = (cls const &)  = delete ;\
  cls & operator = (cls &&)       = delete ;
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
