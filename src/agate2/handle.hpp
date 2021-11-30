//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_HANDLE_HPP
#define JEMSYS_AGATE2_HANDLE_HPP

#include <agate2.h>

#include "fwd.hpp"
#include "objects.hpp"
#include "context.hpp"
#include "wrapper.hpp"

namespace Agt {

  struct SharedHandle : HandleHeader {
    SharedVPtr    vtable;
    SharedObject* object;
  };

  class Handle : public Wrapper<Handle, AgtHandle, HandleHeader*> {

    JEM_forceinline ObjectHeader* getObject() const noexcept {
      return isShared() ? (ObjectHeader*)(static_cast<SharedHandle*>(value)->object) : (ObjectHeader*)value;
    }


    JEM_forceinline SharedHandle* asShared() const noexcept {
      return (SharedHandle*)value;
    }

    JEM_forceinline LocalObject* asLocal() const noexcept {
      return (LocalObject*)value;
    }

  public:

    JEM_forceinline Id getLocalId() const noexcept {
      return value->localId;
    }
    JEM_forceinline Id getExportId() const noexcept {
      return isShared() ? static_cast<SharedHandle*>(value)->object->exportId : Id::invalid();
    }

    JEM_forceinline ObjectType getObjectType() const noexcept {
      return value->type;
    }
    JEM_forceinline ObjectFlags getObjectFlags() const noexcept {
      return value->flags;
    }

    JEM_forceinline Context getContext() const noexcept {
      return value->context;
    }


    JEM_forceinline bool isShared() const noexcept {
      return static_cast<std::underlying_type_t<ObjectType>>(value->flags & ObjectFlags::isShared);
    }


    void close() const noexcept;

    AgtStatus duplicate(AgtHandle* pOut) const noexcept;

  };



}

#endif//JEMSYS_AGATE2_HANDLE_HPP
