//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_ATOMIC_HPP
#define JEMSYS_AGATE2_ATOMIC_HPP

#include "support/atomicutils.hpp"

#include <agate2.h>

#define AGT_LONG_TIMEOUT_THRESHOLD 20000

#define DUFFS_MACHINE(backoff) switch (backoff) { \
default:                                          \
  PAUSE_x32();                                    \
  [[fallthrough]];                                \
case 5:                                           \
  PAUSE_x16();                                    \
  [[fallthrough]];                                \
case 4:                                           \
  PAUSE_x8();                                     \
  [[fallthrough]];                                \
case 3:                                           \
  PAUSE_x4();                                     \
  [[fallthrough]];                                \
case 2:                                           \
  PAUSE_x2();                                     \
  [[fallthrough]];                                \
case 1:                                           \
  PAUSE_x1();                                     \
  [[fallthrough]];                                \
case 0:                                           \
  PAUSE_x1();                                     \
  }                                               \
  ++backoff

namespace Agt {

  class Deadline {


    inline constexpr static AgtUInt64 Nanoseconds  = 1;
    inline constexpr static AgtUInt64 NativePeriod = 100;
    inline constexpr static AgtUInt64 Microseconds = 1'000;
    inline constexpr static AgtUInt64 Milliseconds = 1'000'000;



    Deadline(AgtUInt64 timestamp) noexcept : timestamp(timestamp) { }

  public:



    JEM_forceinline static Deadline fromTimeout(AgtTimeout timeoutUs) noexcept {
      return { getCurrentTimestamp() + (timeoutUs * (Microseconds / NativePeriod)) };
    }

    JEM_forceinline AgtUInt32 toTimeoutMs() const noexcept {
      return getTimeoutNative() / (Milliseconds / NativePeriod);
    }

    JEM_forceinline AgtTimeout toTimeoutUs() const noexcept {
      return getTimeoutNative() / (Microseconds / NativePeriod);
    }

    JEM_forceinline bool hasPassed() const noexcept {
      return getCurrentTimestamp() >= timestamp;
    }
    JEM_forceinline bool hasNotPassed() const noexcept {
      return getCurrentTimestamp() < timestamp;
    }

    JEM_forceinline bool isLong() const noexcept {
      return getTimeoutNative() >= (AGT_LONG_TIMEOUT_THRESHOLD * (Microseconds / NativePeriod));
    }

  private:

    JEM_forceinline AgtUInt64 getTimeoutNative() const noexcept {
      return timestamp - getCurrentTimestamp();
    }

    static AgtUInt64 getCurrentTimestamp() noexcept;

    AgtUInt64 timestamp;
  };

  class ResponseQuery {
    enum {
      eRequireAny      = 0x1,
      eRequireMinCount = 0x2,
      eAllowDropped    = 0x4
    };

    ResponseQuery(AgtUInt32 minResponseCount, AgtUInt32 flags) noexcept
        : minDesiredResponseCount(minResponseCount), flags(flags){}

  public:


    // ResponseQuery() noexcept : minDesiredResponseCount(), flags(){}



    static ResponseQuery requireAny(bool countDropped = false) noexcept {
      return { 0, eRequireAny | (countDropped ? eAllowDropped : 0u) };
    }
    static ResponseQuery requireAll(bool countDropped = false) noexcept {
      return { 0, countDropped ? eAllowDropped : 0u };
    }
    static ResponseQuery requireAtLeast(AgtUInt32 minCount, bool countDropped = false) noexcept {
      return { minCount, eRequireMinCount | (countDropped ? eAllowDropped : 0u) };
    }

  private:

    friend class ResponseCount;

    AgtUInt32 minDesiredResponseCount;
    AgtUInt32 flags;
  };

  enum class ResponseQueryResult {
    eNotReady,
    eReady,
    eFailed
  };


  // using AtomicCounter = atomic_u32_t;
  using AtomicFlags32 = atomic_flags32_t;
  using AtomicFlags64 = atomic_flags64_t;

  class ReferenceCount {
  public:

    ReferenceCount() = default;
    explicit ReferenceCount(AgtUInt32 initialCount) noexcept : value_(initialCount) { }


    JEM_nodiscard AgtUInt32 get() const noexcept {
      return (AgtUInt32)__iso_volatile_load32(&reinterpret_cast<const int&>(value_));
    }


    AgtUInt32 acquire() noexcept {
      return _InterlockedIncrement(&value_);
      // return _InterlockedExchangeAdd(&value_, 1) + 1;
    }
    AgtUInt32 acquire(AgtUInt32 n) noexcept {
      return _InterlockedExchangeAdd(&value_, n) + n;
    }

    AgtUInt32 release() noexcept {
      return _InterlockedDecrement(&value_);
    }
    AgtUInt32 release(AgtUInt32 n) noexcept {
      return _InterlockedExchangeAdd(&value_, 0 - n) - n;
    }

    AgtUInt32 operator++()    noexcept {
      return _InterlockedExchangeAdd(&value_, 1);
    }
    AgtUInt32 operator++(int) noexcept {
      return acquire();
    }

    AgtUInt32 operator--()    noexcept {
      return _InterlockedExchangeAdd(&value_, 1);
    }
    AgtUInt32 operator--(int) noexcept {
      return release();
    }

  private:

    AgtUInt32 value_ = 0;
  };

  class AtomicMonotonicCounter {
  public:

    AtomicMonotonicCounter() = default;


    AgtUInt32 getValue() const noexcept {
      return __iso_volatile_load32(&reinterpret_cast<const int&>(value_));
    }

    void reset() noexcept {
      // __iso_volatile_store32(&reinterpret_cast<int&>(value_), 0);     // This doesn't always provide proper ordering
      AgtUInt32 capturedValue = 0;
      while ((capturedValue = _InterlockedCompareExchange(&value_, 0, capturedValue)));
    }



    AgtUInt32 operator++()    noexcept {
      AgtUInt32 result = _InterlockedExchangeAdd(&value_, 1);
      notifyWaiters();
      return result;
    }
    AgtUInt32 operator++(int) noexcept {
      AgtUInt32 result = _InterlockedExchangeAdd(&value_, 1) + 1;
      notifyWaiters();
      return result;
    }


    bool      waitFor(AgtUInt32 expectedValue, AgtTimeout timeout) const noexcept {
      switch (timeout) {
        case JEM_WAIT:
          deepWait(expectedValue);
          return true;
        case JEM_DO_NOT_WAIT:
          return tryWaitOnce(expectedValue);
        default:
          if (timeout < TIMEOUT_US_LONG_WAIT_THRESHOLD)
            return shallowWait(expectedValue, timeout);
          else
            return deepWaitFor(expectedValue, timeout);
      }
    }


  private:

    bool doDeepWait(AgtUInt32& capturedValue, AgtUInt32 timeout) const noexcept {
      _InterlockedIncrement(&deepSleepers_);
      BOOL result = WaitOnAddress(&const_cast<volatile AgtUInt32&>(value_), &capturedValue, sizeof(AgtUInt32), timeout);
      _InterlockedDecrement(&deepSleepers_);
      if (result)
        capturedValue = getValue();
      return result;
    }

    void notifyWaiters() noexcept {
      const AgtUInt32 waiters = __iso_volatile_load32(&reinterpret_cast<const int&>(deepSleepers_));

      if ( waiters == 0 ) [[likely]] {

      } else if ( waiters == 1 ) {
        WakeByAddressSingle(&value_);
      } else {
        WakeByAddressAll(&value_);
      }
    }

    JEM_forceinline AgtUInt32 orderedLoad() const noexcept {
      return _InterlockedCompareExchange(&const_cast<volatile AgtUInt32&>(value_), 0, 0);
    }

    JEM_forceinline bool isLessThan(AgtUInt32 value) const noexcept {
      return orderedLoad() < value;
    }
    JEM_forceinline bool isAtLeast(AgtUInt32 value) const noexcept {
      return orderedLoad() >= value;
    }

    JEM_forceinline bool      shallowWait(AgtUInt32 expectedValue, AgtTimeout timeout) const noexcept {
      deadline_t deadline = deadline_t::from_timeout_us(timeout);
      jem_u32_t backoff = 0;
      while (isLessThan(expectedValue)) {
        if (deadline.has_passed())
          return false;
        switch (backoff) {
          default:
            PAUSE_x32();
            [[fallthrough]];
          case 5:
            PAUSE_x16();
            [[fallthrough]];
          case 4:
            PAUSE_x8();
            [[fallthrough]];
          case 3:
            PAUSE_x4();
            [[fallthrough]];
          case 2:
            PAUSE_x2();
            [[fallthrough]];
          case 1:
            PAUSE_x1();
            [[fallthrough]];
          case 0:
            PAUSE_x1();
        }
        ++backoff;
      }
      return true;
    }

    JEM_forceinline bool      tryWaitOnce(AgtUInt32 expectedValue) const noexcept {
      return orderedLoad() >= expectedValue;
    }

    JEM_noinline    void      deepWait(AgtUInt32 expectedValue) const noexcept {
      AgtUInt32 capturedValue = getValue();
      while (capturedValue < expectedValue) {
        doDeepWait(capturedValue, INFINITE);
        /*WaitOnAddress(&const_cast<volatile AgtUInt32&>(value_), &capturedValue, sizeof(AgtUInt32), INFINITE);
        capturedValue = getValue();*/
      }
    }

    JEM_noinline    bool      deepWaitFor(AgtUInt32 expectedValue, AgtTimeout timeout) const noexcept {

      deadline_t deadline = deadline_t::from_timeout_us(timeout - 1);
      AgtUInt32 capturedValue = getValue();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.to_timeout_ms()))
          return false;
      }

      return true;
    }


    AgtUInt32 value_ = 0;
    mutable AgtUInt32 deepSleepers_ = 0;
  };

  class ResponseCount {
  public:

    ResponseCount() = default;


    void arrive() noexcept {
      _InterlockedIncrement(&arrivedCount_);
      notifyWaiters();
    }

    void drop() noexcept {
      _InterlockedIncrement(&droppedCount_);
      notifyWaiters();
    }

    void reset(AgtUInt32 maxExpectedResponses) noexcept {
      AgtUInt32 capturedValue = 0;
      while ((capturedValue = _InterlockedCompareExchange(&arrivedCount_, 0, capturedValue)));

    }

    bool waitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
      switch (timeout) {
        case JEM_WAIT:
          deepWait(expectedValue);
          return true;
        case JEM_DO_NOT_WAIT:
          return tryWaitOnce(expectedValue);
        default:
          if (timeout < TIMEOUT_US_LONG_WAIT_THRESHOLD)
            return shallowWaitFor(expectedValue, timeout);
          else
            return deepWaitFor(expectedValue, timeout);
      }
    }

    bool waitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      if (!deadline.isLong()) [[likely]] {
        return shallowWaitUntil(expectedValue, deadline);
      } else {
        return deepWaitUntil(expectedValue, deadline);
      }
    }



  private:

    bool      doDeepWait(AgtUInt32& capturedValue, AgtUInt32 timeout) const noexcept;
    void      notifyWaiters() noexcept;


    JEM_forceinline AgtUInt32 fastLoad() const noexcept {
      return __iso_volatile_load32(&reinterpret_cast<const int&>(arrivedCount_));
    }

    JEM_forceinline AgtUInt32 orderedLoad() const noexcept {
      return _InterlockedCompareExchange(&const_cast<volatile AgtUInt32&>(arrivedCount_), 0, 0);
    }

    JEM_forceinline bool      isLessThan(AgtUInt32 value) const noexcept {
      return orderedLoad() < value;
    }
    JEM_forceinline bool      isAtLeast(AgtUInt32 value) const noexcept {
      return orderedLoad() >= value;
    }

    JEM_forceinline bool      isLessThan(ResponseQuery query) const noexcept {
      // return orderedLoad() < query.;
    }
    JEM_forceinline bool      isAtLeast(ResponseQuery query) const noexcept {
      return orderedLoad() >= value;
    }

    JEM_forceinline bool      shallowWaitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
      return shallowWaitUntil(query, Deadline::fromTimeout(timeout));
    }

    JEM_forceinline bool      shallowWaitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      AgtUInt32 backoff = 0;
      while (isLessThan(expectedValue)) {
        if (deadline.hasPassed())
          return false;
        DUFFS_MACHINE(backoff);
      }
      return true;
    }

    JEM_forceinline bool      tryWaitOnce(ResponseQuery query) const noexcept {
      return orderedLoad() >= expectedValue;
    }


    JEM_noinline    void      deepWait(ResponseQuery query) const noexcept {
      AgtUInt32 capturedValue = fastLoad();
      while (capturedValue < expectedValue) {
        doDeepWait(capturedValue, 0xFFFF'FFFF);
      }
    }

    JEM_noinline    bool      deepWaitFor(ResponseQuery query, AgtTimeout timeout) const noexcept {
      Deadline deadline = Deadline::fromTimeout(timeout);
      AgtUInt32 capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }

    JEM_noinline    bool      deepWaitUntil(ResponseQuery query, Deadline deadline) const noexcept {
      AgtUInt32 capturedValue = fastLoad();

      while (capturedValue < expectedValue) {
        if (!doDeepWait(capturedValue, deadline.toTimeoutMs()))
          return false;
      }
      return true;
    }



    union {
      struct {
        AgtUInt32     arrivedCount_;
        AgtUInt32     droppedCount_;
      };
      AgtUInt64       totalCount_ = 0;
    };

    AgtUInt32         expectedResponses_ = 0;
    mutable AgtUInt32 deepSleepers_ = 0;
  };
}

#endif//JEMSYS_AGATE2_ATOMIC_HPP
