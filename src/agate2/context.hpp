//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_CONTEXT_HPP
#define JEMSYS_AGATE2_CONTEXT_HPP

#include "fwd.hpp"

#include "wrapper.hpp"


namespace Agt {

  enum class BuiltinValue : AgtUInt32 {
    processName,
    defaultPrivateChannelMessageSize,
    defaultLocalChannelMessageSize,
    defaultSharedChannelMessageSize,
    defaultPrivateChannelSlotCount,
    defaultLocalChannelSlotCount,
    defaultSharedChannelSlotCount
  };


  void*         ctxLocalAlloc(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept;
  void          ctxLocalFree(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  void*         ctxSharedAlloc(AgtContext ctx, AgtSize size, AgtSize alignment, SharedAllocationId& allocationId) noexcept;
  void          ctxSharedFree(AgtContext ctx, SharedAllocationId allocId) noexcept;
  void          ctxSharedFree(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept;

  HandleHeader* ctxAllocHandle(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept;
  void          ctxFreeHandle(AgtContext ctx, HandleHeader* handle, AgtSize size, AgtSize alignment) noexcept;

  SharedObjectHeader* ctxAllocSharedObject(AgtContext context, AgtSize size, AgtSize alignment) noexcept;
  void                ctxFreeSharedObject(AgtContext ctx, SharedObjectHeader* object, AgtSize size, AgtSize alignment) noexcept;

  // void*         ctxAllocAsyncData(AgtContext context, AgtObjectId& id) noexcept;
  // void          ctxFreeAsyncData(AgtContext context, void* memory) noexcept;




  void*         ctxGetLocalAddress(SharedAllocationId allocId) noexcept;

  AgtStatus     ctxOpenHandleById(AgtContext ctx, AgtObjectId id, HandleHeader*& handle) noexcept;
  AgtStatus     ctxOpenHandleByName(AgtContext ctx, const char* name, HandleHeader*& handle) noexcept;

  // TODO: Optimize interface.
  //  Name clashes and bad encoding are detected by ctxRegisterNamedObject, which will fail in those cases.
  //  However, by the current design, objects are made publicly visible as soon as they are registered,
  //  which therefore requires that only fully initialized objects be registered. This means that everytime
  //  an API call is made that attempts to create an object with an invalid name, the entire (potentially costly)
  //  initialization process takes place, only to be immediately undone when the error is detected. This is
  //  undesirable and unnecessarily wasteful, given that the naming errors can be detected before object
  //  initialization even begins.
  //  Potential Fix: make ctxRegisterNamedObject detect errors and lay claim to a name for the purposes of
  //  clash detection, but not yet make the object publicly visible. Add a second function that would be used
  //  to indicate that a registered object is now in a valid state, and can be made visible. Finally, add a
  //  third function (or modify ctxUnregisterNamedObject?) to indicate that object creation failed, and the
  //  name can be freed.
  AgtStatus     ctxRegisterNamedObject(AgtContext ctx, HandleHeader* handle, const char* pName) noexcept;
  AgtStatus     ctxUnregisterNamedObject(AgtContext ctx, AgtObjectId id) noexcept;

  AgtStatus     ctxEnumerateNamedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;
  AgtStatus     ctxEnumerateSharedObjects(AgtContext ctx, AgtSize& count, const char** pNames) noexcept;



  VPointer      ctxLookupVTable(AgtContext ctx, AgtTypeId typeId) noexcept;

  bool          ctxSetBuiltinValue(AgtContext ctx, BuiltinValue value, const void* data, AgtSize dataSize) noexcept;
  bool          ctxGetBuiltinValue(AgtContext ctx, BuiltinValue value, void* data, AgtSize& outSize) noexcept;

  AgtStatus     createCtx(AgtContext& pCtx) noexcept;
  void          destroyCtx(AgtContext ctx) noexcept;



}

#endif//JEMSYS_AGATE2_CONTEXT_HPP
