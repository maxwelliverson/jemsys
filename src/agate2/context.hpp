//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "fwd.hpp"

#include "wrapper.hpp"

namespace Agt {

  /*class Context : public Wrapper<Context, AgtContext> {
  public:

    AgtUInt32 getProcessId() const noexcept;

    AgtStatus registerHandle() const noexcept;

    bool registerPrivate(ObjectHeader* object) const noexcept;
    bool registerShared(ObjectHeader* object) const noexcept;

    void release(ObjectHeader* obj) const noexcept;
    void release(Id id) const noexcept;

    void releaseSharedObject(SharedObject* pSharedObject) const noexcept;

    ObjectHeader* lookup(Id id) const noexcept;
    ObjectHeader* unsafeLookup(Id id) const noexcept;

    void*         allocAsyncData() const noexcept;
    void          freeAsyncData(void* asyncData) const noexcept;


    SharedHandle* newSharedHandle(ObjectType type, ObjectFlags flags, AgtUInt32 pageId, AgtUInt32 pageOffset) const noexcept;
    SharedHandle* newSharedHandle(SharedHandle* pOtherHandle) const noexcept;
    void          destroySharedHandle(SharedHandle* pHandle) const noexcept;

  };*/

  using ContextId = AgtUInt64;


  ContextId     ctxGetContextId(const AgtContext_st* ctx) noexcept;
  AgtContextRef ctxGetContextById(const AgtContext_st* ctx, ContextId id) noexcept;

  void          ctxRefFreeAsyncData(AgtContextRef ctxRef, void* memory) noexcept;



  AgtUInt32     ctxGetProcessId(const AgtContext_st* ctx) noexcept;

  AgtStatus     ctxRegisterHandle(AgtContext ctx) noexcept;

  void          ctxReleaseObject(AgtContext ctx, ObjectHeader* object) noexcept;
  void          ctxReleaseObject(AgtContext ctx, Id id) noexcept;

  void          ctxReleaseLocalObject(AgtContext ctx, LocalObject* pLocalObject) noexcept;
  void          ctxReleaseSharedObject(AgtContext ctx, SharedObject* pSharedObject) noexcept;

  ObjectHeader* ctxLookupId(const AgtContext_st* ctx, Id id) noexcept;
  ObjectHeader* ctxUnsafeLookupId(const AgtContext_st* ctx, Id id) noexcept;



  void*         ctxAllocAsyncData(AgtContext context) noexcept;
  void          ctxFreeAsyncData(AgtContext context, void* memory) noexcept;

  void*         ctxAllocSharedObject(AgtContext context, AgtSize size, AgtSize alignment) noexcept;
  void          ctxFreeSharedObject(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  SharedHandle* ctxNewSharedHandle(AgtContext ctx, SharedObject* pObject) noexcept;
  void          ctxDestroySharedHandle(AgtContext ctx, SharedHandle* pHandle) noexcept;

  AgtStatus     createCtx(AgtContext& pCtx) noexcept;
}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
