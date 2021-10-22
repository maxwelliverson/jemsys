//
// Created by maxwe on 2021-10-22.
//



#include "internal.hpp"

#include <agate.h>

namespace qtz {
  static constinit thread_local agt_agent_t tl_self = nullptr;
}

extern "C" {
  JEM_api void          JEM_stdcall agt_set_self(agt_agent_t self) JEM_noexcept {
    qtz::tl_self = self;
  }
  JEM_api agt_agent_t   JEM_stdcall agt_self() JEM_noexcept {
    return qtz::tl_self;
  }
}
