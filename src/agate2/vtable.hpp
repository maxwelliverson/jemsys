//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_VTABLE_HPP
#define JEMSYS_AGATE2_VTABLE_HPP


#include "fwd.hpp"

namespace Agt {

  struct LocalVTable {
    AgtStatus(* acquireRef)(LocalObject* object);
    bool     (* releaseRef)(LocalObject* object);
  };

  struct SharedVTable {
    AgtStatus(* acquireRef)(SharedObject* object);
    bool     (* releaseRef)(SharedObject* object);
  };

}

#endif//JEMSYS_AGATE2_VTABLE_HPP
