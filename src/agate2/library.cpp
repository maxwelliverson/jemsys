//
// Created by maxwe on 2021-11-23.
//

#include "internal.hpp"
#include "handle.hpp"

#include "async.hpp"
#include "context.hpp"
#include "signal.hpp"


using namespace Agt;


extern "C" {

/*struct AgtContext_st {

  AgtUInt32                processId;

  SharedPageMap            sharedPageMap;
  PageList                 localFreeList;
  ListNode*                emptyLocalPage;

  AgtSize                  localPageSize;
  AgtSize                  localEntryCount;

  ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)> handlePool;

  IpcBlock                 ipcBlock;

};*/





}




JEM_noinline static AgtStatus agtGetSharedObjectInfoById(AgtContext context, AgtObjectInfo* pObjectInfo) noexcept;


extern "C" {



JEM_api AgtStatus     JEM_stdcall agtNewContext(AgtContext* pContext) JEM_noexcept {

  if (!pContext)
    return AGT_ERROR_INVALID_ARGUMENT;

  return createCtx(*pContext);
}

JEM_api AgtStatus     JEM_stdcall agtDestroyContext(AgtContext context) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}


JEM_api AgtStatus     JEM_stdcall agtGetObjectInfo(AgtContext context, AgtObjectInfo* pObjectInfo) JEM_noexcept {
  if (!pObjectInfo) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  if (pObjectInfo->id != 0) {

    if (!context) [[unlikely]] {
      return AGT_ERROR_INVALID_ARGUMENT;
    }

    Agt::Id id = Agt::Id::convert(pObjectInfo->id);

    if (!id.isShared()) [[likely]] {
      if (id.getProcessId() != ctxGetProcessId(context)) {
        return AGT_ERROR_UNKNOWN_FOREIGN_OBJECT;
      }
    }
    else {
      return agtGetSharedObjectInfoById(context, pObjectInfo);
    }

    LocalObject* object;

    if (auto objectHeader = ctxLookupId(context, id))
      object = reinterpret_cast<LocalObject*>(objectHeader);
    else
      return AGT_ERROR_EXPIRED_OBJECT_ID;

    pObjectInfo->handle = object;
    pObjectInfo->exportId = AGT_INVALID_OBJECT_ID;
    pObjectInfo->flags = toHandleFlags(object->flags);
    pObjectInfo->type = toHandleType(object->type);

  } else if (pObjectInfo->handle != nullptr)  {
    Handle handle = Handle::wrap(pObjectInfo->handle);

    pObjectInfo->type     = toHandleType(handle.getObjectType());
    pObjectInfo->flags    = toHandleFlags(handle.getObjectFlags());
    pObjectInfo->id       = handle.getLocalId().toRaw();
    pObjectInfo->exportId = handle.getExportId().toRaw();

  } else [[unlikely]] {
    return AGT_ERROR_INVALID_OBJECT_ID;
  }

  return AGT_SUCCESS;

}

JEM_api AgtStatus     JEM_stdcall agtDuplicateHandle(AgtHandle inHandle, AgtHandle* pOutHandle) JEM_noexcept {
  if (!pOutHandle) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  if (inHandle == AGT_NULL_HANDLE) [[unlikely]] {
    *pOutHandle = AGT_NULL_HANDLE;
    return AGT_ERROR_NULL_HANDLE;
  }

  return Handle::wrap(inHandle).duplicate(pOutHandle);
}

JEM_api void          JEM_stdcall agtCloseHandle(AgtHandle handle) JEM_noexcept {
  if (handle) [[likely]] {
    Handle::wrap(handle).close();
  }
}



JEM_api AgtStatus     JEM_stdcall agtCreateChannel(const AgtChannelCreateInfo* cpCreateInfo, AgtHandle* pSender, AgtHandle* pReceiver) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateAgent(const AgtAgentCreateInfo* cpCreateInfo, AgtHandle* pAgent) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateAgency(const AgtAgencyCreateInfo* cpCreateInfo, AgtHandle* pAgency) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateThread(const AgtThreadCreateInfo* cpCreateInfo, AgtHandle* pThread) JEM_noexcept;



JEM_api AgtStatus     JEM_stdcall agtStage(AgtHandle sender, AgtStagedMessage* pStagedMessage, AgtSize messageSize, AgtTimeout usTimeout) JEM_noexcept;
JEM_api void          JEM_stdcall agtSendStaged(const AgtStagedMessage* cpStagedMessage, AgtHandle sender, AgtAsync asyncHandle, AgtSendFlags flags) JEM_noexcept;

JEM_api AgtStatus     JEM_stdcall agtSend(AgtHandle sender, const AgtSendInfo* pSendInfo, AgtTimeout timeout) JEM_noexcept {

}

JEM_api AgtStatus     JEM_stdcall agtReceive(AgtHandle receiver, AgtMessageInfo* pMessageInfo, AgtTimeout usTimeout) JEM_noexcept {

}



JEM_api void          JEM_stdcall agtReturn(AgtMessage message, AgtStatus status) JEM_noexcept {

}


JEM_api void          JEM_stdcall agtYieldExecution() JEM_noexcept;

JEM_api AgtStatus     JEM_stdcall agtDispatchMessage(const AgtActor* pActor, const AgtMessageInfo* pMessageInfo) JEM_noexcept;
// JEM_api AgtStatus     JEM_stdcall agtExecuteOnThread(AgtThread thread, ) JEM_noexcept;





JEM_api AgtStatus     JEM_stdcall agtGetSenderHandle(AgtMessage message, AgtHandle* pSenderHandle) JEM_noexcept;




/* ========================= [ Async ] ========================= */

JEM_api AgtStatus     JEM_stdcall agtNewAsync(AgtContext ctx, AgtAsync* pAsync) JEM_noexcept {

  if (!pAsync) [[unlikely]] {
    return AGT_ERROR_INVALID_ARGUMENT;
  }

  // TODO: Detect bad context?

  *pAsync = createAsync(ctx);

  return AGT_SUCCESS;
}

JEM_api void          JEM_stdcall agtCopyAsync(AgtAsync from, AgtAsync to) JEM_noexcept {
  asyncCopyTo(from, to);
}

JEM_api void          JEM_stdcall agtClearAsync(AgtAsync async) JEM_noexcept {
  asyncClear(async);
}

JEM_api void          JEM_stdcall agtDestroyAsync(AgtAsync async) JEM_noexcept {
  asyncDestroy(async);
}

JEM_api AgtStatus     JEM_stdcall agtWait(AgtAsync async, AgtTimeout timeout) JEM_noexcept {
  return asyncWait(async, timeout);
}

JEM_api AgtStatus     JEM_stdcall agtWaitMany(const AgtAsync* pAsyncs, AgtSize asyncCount, AgtTimeout timeout) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}




/* ========================= [ Signal ] ========================= */

JEM_api AgtStatus     JEM_stdcall agtNewSignal(AgtContext ctx, AgtSignal* pSignal) JEM_noexcept {

}

JEM_api void          JEM_stdcall agtAttachSignal(AgtSignal signal, AgtAsync async) JEM_noexcept {

}

JEM_api void          JEM_stdcall agtRaiseSignal(AgtSignal signal) JEM_noexcept {

}

JEM_api void          JEM_stdcall agtRaiseManySignals(AgtSignal signal) JEM_noexcept {

}

JEM_api void          JEM_stdcall agtDestroySignal(AgtSignal signal) JEM_noexcept {

}


}