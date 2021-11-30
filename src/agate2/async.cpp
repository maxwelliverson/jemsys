//
// Created by maxwe on 2021-11-28.
//

#include "async.hpp"
#include "atomic.hpp"

#include "context.hpp"
#include "message.hpp"
#include "signal.hpp"


using namespace Agt;


extern "C" {



struct AgtAsyncData_st {
  // std::atomic<AsyncDataState> state;
  ReferenceCount              refCount;
  AtomicMonotonicCounter      responseCount;
  AgtUInt32                   maxResponseCount;
  AsyncType                   type;
  union {
    AgtSignal  signal;
    AgtMessage message;
  };
};

struct AgtAsync_st {
  AgtContext   context;
  AgtUInt32    desiredResponseCount;
  AsyncFlags   flags;
  AsyncState   state;
  AgtAsyncData data;
};

}


AsyncType    Agt::asyncDataGetType(const AgtAsyncData_st* asyncData) noexcept {
  return asyncData->type;
}

void         Agt::asyncDataReset(AgtAsyncData data, AgtUInt32 expectedCount, AgtMessage message) noexcept {}
void         Agt::asyncDataReset(AgtAsyncData data, AgtSignal signal) noexcept {}

void         Agt::asyncDataAttach(AgtAsyncData data, AgtSignal signal) noexcept {}
bool         Agt::asyncDataTryAttach(AgtAsyncData data, AgtSignal signal) noexcept {}

void         Agt::asyncDataDetachSignal(AgtAsyncData data) noexcept {}

void         Agt::asyncDataAttach(AgtAsyncData data) noexcept {}

AgtUInt32    Agt::asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept {
  return data->maxResponseCount;
}

AsyncFlags   Agt::asyncDataWait(AgtAsyncData data, AgtUInt32 expectedCount, AgtTimeout timeout) noexcept {
  if (data->type == AsyncType::eUnbound) [[unlikely]] {
    return eAsyncUnbound;
  }

  if (expectedCount != 0) [[likely]] {
    if (!data->responseCount.waitFor(expectedCount, timeout))
      return eAsyncNotReady;
  }
  if (data->refCount.release() == 0) {
    return eAsyncReadyAndReusable;
  }
  return eAsyncReady;
}

AgtAsyncData Agt::createAsyncData(AgtContext context) noexcept {
  auto dataMemory      = ctxAllocAsyncData(context);
  return new(dataMemory) AgtAsyncData_st {
    .refCount         = ReferenceCount(1),
    .responseCount    = {},
    .maxResponseCount = 0,
    .type             = AsyncType::eUnbound
  };
}


AgtContext   Agt::asyncGetContext(const AgtAsync_st* async) noexcept {
  return async->context;
}
AgtAsyncData Agt::asyncGetData(const AgtAsync_st* async) noexcept {

}

void         Agt::asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync_st* toAsync) noexcept {

}
void         Agt::asyncClear(AgtAsync async) noexcept {

}
void         Agt::asyncDestroy(AgtAsync async) noexcept {

}

AgtStatus    Agt::asyncWait(AgtAsync async, AgtTimeout timeout) noexcept {

}

AgtAsync     Agt::createAsync(AgtContext context) noexcept {
  auto async     = new AgtAsync_st;

  async->context = context;
  async->data    = createAsyncData(context);
  async->flags   = eAsyncUnbound;
  async->state   = AsyncState::eEmpty;
  async->desiredResponseCount = 0;
  return async;
}





AsyncType AsyncData::getType() const noexcept {
  return value->type;
}

void AsyncData::reset(AgtUInt32 expectedCount, Message message) const noexcept {

}

void AsyncData::reset(Signal signal) const noexcept {

}

void AsyncData::arrive() const noexcept {
  if (++value->responseCount == value->)
}

AgtUInt32 AsyncData::getMaxExpectedCount() const noexcept {
  return value->maxResponseCount;
}

AsyncFlags AsyncData::wait(AgtUInt32 expectedCount, AgtTimeout timeout) const noexcept {

  if (value->type == AsyncType::eUnbound) [[unlikely]] {
    return eAsyncUnbound;
  }

  if (expectedCount != 0) [[likely]] {
    if (!value->responseCount.waitFor(expectedCount, timeout))
      return eAsyncNotReady;
  }
  if (value->refCount.release() == 0) {
    return eAsyncReadyAndReusable;
  }
  return eAsyncReady;
}



void Async::clear() const noexcept {

}

void Async::destroy() const noexcept {

}

void Async::copyTo(Async other) const noexcept {

}

Context   Async::getContext() const noexcept {
  return value->context;
}

AsyncData Async::getData() const noexcept {
  return value->data;
}



AgtStatus Async::wait(AgtTimeout timeout) const noexcept {

  AsyncFlags result = value->data.wait();



  return AGT_SUCCESS;
}

Async Async::create(Context ctx) noexcept {
  auto async     = new AgtAsync_st;
  auto data      = createAsyncData(ctx);

  async->context = ctx;
  async->data    = AsyncData::wrap(data);
  async->flags   = eAsyncUnbound;
  async->state   = AsyncState::eEmpty;
  async->desiredResponseCount = 0;
  return Async::wrap(async);
}
