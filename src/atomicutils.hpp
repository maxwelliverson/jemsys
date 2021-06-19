//
// Created by maxwe on 2021-06-18.
//

#ifndef JEMSYS_ATOMICUTILS_INTERNAL_HPP
#define JEMSYS_ATOMICUTILS_INTERNAL_HPP

#include <jemsys.h>

#include <atomic>
#include <semaphore>


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define REPEAT_STMT_x2(stmt) stmt stmt
#define REPEAT_STMT_x4(stmt) REPEAT_STMT_x2(REPEAT_STMT_x2(stmt))
#define REPEAT_STMT_x8(stmt) REPEAT_STMT_x2(REPEAT_STMT_x4(stmt))
#define REPEAT_STMT_x16(stmt) REPEAT_STMT_x4(REPEAT_STMT_x4(stmt))
#define REPEAT_STMT_x32(stmt) REPEAT_STMT_x2(REPEAT_STMT_x16(stmt))

#define PAUSE_x1()  _mm_pause()
#define PAUSE_x2()  do { REPEAT_STMT_x2(PAUSE_x1();) } while(false)
#define PAUSE_x4()  do { REPEAT_STMT_x4(PAUSE_x1();) } while(false)
#define PAUSE_x8()  do { REPEAT_STMT_x8(PAUSE_x1();) } while(false)
#define PAUSE_x16() do { REPEAT_STMT_x16(PAUSE_x1();) } while(false)
#define PAUSE_x32() do { REPEAT_STMT_x32(PAUSE_x1();) } while(false)

namespace {

  void acquire_duffs_spinlock(long& spinlock) {
    jem_u32_t backoff = 0;
    while(_InterlockedExchange(&spinlock, 1) != 0) {
      while (__iso_volatile_load32(&reinterpret_cast<int&>(spinlock)) != 0) {
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
    }
  }
  void release_duffs_spinlock(long& spinlock) {
    _InterlockedExchange(&spinlock, 0);
  }

  struct deadline_t{
    jem_u64_t timestamp;

    inline static deadline_t from_timeout_us(jem_u64_t timeout_us) noexcept {
      jem_u64_t extraTime = timeout_us * 10;
      LARGE_INTEGER lrgInt;
      QueryPerformanceCounter(&lrgInt);
      return deadline_t{
        .timestamp = lrgInt.QuadPart + extraTime
      };
    }

    inline jem_u32_t to_timeout_ms() const noexcept {
      LARGE_INTEGER lrgInt;
      QueryPerformanceCounter(&lrgInt);
      return (timestamp - lrgInt.QuadPart) / 10000;
    }

    inline bool has_passed() const noexcept {
      LARGE_INTEGER lrgInt;
      QueryPerformanceCounter(&lrgInt);
      return lrgInt.QuadPart >= timestamp;
    }
    inline bool has_not_passed() const noexcept {
      LARGE_INTEGER lrgInt;
      QueryPerformanceCounter(&lrgInt);
      return lrgInt.QuadPart < timestamp;
    }
  };

  template <typename T, typename F>
  bool atomic_wait(const std::atomic<T>& target, std::type_identity_t<T> value, deadline_t deadline, F&& func) noexcept {
    T capturedValue = value;
    jem_u32_t remainingTimeout = deadline.to_timeout_ms();
    do {
      WaitOnAddress((volatile void*)std::addressof(target), &capturedValue, sizeof(T), remainingTimeout);
      capturedValue = target.load(std::memory_order_acquire);
      if ( func(capturedValue, value) )
        return true;
      if ( deadline.has_passed() )
        return false;
      remainingTimeout = deadline.to_timeout_ms();
    } while( remainingTimeout > 0 );

    while ( deadline.has_not_passed() ) {
      if ( func(target.load(std::memory_order_acquire), value) )
        return true;
      _mm_pause();
    }
    return false;
  }

  template <typename IntType>
  class atomic_flags{
  public:

    using flag_type = IntType;

    JEM_nodiscard JEM_forceinline bool test(flag_type flags) const noexcept {
      return test_any(flags);
    }

    JEM_nodiscard JEM_forceinline bool test_any(flag_type flags) const noexcept {
      return bits.load() & flags;
    }
    JEM_nodiscard JEM_forceinline bool test_all(flag_type flags) const noexcept {
      return (bits.load() & flags) == flags;
    }
    JEM_nodiscard JEM_forceinline bool test_any() const noexcept {
      return bits.load();
    }

    JEM_nodiscard JEM_forceinline bool test_and_set(flag_type flags) noexcept {
      return test_any_and_set(flags);
    }
    JEM_nodiscard JEM_forceinline bool test_any_and_set(flag_type flags) noexcept {
      return std::atomic_fetch_or(&bits, flags) & flags;
    }
    JEM_nodiscard JEM_forceinline bool test_all_and_set(flag_type flags) noexcept {
      return (std::atomic_fetch_or(&bits, flags) & flags) == flags;
    }

    JEM_nodiscard JEM_forceinline bool test_and_reset(flag_type flags) noexcept {
      return test_any_and_reset(flags);
    }
    JEM_nodiscard JEM_forceinline bool test_any_and_reset(flag_type flags) noexcept {
      return std::atomic_fetch_and(&bits, ~flags) & flags;
    }
    JEM_nodiscard JEM_forceinline bool test_all_and_reset(flag_type flags) noexcept {
      return (std::atomic_fetch_and(&bits, ~flags) & flags) == flags;
    }

    JEM_nodiscard JEM_forceinline bool test_and_flip(flag_type flags) noexcept {
      return test_any_and_flip(flags);
    }
    JEM_nodiscard JEM_forceinline bool test_any_and_flip(flag_type flags) noexcept {
      return std::atomic_fetch_xor(&bits, flags) & flags;
    }
    JEM_nodiscard JEM_forceinline bool test_all_and_flip(flag_type flags) noexcept {
      return (std::atomic_fetch_xor(&bits, flags) & flags) == flags;
    }

    JEM_nodiscard JEM_forceinline flag_type fetch() const noexcept {
      return bits.load();
    }
    JEM_nodiscard JEM_forceinline flag_type fetch(flag_type flags) const noexcept {
      return bits.load() & flags;
    }

    JEM_nodiscard JEM_forceinline flag_type fetch_and_set(flag_type flags) noexcept {
      return std::atomic_fetch_or(&bits, flags);
    }
    JEM_nodiscard JEM_forceinline flag_type fetch_and_reset(flag_type flags) noexcept {
      return std::atomic_fetch_and(&bits, ~flags);
    }
    JEM_nodiscard JEM_forceinline flag_type fetch_and_flip(flag_type flags) noexcept {
      return std::atomic_fetch_xor(&bits, flags);
    }

    JEM_forceinline void set(flag_type flags) noexcept {
      std::atomic_fetch_or(&bits, flags);
    }
    JEM_forceinline void reset(flag_type flags) noexcept {
      std::atomic_fetch_and(&bits, ~flags);
    }
    JEM_forceinline void flip(flag_type flags) noexcept {
      std::atomic_fetch_xor(&bits, flags);
    }
    JEM_forceinline void reset() noexcept {
      bits.store(0);
    }


    JEM_forceinline void wait_exact(flag_type flags) const noexcept {
      flag_type capturedFlags;
      do {
        WaitOnAddress((volatile void*)&bits, &flags, sizeof(flag_type), INFINITE);
        capturedFlags = bits.load(std::memory_order_relaxed);
      } while( capturedFlags != flags );
    }
    JEM_forceinline void wait_any(flag_type flags) const noexcept {
      flag_type capturedFlags;
      do {
        WaitOnAddress((volatile void*)&bits, &flags, sizeof(flag_type), INFINITE);
        capturedFlags = bits.load(std::memory_order_relaxed);
      } while( (capturedFlags & flags) == 0 );
    }
    JEM_forceinline void wait_all(flag_type flags) const noexcept {
      flag_type capturedFlags;
      do {
        WaitOnAddress((volatile void*)&bits, &flags, sizeof(flag_type), INFINITE);
        capturedFlags = bits.load(std::memory_order_relaxed);
      } while( (capturedFlags & flags) != flags );
    }

    JEM_forceinline bool wait_exact_until(flag_type flags, deadline_t deadline) const noexcept {
      return atomic_wait(bits, flags, deadline, [](flag_type a, flag_type b) noexcept { return a == b; });
    }
    JEM_forceinline bool wait_any_until(flag_type flags, deadline_t deadline) const noexcept {
      return atomic_wait(bits, flags, deadline, [](flag_type a, flag_type b) noexcept { return (a & b) != 0; });
    }
    JEM_forceinline bool wait_all_until(flag_type flags, deadline_t deadline) const noexcept {
      return atomic_wait(bits, flags, deadline, [](flag_type a, flag_type b) noexcept { return (a & b) == b; });
    }

    JEM_forceinline void notify_one() noexcept {
      WakeByAddressSingle(&bits);
    }
    JEM_forceinline void notify_all() noexcept {
      WakeByAddressAll(&bits);
    }

  private:
    std::atomic<flag_type> bits = 0;
  };

  using atomic_flags8_t  = atomic_flags<jem_u8_t>;
  using atomic_flags16_t = atomic_flags<jem_u16_t>;
  using atomic_flags32_t = atomic_flags<jem_u32_t>;
  using atomic_flags64_t = atomic_flags<jem_u64_t>;

  using semaphore_t        = std::counting_semaphore<>;
  using binary_semaphore_t = std::binary_semaphore;

  using atomic_u8_t  = std::atomic_uint8_t;
  using atomic_u16_t = std::atomic_uint16_t;
  using atomic_u32_t = std::atomic_uint32_t;
  using atomic_u64_t = std::atomic_uint64_t;

  using atomic_size_t    = std::atomic_size_t;
  using atomic_ptrdiff_t = std::atomic_ptrdiff_t;
}

#endif//JEMSYS_ATOMICUTILS_INTERNAL_HPP
