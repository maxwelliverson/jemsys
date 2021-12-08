//
// Created by maxwe on 2021-12-02.
//

#include "atomic.hpp"

#include <intrin.h>


using namespace Agt;



AgtUInt8  Impl::atomicLoad(const AgtUInt8& value) noexcept {
  return (AgtUInt8)_InterlockedCompareExchange8((char*)&value, 0, 0);
}
AgtUInt16 Impl::atomicLoad(const AgtUInt16& value) noexcept {
  return (AgtUInt16)_InterlockedCompareExchange16((AgtInt16*)&value, 0, 0);
}
AgtUInt32 Impl::atomicLoad(const AgtUInt32& value) noexcept {
  return _InterlockedCompareExchange((AgtUInt32*)&value, 0, 0);
}
AgtUInt64 Impl::atomicLoad(const AgtUInt64& value) noexcept {
  return _InterlockedCompareExchange((AgtUInt64*)&value, 0, 0);
}

AgtUInt8 Impl::atomicRelaxedLoad(const AgtUInt8& value) noexcept {
  return __iso_volatile_load8((const char*)&value);
}
AgtUInt16 Impl::atomicRelaxedLoad(const AgtUInt16& value) noexcept {
  return __iso_volatile_load16((const AgtInt16*)&value);
}
AgtUInt32 Impl::atomicRelaxedLoad(const AgtUInt32& value) noexcept {
  return __iso_volatile_load32((const AgtInt32*)&value);
}
AgtUInt64 Impl::atomicRelaxedLoad(const AgtUInt64& value) noexcept {
  return __iso_volatile_load64((const AgtInt64*)&value);
}

void Impl::atomicStore(AgtUInt8& value,  AgtUInt8 newValue) noexcept {
  Impl::atomicExchange(value, newValue);
}
void Impl::atomicStore(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  Impl::atomicExchange(value, newValue);
}
void Impl::atomicStore(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  Impl::atomicExchange(value, newValue);
}
void Impl::atomicStore(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  Impl::atomicExchange(value, newValue);
}

void Impl::atomicRelaxedStore(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  __iso_volatile_store8((char*)&value, (char)newValue);
}
void Impl::atomicRelaxedStore(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  __iso_volatile_store16((AgtInt16*)&value, (AgtInt16)newValue);
}
void Impl::atomicRelaxedStore(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  __iso_volatile_store32((AgtInt32*)&value, (AgtInt32)newValue);
}
void Impl::atomicRelaxedStore(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  __iso_volatile_store64((AgtInt64*)&value, (AgtInt64)newValue);
}

AgtUInt8  Impl::atomicExchange(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  return (AgtUInt8)_InterlockedExchange8((char*)&value, (char)newValue);
}
AgtUInt16 Impl::atomicExchange(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  return (AgtUInt16)_InterlockedExchange16((AgtInt16*)&value, (AgtInt16)newValue);
}
AgtUInt32 Impl::atomicExchange(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  return _InterlockedExchange(&value, newValue);
}
AgtUInt64 Impl::atomicExchange(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  return _InterlockedExchange(&value, newValue);
}

bool Impl::atomicCompareExchange(AgtUInt8& value,  AgtUInt8& compare, AgtUInt8 newValue)    noexcept {
  AgtUInt8 oldComp = compare;
  return (compare = (AgtUInt8)_InterlockedCompareExchange8((char*)&value, (char)newValue, (char)compare)) == oldComp;
}
bool Impl::atomicCompareExchange(AgtUInt16& value, AgtUInt16& compare, AgtUInt16 newValue) noexcept {
  AgtUInt16 oldComp = compare;
  return (compare = (AgtUInt16)_InterlockedCompareExchange16((AgtInt16*)&value, (AgtInt16)newValue, (AgtInt16)compare)) == oldComp;
}
bool Impl::atomicCompareExchange(AgtUInt32& value, AgtUInt32& compare, AgtUInt32 newValue) noexcept {
  AgtUInt32 oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
}
bool Impl::atomicCompareExchange(AgtUInt64& value, AgtUInt64& compare, AgtUInt64 newValue) noexcept {
  AgtUInt64 oldComp = compare;
  return (compare = _InterlockedCompareExchange(&value, newValue, compare)) == oldComp;
}

AgtUInt8  Impl::atomicIncrement(AgtUInt8& value) noexcept {
  return (AgtUInt8)_InterlockedExchangeAdd8((char*)&value, 1) + 1;
}
AgtUInt16 Impl::atomicIncrement(AgtUInt16& value) noexcept {
  return (AgtUInt16)InterlockedIncrement16(reinterpret_cast<AgtInt16*>(&value));
}
AgtUInt32 Impl::atomicIncrement(AgtUInt32& value) noexcept {
  return InterlockedIncrement(&value);
}
AgtUInt64 Impl::atomicIncrement(AgtUInt64& value) noexcept {
  return InterlockedIncrement(&value);
}

AgtUInt8  Impl::atomicDecrement(AgtUInt8& value) noexcept {
  return (AgtUInt8)_InterlockedExchangeAdd8((char*)&value, (char)-1) - 1;
}
AgtUInt16 Impl::atomicDecrement(AgtUInt16& value) noexcept {
  return (AgtUInt16)_InterlockedDecrement16((AgtInt16*)&value);
}
AgtUInt32 Impl::atomicDecrement(AgtUInt32& value) noexcept {
  return _InterlockedDecrement(&value);
}
AgtUInt64 Impl::atomicDecrement(AgtUInt64& value) noexcept {
  return _InterlockedDecrement(&value);
}


AgtUInt8  Impl::atomicExchangeAdd(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  return (AgtUInt8)_InterlockedExchangeAdd8((char*)&value, (char)newValue);
}
AgtUInt16 Impl::atomicExchangeAdd(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  return (AgtUInt16)_InterlockedExchangeAdd16((AgtInt16*)&value, (AgtInt16)newValue);
}
AgtUInt32 Impl::atomicExchangeAdd(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  return _InterlockedExchangeAdd(&value, newValue);
}
AgtUInt64 Impl::atomicExchangeAdd(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  return _InterlockedExchangeAdd(&value, newValue);
}


AgtUInt8  Impl::atomicExchangeAnd(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  return (AgtUInt8)InterlockedAnd8((char*)&value, (char)newValue);
}
AgtUInt16 Impl::atomicExchangeAnd(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  return (AgtUInt16)InterlockedAnd16((AgtInt16*)&value, (AgtInt16)newValue);
}
AgtUInt32 Impl::atomicExchangeAnd(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  return (AgtUInt32)InterlockedAnd((LONG*)&value, (AgtInt32)newValue);
}
AgtUInt64 Impl::atomicExchangeAnd(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  return (AgtUInt64)InterlockedAnd64((AgtInt64*)&value, (AgtInt64)newValue);
}


AgtUInt8  Impl::atomicExchangeOr(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  return (AgtUInt8)InterlockedOr8((char*)&value, (char)newValue);
}
AgtUInt16 Impl::atomicExchangeOr(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  return (AgtUInt16)InterlockedOr16((AgtInt16*)&value, (AgtInt16)newValue);
}
AgtUInt32 Impl::atomicExchangeOr(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  return (AgtUInt32)InterlockedOr((LONG*)&value, (AgtInt32)newValue);
}
AgtUInt64 Impl::atomicExchangeOr(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  return (AgtUInt64)InterlockedOr64((AgtInt64*)&value, (AgtInt64)newValue);
}


AgtUInt8  Impl::atomicExchangeXor(AgtUInt8& value, AgtUInt8 newValue) noexcept {
  return (AgtUInt8)InterlockedXor8((char*)&value, (char)newValue);
}
AgtUInt16 Impl::atomicExchangeXor(AgtUInt16& value, AgtUInt16 newValue) noexcept {
  return (AgtUInt16)InterlockedXor16((AgtInt16*)&value, (AgtInt16)newValue);
}
AgtUInt32 Impl::atomicExchangeXor(AgtUInt32& value, AgtUInt32 newValue) noexcept {
  return (AgtUInt32)InterlockedXor((LONG*)&value, (AgtInt32)newValue);
}
AgtUInt64 Impl::atomicExchangeXor(AgtUInt64& value, AgtUInt64 newValue) noexcept {
  return (AgtUInt64)InterlockedXor64((AgtInt64*)&value, (AgtInt64)newValue);
}


void Impl::atomicNotifyOne(void* value) noexcept {
  WakeByAddressSingle(value);
}
void Impl::atomicNotifyAll(void* value) noexcept {
  WakeByAddressAll(value);
}


void Impl::atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, AgtSize addressSize) noexcept {
  WaitOnAddress(const_cast<volatile void*>(atomicAddress), compareAddress, addressSize, INFINITE);
}

bool Impl::atomicDeepWaitRaw(const void* atomicAddress, void* compareAddress, AgtSize addressSize, AgtUInt32 timeout) noexcept {
  if (!WaitOnAddress(const_cast<volatile void*>(atomicAddress), compareAddress, addressSize, timeout))
    return GetLastError() != ERROR_TIMEOUT;
  return true;
}




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
