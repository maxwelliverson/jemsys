//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_VTABLE_HPP
#define JEMSYS_AGATE2_VTABLE_HPP


#include "fwd.hpp"



namespace Agt {

  struct SharedVTable {
    AgtStatus (* const acquireRef)(SharedObject* object, AgtContext ctx) noexcept;
    AgtSize   (* const releaseRef)(SharedObject* object, AgtContext ctx) noexcept;
    void      (* const destroy)(SharedObject* object, AgtContext ctx) noexcept;
    AgtStatus (* const stage)(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    void      (* const send)(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    AgtStatus (* const receive)(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    AgtStatus (* const connect)(SharedObject* object, AgtContext ctx, Handle* handle, ConnectAction action) noexcept;
  };

  SharedVPtr lookupSharedVTable(ObjectType type) noexcept;

}

#endif//JEMSYS_AGATE2_VTABLE_HPP
