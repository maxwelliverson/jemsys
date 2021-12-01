//
// Created by maxwe on 2021-11-29.
//

#include "signal.hpp"

#include "context.hpp"
#include "async.hpp"

using namespace Agt;

extern "C" {

struct AgtSignal_st {

  AgtContext context;
  AgtAsyncData asyncData;
  AgtUInt32    dataEpoch;
  bool         isRaised;

};


}

AgtAsyncData Agt::signalGetAttachment(const AgtSignal_st* signal) noexcept {
  return signal->asyncData;
}

void         Agt::signalAttach(AgtSignal signal, AgtAsync async) noexcept {
  AgtAsyncData data = asyncGetData(async);

  if ( !asyncDataTryAttach(data, signal) ) {
    asyncClear(async);
    asyncAttach(async, signal);
  }

  if (signal->asyncData) {
    asyncDataDetachSignal(signal->asyncData);
  }
  signal->asyncData = data;
  signal->dataEpoch = asyncDataGetEpoch(data);
  signal->isRaised  = false;

}

void         Agt::signalDetach(AgtSignal signal) noexcept {
  if (signal->asyncData) {
    asyncDataDetachSignal(signal->asyncData);
    signal->asyncData = nullptr;
    signal->isRaised = false;
  }
}

void         Agt::signalRaise(AgtSignal signal) noexcept {
  if (!signal->isRaised) {
    asyncDataArrive(signal->asyncData, signal->dataEpoch);
    signal->isRaised = true;
  }
}

void         Agt::signalClose(AgtSignal signal) noexcept {

}
