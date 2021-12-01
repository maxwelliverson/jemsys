//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"
#include "atomic.hpp"

#include "context.hpp"
#include "message.hpp"
#include "signal.hpp"
#include "objects.hpp"


#include <utility>
#include <cmath>


using namespace Agt;

namespace {
  enum class AsyncFlags : AgtUInt32 {
    eUnbound   = 0x0,
    eBound     = 0x1,
    eReady     = 0x2,
    eWaiting   = 0x4
  };

  JEM_forceinline constexpr AsyncFlags operator~(AsyncFlags a) noexcept {
    return static_cast<AsyncFlags>(~static_cast<AgtUInt32>(a));
  }
  JEM_forceinline constexpr AsyncFlags operator&(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) & static_cast<AgtUInt32>(b));
  }
  JEM_forceinline constexpr AsyncFlags operator|(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) | static_cast<AgtUInt32>(b));
  }
  JEM_forceinline constexpr AsyncFlags operator^(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) ^ static_cast<AgtUInt32>(b));
  }

  inline constexpr AsyncFlags eAsyncNoFlags          = {};
  inline constexpr AsyncFlags eAsyncUnbound          = AsyncFlags::eUnbound;
  inline constexpr AsyncFlags eAsyncBound            = AsyncFlags::eBound;
  inline constexpr AsyncFlags eAsyncReady            = AsyncFlags::eBound | AsyncFlags::eReady;
  inline constexpr AsyncFlags eAsyncWaiting          = AsyncFlags::eBound | AsyncFlags::eWaiting;
}


extern "C" {



struct AgtAsyncData_st {
  ContextId              contextId;
  ReferenceCount         refCount;
  std::atomic<AgtUInt32> waiterRefCount;
  ReferenceCount         attachedRefCount;
  AtomicMonotonicCounter responseCount;
  std::atomic<AgtUInt32> atomicKey;
  AgtUInt32              currentKey;
  AgtUInt32              maxResponseCount;
  bool                   isShared;
};

struct AgtAsync_st {
  AgtContext   context;
  AgtUInt32    desiredResponseCount;
  AsyncFlags   flags;
  AsyncState   state;
  AgtUInt32    dataKey;
  AgtAsyncData data;
};

}


namespace {







  void        asyncDataDoReset(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {
    key = ++data->currentKey;

    data->attachedRefCount = ReferenceCount(maxExpectedCount);
    data->responseCount    = AtomicMonotonicCounter();
    data->maxResponseCount = maxExpectedCount;
  }


  bool         asyncDataResetWaiter(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {

    AgtUInt32 expectedWaiterCount = 1;
    AgtUInt32 expectedNextWaiterCount = 1;

    while ( !data->waiterRefCount.compare_exchange_strong(expectedWaiterCount, expectedNextWaiterCount) ) {
      expectedNextWaiterCount = std::max(expectedWaiterCount - 1, 1u);
    }

    if (expectedNextWaiterCount != 1)
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }

  bool         asyncDataResetNonWaiter(AgtAsyncData data, AgtUInt32& key, AgtUInt32 maxExpectedCount) noexcept {

    AgtUInt32 expectedWaiterCount = 0;

    if ( !data->waiterRefCount.compare_exchange_strong(expectedWaiterCount, 1) )
      return false;

    asyncDataDoReset(data, key, maxExpectedCount);

    return true;
  }


  bool         asyncDataWait(AgtAsyncData data, AgtUInt32 expectedCount, AgtTimeout timeout) noexcept {
    if (expectedCount != 0) [[likely]] {
      if (!data->responseCount.waitFor(expectedCount, timeout))
        return false;
    }
    return true;
  }

  void         asyncDataDestroy(AgtAsyncData data, AgtContext context) noexcept {

    JEM_assert(data->waiterRefCount == 0);
    JEM_assert(data->attachedRefCount.get() == 0);

    if (!data->isShared) {
      ctxFreeAsyncData(context, data);
    }
    else {
      ctxRefFreeAsyncData(ctxGetContextById(context, data->contextId), data);
    }
  }

  AgtAsyncData createUnboundAsyncData(AgtContext context) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ReferenceCount(1),
      .waiterRefCount   = 0,
      .attachedRefCount = ReferenceCount(0),
      .responseCount    = AtomicMonotonicCounter(),
      .atomicKey        = { 0 },
      .currentKey       = 0,
      .maxResponseCount = 0,
      .isShared         = false
    };

    return data;
  }

  AgtAsyncData createAsyncData(AgtContext context, AgtUInt32 maxExpectedCount) noexcept {
    auto memory = ctxAllocAsyncData(context);

    auto data = new (memory) AgtAsyncData_st {
      .contextId        = ctxGetContextId(context),
      .refCount         = ReferenceCount(1),
      .waiterRefCount   = 1,
      .attachedRefCount = ReferenceCount(maxExpectedCount),
      .responseCount    = AtomicMonotonicCounter(),
      .atomicKey        = { 0 },
      .currentKey       = 0,
      .maxResponseCount = maxExpectedCount,
      .isShared         = false
    };

    return data;
  }



}






bool         Agt::asyncDataAttach(AgtAsyncData data, AgtContext ctx, AgtUInt32& key) noexcept {
  data->attachedRefCount.acquire();
  if (data->contextId != ctxGetContextId(ctx)) {
    data->isShared = true;
  }
  key = data->currentKey;
}

void         Agt::asyncDataArrive(AgtAsyncData data, AgtContext ctx, AgtUInt32 key) noexcept {
  if (data->currentKey == key) {
    ++data->responseCount;
  }

  if (data->refCount.release() == 0) {
    asyncDataDestroy(data, ctx);
  }
}











AgtContext   Agt::asyncGetContext(const AgtAsync_st* async) noexcept {
  return async->context;
}
AgtAsyncData Agt::asyncGetData(const AgtAsync_st* async) noexcept {
  return async->data;
}

void         Agt::asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync toAsync) noexcept {

}

AgtAsyncData Agt::asyncAttach(AgtAsync async, AgtSignal) noexcept {

}



void         Agt::asyncClear(AgtAsync async) noexcept {
  if ( static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  async->flags = async->flags & eAsyncBound;
}

void         Agt::asyncDestroy(AgtAsync async) noexcept {

  if ( static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting) ) {
    --async->data->waiterRefCount;
  }

  if ( async->data->refCount.release() == 0 ) {
    asyncDataDestroy(async->data, async->context);
  }

  delete async;
}

void         Agt::asyncReset(AgtAsync async, AgtUInt32 targetExpectedCount, AgtUInt32 maxExpectedCount) noexcept {

  const bool isWaiting = static_cast<AgtUInt32>(async->flags & AsyncFlags::eWaiting);

  if ( !( isWaiting ? asyncDataResetWaiter : asyncDataResetNonWaiter )(async->data, async->dataKey, maxExpectedCount) ) {
    if ( --async->data->refCount == 0 ) {
      async->data->waiterRefCount.store(1);
      asyncDataDoReset(async->data, async->dataKey, maxExpectedCount);
    }
    else {
      async->data = createAsyncData(async->context, maxExpectedCount);
    }
  }

  async->flags = AsyncFlags::eBound | AsyncFlags::eWaiting;
  async->desiredResponseCount = targetExpectedCount;
}

AgtStatus    Agt::asyncWait(AgtAsync async, AgtTimeout timeout) noexcept {

  if ( static_cast<AgtUInt32>(async->flags & eAsyncReady) )
    return AGT_SUCCESS;

  if ( !static_cast<AgtUInt32>(async->flags & eAsyncBound) )
    return AGT_ERROR_NOT_BOUND;

  if (asyncDataWait(async->data, async->desiredResponseCount, timeout)) {
    async->flags = async->flags | eAsyncReady;
    return AGT_SUCCESS;
  }

  return AGT_NOT_READY;
}

AgtAsync     Agt::createAsync(AgtContext context) noexcept {
  auto async = new AgtAsync_st;
  auto data = createUnboundAsyncData(context);

  async->context   = context;
  async->data      = data;
  async->dataKey   = data->currentKey;
  async->flags     = eAsyncUnbound;
  async->state     = AsyncState::eEmpty;
  async->desiredResponseCount = 0;

  return async;
}
