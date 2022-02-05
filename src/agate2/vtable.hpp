//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_VTABLE_HPP
#define JEMSYS_AGATE2_VTABLE_HPP


#include "fwd.hpp"



namespace Agt {

  struct VTable {
    AgtStatus (* const pfnAcquireMessage )(Object* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept;
    void      (* const pfnPushQueue )(Object* object, AgtMessage message, AgtSendFlags flags) noexcept;
    AgtStatus (* const pfnPopQueue )(Object* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;
    void      (* const pfnReleaseMessage )(Object* object, AgtMessage message) noexcept;
    AgtStatus (* const pfnConnect )(Object* object, Handle* handle, ConnectAction action) noexcept;
    AgtStatus (* const pfnAcquireRef )(Object* object) noexcept;
    AgtSize   (* const pfnReleaseRef )(Object* object) noexcept;
    void      (* const pfnDestroy )(Object* object) noexcept;
  };

  template <typename T>
  struct ObjectInfo{

    using HandleType = typename T::HandleType;
    using ObjectType = typename T::ObjectType;


    static AgtStatus acquireMessage(Object* object, AgtStagedMessage* pStagedMessage, AgtTimeout timeout) noexcept;
    static void      pushQueue(Object* object, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus popQueue(Object* object, AgtMessageInfo* pMessageInfo, AgtTimeout timeout) noexcept;
    static void      releaseMessage(Object* object, AgtMessage message) noexcept;
    static AgtStatus connect(Object* object, Handle* handle, ConnectAction action) noexcept;
    static AgtStatus acquireRef(Object* object) noexcept;
    static AgtSize   releaseRef(Object* object) noexcept;
    static void      destroy(Object* object) noexcept;
  };

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
