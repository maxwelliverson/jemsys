//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "fwd.hpp"

#include "wrapper.hpp"


namespace Agt {

  void*         ctxAllocLocal(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept;
  void          ctxFreeLocal(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  void*         ctxAllocLocalObject(AgtContext ctx, AgtSize size, AgtSize alignment, AgtObjectId& id) noexcept;
  void          ctxFreeLocalObject(AgtContext ctx, void* pObject, AgtSize size, AgtSize alignment, AgtObjectId id) noexcept;
  void*         ctxAllocSharedObject(AgtContext context, AgtSize size, AgtSize alignment, AgtObjectId& id) noexcept;
  void          ctxFreeSharedObject(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment, AgtObjectId id) noexcept;

  AgtStatus     ctxOpenHandleById(AgtContext ctx, AgtObjectId id, Handle*& handle) noexcept;
  AgtStatus     ctxOpenHandleByName(AgtContext ctx, const char* name, Handle*& handle) noexcept;

  AgtStatus     ctxRegisterNamedObject(AgtContext ctx, AgtObjectId id, const char* pName) noexcept;
  AgtStatus     ctxUnregisterNamedObject(AgtContext ctx, AgtObjectId id) noexcept;

  AgtStatus     ctxEnumerateNamedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;
  AgtStatus     ctxEnumerateSharedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;



  VPointer      ctxLookupVTable(AgtContext ctx, AgtTypeId typeId) noexcept;



  void*         ctxAllocAsyncData(AgtContext context, AgtObjectId& id) noexcept;
  void          ctxFreeAsyncData(AgtContext context, void* memory) noexcept;

  AgtStatus     createCtx(AgtContext& pCtx) noexcept;
  void          destroyCtx(AgtContext ctx) noexcept;
}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
