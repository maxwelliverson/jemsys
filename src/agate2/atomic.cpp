//
// Created by maxwe on 2021-12-02.
//

#include "atomic.hpp"


using namespace Agt;


AgtUInt64 Deadline::getCurrentTimestamp() noexcept {
  LARGE_INTEGER lrgInt;
  QueryPerformanceCounter(&lrgInt);
  return lrgInt.QuadPart;
}


bool ResponseCount::doDeepWait(AgtUInt32& capturedValue, AgtUInt32 timeout) const noexcept {
  _InterlockedIncrement(&deepSleepers_);
  BOOL result = WaitOnAddress(&const_cast<volatile AgtUInt32&>(arrivedCount_), &capturedValue, sizeof(AgtUInt32), timeout);
  _InterlockedDecrement(&deepSleepers_);
  if (result)
    capturedValue = fastLoad();
  return result;
}

void ResponseCount::notifyWaiters() noexcept {
  const AgtUInt32 waiters = __iso_volatile_load32(&reinterpret_cast<const int&>(deepSleepers_));

  if ( waiters == 0 ) [[likely]] {

  } else if ( waiters == 1 ) {
    WakeByAddressSingle(&arrivedCount_);
  } else {
    WakeByAddressAll(&arrivedCount_);
  }
}
