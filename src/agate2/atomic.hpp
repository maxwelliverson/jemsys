//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_ATOMIC_HPP
#define JEMSYS_AGATE2_ATOMIC_HPP

#include "support/atomicutils.hpp"

namespace Agt {
  // using AtomicCounter = atomic_u32_t;
  using AtomicFlags32 = atomic_flags32_t;
  using AtomicFlags64 = atomic_flags64_t;

  class ReferenceCount {
  public:

    ReferenceCount() = default;
    explicit ReferenceCount(AgtUInt32 initialCount) noexcept : value_(initialCount) { }



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
      return _InterlockedExchangeAdd(&value_, 1);
    }
    AgtUInt32 operator++(int) noexcept {
      return _InterlockedExchangeAdd(&value_, 1) + 1;
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
        WaitOnAddress(&const_cast<volatile AgtUInt32&>(value_), &capturedValue, sizeof(AgtUInt32), INFINITE);
        capturedValue = getValue();
      }
    }

    JEM_noinline    bool      deepWaitFor(AgtUInt32 expectedValue, AgtTimeout timeout) const noexcept {

      deadline_t deadline = deadline_t::from_timeout_us(timeout - 1);
      AgtUInt32 capturedValue = getValue();

      while (capturedValue < expectedValue) {
        if (!WaitOnAddress(&const_cast<volatile AgtUInt32&>(value_), &capturedValue, sizeof(AgtUInt32), deadline.to_timeout_ms())) {
          if (GetLastError() == ERROR_TIMEOUT)
            return false;
        }
        capturedValue = getValue();
      }

      return true;
    }


    AgtUInt32 value_ = 0;
  };
}

#endif//JEMSYS_AGATE2_ATOMIC_HPP
