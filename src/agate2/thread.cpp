//
// Created by maxwe on 2022-02-19.
//

#include "thread.hpp"
#include "channel.hpp"
#include "context.hpp"
#include "error.hpp"



#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>


using namespace Agt;

namespace {

  /** ========================= [ Thread Data ] =============================== */

  struct LocalBlockingThreadDataHeader {
    LocalBlockingThread*             threadHandle;
    LocalMpScChannel*                channel;
    PFN_agtBlockingThreadMessageProc msgProc;
  };

  struct LocalBlockingThreadInlineData : LocalBlockingThreadDataHeader {
    AgtSize                          structSize;
    std::byte                        userData[];
  };

  struct LocalBlockingThreadOutOfLineData  : LocalBlockingThreadDataHeader {
    PFN_agtUserDataDtor              userDataDtor;
    void*                            userData;
  };

  struct SharedBlockingThreadInlineData {
    SharedBlockingThread*      threadHandle;
    SharedMpScChannelReceiver* channel;
    PFN_agtBlockingThreadMessageProc msgProc;
    AgtSize                    structSize;
    std::byte                  userData[];
  };



  struct SharedBlockingThreadOutOfLineData {
    SharedBlockingThread*            threadHandle;
    SharedMpScChannelReceiver*       channel;
    PFN_agtBlockingThreadMessageProc msgProc;
    PFN_agtUserDataDtor              userDataDtor;
    void*                            userData;
  };



  /** ========================= [ Thread Cleanup ] =============================== */

  struct ThreadCleanupCallback {
    ThreadCleanupCallback* pNext;
    void (* pfnCleanupFunction )(void* pUserData);
    void* pUserData;
  };

  class ThreadCleanupCallbackStack {
    ThreadCleanupCallback* listHead = nullptr;

  public:

    constexpr ThreadCleanupCallbackStack() = default;

    ~ThreadCleanupCallbackStack() {
      ThreadCleanupCallback* currentCallback = listHead;
      ThreadCleanupCallback* nextCallback    = nullptr;
      while (currentCallback) {
        nextCallback = currentCallback->pNext;
        currentCallback->pfnCleanupFunction(currentCallback->pUserData);
        delete currentCallback;
        currentCallback = nextCallback;
      }
    }

    void pushCallback(void(*pfnFunc)(void*), void* pUserData) noexcept {
      auto callbackFrame = new ThreadCleanupCallback{
        .pNext              = listHead,
        .pfnCleanupFunction = pfnFunc,
        .pUserData          = pUserData
      };
      listHead = callbackFrame;
    }
  };

  constinit thread_local ThreadCleanupCallbackStack ThreadCleanupCallbacks{};


  inline void callOnThreadExit(void(* pfnFunc)(void*), void* pUserData) noexcept {
    ThreadCleanupCallbacks.pushCallback(pfnFunc, pUserData);
  }


  template <typename Data>
  void threadDataDestructor(void* pData);


  template <>
  void threadDataDestructor<LocalBlockingThreadInlineData>(void* pData) {
    const auto data = static_cast<LocalBlockingThreadInlineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(LocalBlockingThreadInlineData));
  }
  template <>
  void threadDataDestructor<SharedBlockingThreadInlineData>(void* pData) {
    const auto data = static_cast<SharedBlockingThreadInlineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    ctxLocalFree(context, data, data->structSize, alignof(SharedBlockingThreadInlineData));
  }
  template <>
  void threadDataDestructor<LocalBlockingThreadOutOfLineData>(void* pData) {
    const auto data = static_cast<LocalBlockingThreadOutOfLineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(LocalBlockingThreadOutOfLineData),
                 alignof(LocalBlockingThreadOutOfLineData));
  }
  template <>
  void threadDataDestructor<SharedBlockingThreadOutOfLineData>(void* pData) {
    const auto data = static_cast<SharedBlockingThreadOutOfLineData*>(pData);
    AgtContext context = data->threadHandle->context;

    handleSetErrorState(data->channel, ErrorState::noReceivers);
    handleReleaseRef(data->channel);
    handleReleaseRef(data->threadHandle);

    if (data->userDataDtor)
      data->userDataDtor(data->userData);

    ctxLocalFree(context, data,
                 sizeof(SharedBlockingThreadOutOfLineData),
                 alignof(SharedBlockingThreadOutOfLineData));
  }



  /** ========================= [ Thread Creation ] =============================== */

#if JEM_system_windows

  DWORD __stdcall localBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<LocalBlockingThreadInlineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;


    callOnThreadExit(threadDataDestructor<LocalBlockingThreadInlineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
      // TODO: Potential Bug, only handles AGT_SUCCESS and AGT_ERROR_NO_SENDERS. If channel is caught in some other state,
      //       this will either block indefinitely, or will get caught in an infinite loop. This is true for all 4 thread routine functions
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadInlineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<SharedBlockingThreadInlineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<SharedBlockingThreadInlineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall localBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<LocalBlockingThreadOutOfLineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<LocalBlockingThreadOutOfLineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }
  DWORD __stdcall sharedBlockingThreadOutOfLineStartRoutine(LPVOID threadParam) {
    const auto threadData = static_cast<SharedBlockingThreadOutOfLineData*>(threadParam);
    const auto threadHandle = threadData->threadHandle;
    const auto channel = threadData->channel;
    const auto msgProc = threadData->msgProc;
    void* const pUserData = threadData->userData;

    callOnThreadExit(threadDataDestructor<SharedBlockingThreadOutOfLineData>, threadParam);

    AgtMessageInfo messageInfo;
    AgtStatus status;

    do {
      status = Agt::handlePopQueue(channel, &messageInfo, AGT_WAIT);
      if ( status == AGT_SUCCESS ) {
        msgProc(threadHandle, &messageInfo, pUserData);
      }
    } while( status != AGT_ERROR_NO_SENDERS );

    return NO_ERROR;
  }





  HANDLE CreateThread(
    [in, optional]  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    [in]            SIZE_T                  dwStackSize,
    [in]            LPTHREAD_START_ROUTINE  lpStartAddress,
    [in, optional]  __drv_aliasesMem LPVOID lpParameter,
    [in]            DWORD                   dwCreationFlags,
    [out, optional] LPDWORD                 lpThreadId
  );
#else
#endif
}


AgtStatus Agt::createInstance(LocalBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept {
  LocalBlockingThreadDataHeader* header;
  LPTHREAD_START_ROUTINE startRoutine;
  AgtStatus status;
  if (createInfo.flags & AGT_BLOCKING_THREAD_COPY_USER_DATA) {
    AgtSize dataLength = (createInfo.flags & AGT_BLOCKING_THREAD_USER_DATA_STRING)
                           ? (strlen((const char*)createInfo.pUserData) + 1)
                           : createInfo.dataSize;
    AgtSize structSize = dataLength + sizeof(LocalBlockingThreadInlineData);
    auto data = (LocalBlockingThreadInlineData*)ctxLocalAlloc(ctx, structSize, alignof(LocalBlockingThreadInlineData));
    data->structSize = structSize;
    std::memcpy(data->userData, createInfo.pUserData, dataLength);
    header = data;
    startRoutine = &localBlockingThreadInlineStartRoutine;
  } else {
    auto data = (LocalBlockingThreadOutOfLineData*)ctxLocalAlloc(ctx, sizeof(LocalBlockingThreadOutOfLineData), alignof(LocalBlockingThreadOutOfLineData));
    data->userData = createInfo.pUserData;
    data->userDataDtor = createInfo.pfnUserDataDtor;
    header = data;
    startRoutine = &localBlockingThreadOutOfLineStartRoutine;
  }
  header->msgProc = createInfo.pfnMessageProc;

  AgtChannelCreateInfo channelCreateInfo;
  channelCreateInfo.scope = AGT_SCOPE_LOCAL;
  channelCreateInfo.maxReceivers = 1;
  channelCreateInfo.maxSenders = 0;
  //channelCreateInfo.name


  status = createInstance(header->channel, ctx, );
}

AgtStatus Agt::createInstance(SharedBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept {

}
