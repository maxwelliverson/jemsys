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
  bool atomic_wait(const std::atomic<T>& target, deadline_t deadline, F&& func) noexcept {
    T capturedValue = target.load(std::memory_order_acquire);
    if ( func(capturedValue) )
      return true;

    jem_u32_t remainingTimeout = deadline.to_timeout_ms();
    do {
      WaitOnAddress((volatile void*)std::addressof(target), &capturedValue, sizeof(T), remainingTimeout);
      capturedValue = target.load(std::memory_order_acquire);
      if ( func(capturedValue) )
        return true;
      if ( deadline.has_passed() )
        return false;
      remainingTimeout = deadline.to_timeout_ms();
    } while( remainingTimeout > 0 );

    while ( deadline.has_not_passed() ) {
      if ( func(target.load(std::memory_order_acquire)) )
        return true;
      _mm_pause();
      _mm_pause();
    }
    return false;
  }
  template <typename T>
  bool atomic_wait(const std::atomic<T>& target, std::type_identity_t<T> value, deadline_t deadline) noexcept {
    jem_u32_t remainingTimeout = deadline.to_timeout_ms();
    do {
      WaitOnAddress((volatile void*)std::addressof(target), &value, sizeof(T), remainingTimeout);
      if ( target.load(std::memory_order_relaxed) != value )
        return true;
      if ( deadline.has_passed() )
        return false;
      remainingTimeout = deadline.to_timeout_ms();
    } while( remainingTimeout > 0 );

    while ( deadline.has_not_passed() ) {
      if ( target.load(std::memory_order_relaxed) != value )
        return true;
      _mm_pause();
      _mm_pause();
    }
    return false;
  }

  template <typename IntType>
  class atomic_flags{
  public:

    using flag_type = IntType;

    atomic_flags() = default;
    atomic_flags(flag_type flags) noexcept : bits(flags){}




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
      return atomic_wait(bits, deadline, [flags](flag_type a) noexcept { return a == flags; });
    }
    JEM_forceinline bool wait_any_until(flag_type flags, deadline_t deadline) const noexcept {
      return atomic_wait(bits, deadline, [flags](flag_type a) noexcept { return (a & flags) != 0; });
    }
    JEM_forceinline bool wait_all_until(flag_type flags, deadline_t deadline) const noexcept {
      return atomic_wait(bits, deadline, [flags](flag_type a) noexcept { return (a & flags) == flags; });
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
  
  class semaphore_t {
    inline constexpr static jem_ptrdiff_t LeastMaxValue = (std::numeric_limits<jem_ptrdiff_t>::max)();
  public:
    JEM_nodiscard static constexpr jem_ptrdiff_t(max)() noexcept {
      return LeastMaxValue;
    }

    constexpr explicit semaphore_t(const jem_ptrdiff_t desired) noexcept
        : value(desired) { }

    semaphore_t(const semaphore_t&) = delete;
    semaphore_t& operator=(const semaphore_t&) = delete;

    void release(jem_ptrdiff_t update = 1) noexcept {
      if (update == 0) {
        return;
      }
      
      value.fetch_add(update);
      value.notify_one();
    }



    void acquire() noexcept {
      ptrdiff_t current = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          priv_wait();
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);

        if ( value.compare_exchange_weak(current, current - 1) ) 
          return;
      }
    }

    JEM_nodiscard bool try_acquire() noexcept {
      jem_ptrdiff_t current = value.load();
      if (current == 0)
        return false;
      
      assert(current > 0 && current <= LeastMaxValue);
      
      return value.compare_exchange_weak(current, current - 1);
    }

    JEM_nodiscard bool try_acquire_for(jem_u64_t timeout_us) noexcept {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }
    JEM_nodiscard bool try_acquire_until(deadline_t deadline) noexcept {
      jem_ptrdiff_t current  = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);
        if ( value.compare_exchange_weak(current, current - 1) )
          return true;
      }
    }
    
  private:

    bool priv_wait_until(deadline_t deadline) noexcept {
      jem_ptrdiff_t current = value.load();
      if ( current == 0 )
        return atomic_wait(value, 0, deadline);
      return true;
    }
    void priv_wait() noexcept {
      jem_ptrdiff_t current = value.load();
      if ( current == 0 )
        WaitOnAddress(&value, &current, sizeof(current), INFINITE);
    }

    std::atomic<jem_ptrdiff_t> value;
  };
  class shared_semaphore_t {
  
    inline constexpr static jem_ptrdiff_t LeastMaxValue = (std::numeric_limits<jem_ptrdiff_t>::max)();
  public:
    JEM_nodiscard static constexpr jem_ptrdiff_t(max)() noexcept {
      return LeastMaxValue;
    }

    constexpr explicit shared_semaphore_t(const jem_ptrdiff_t desired) noexcept
        : value(desired) { }

    shared_semaphore_t(const shared_semaphore_t&) = delete;
    shared_semaphore_t& operator=(const shared_semaphore_t&) = delete;

    void release(jem_ptrdiff_t update = 1) noexcept {
      if (update == 0)
        return;
      
      value.fetch_add(update);
      
      const jem_ptrdiff_t waitersUpperBound = waiters.load();
      
      if ( waitersUpperBound == 0 ) {
        
      }
      else if ( waitersUpperBound <= update ) {
        value.notify_all();
      }
      else {
        for (; update != 0; --update)
          value.notify_one();
      }
    }
    
    void acquire() noexcept {
      ptrdiff_t current = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          priv_wait();
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);

        if ( value.compare_exchange_weak(current, current - 1) ) 
          return;
      }
    }

    JEM_nodiscard bool try_acquire() noexcept {
      jem_ptrdiff_t current = value.load();
      if (current == 0)
        return false;
      
      assert(current > 0 && current <= LeastMaxValue);
      
      return value.compare_exchange_weak(current, current - 1);
    }

    JEM_nodiscard bool try_acquire_for(jem_u64_t timeout_us) noexcept {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }
    JEM_nodiscard bool try_acquire_until(deadline_t deadline) noexcept {
      jem_ptrdiff_t current  = value.load(std::memory_order_relaxed);
      for (;;) {
        while ( current == 0 ) {
          if ( !priv_wait_until(deadline) )
            return false;
          current = value.load(std::memory_order_relaxed);
        }
        assert(current > 0 && current <= LeastMaxValue);
        if ( value.compare_exchange_weak(current, current - 1) )
          return true;
      }
    }
    
  private:
    
    bool priv_wait_until(deadline_t deadline) noexcept {
      waiters.fetch_add(1);
      jem_ptrdiff_t current = value.load();
      bool retVal = true;
      if ( current == 0 )
        retVal = atomic_wait(value, 0, deadline);
      waiters.fetch_sub(1, std::memory_order_relaxed);
      return retVal;
    }
    void priv_wait() noexcept {
      waiters.fetch_add(1);
      jem_ptrdiff_t current = value.load();
      if ( current == 0 )
        WaitOnAddress(&value, &current, sizeof(current), INFINITE);
      waiters.fetch_sub(1, std::memory_order_relaxed);
    }
    
    std::atomic<jem_ptrdiff_t> value;
    std::atomic<jem_ptrdiff_t> waiters;
  };
  class binary_semaphore_t{
    inline constexpr static jem_ptrdiff_t LeastMaxValue = 1;
  public:
    JEM_nodiscard static constexpr jem_ptrdiff_t(max)() noexcept {
      return 1;
    }

    constexpr explicit binary_semaphore_t(const jem_ptrdiff_t desired) noexcept
        : value(desired) { }

    binary_semaphore_t(const binary_semaphore_t&) = delete;
    binary_semaphore_t& operator=(const binary_semaphore_t&) = delete;
    
    void release(const jem_ptrdiff_t update = 1) noexcept {
      if (update == 0)
        return;
      assert(update == 1);
      // TRANSITION, GH-1133: should be memory_order_release
      value.store(1, std::memory_order_release);
      value.notify_one();
    }

    void acquire() noexcept {
      for (;;) {
        jem_u8_t previous = value.exchange(0, std::memory_order_acquire);
        if (previous == 1)
          break;
        assert(previous == 0);
        value.wait(0, std::memory_order_relaxed);
      }
    }

    JEM_nodiscard bool try_acquire() noexcept {
      // TRANSITION, GH-1133: should be memory_order_acquire
      jem_u8_t previous = value.exchange(0);
      assert((previous & ~1) == 0);
      return reinterpret_cast<const bool&>(previous);
    }

    JEM_nodiscard bool try_acquire_for(jem_u64_t timeout_us) {
      return try_acquire_until(deadline_t::from_timeout_us(timeout_us));
    }

    JEM_nodiscard bool try_acquire_until(deadline_t deadline) {
      for (;;) {
        jem_u8_t previous = value.exchange(0, std::memory_order_acquire);
        if (previous == 1)
          return true;
        assert(previous == 0);
        if ( !atomic_wait(value, 0, deadline) )
          return false;
      }
    }
    
  private:
    std::atomic<jem_u8_t> value;
  };

  using atomic_flags8_t  = atomic_flags<jem_u8_t>;
  using atomic_flags16_t = atomic_flags<jem_u16_t>;
  using atomic_flags32_t = atomic_flags<jem_u32_t>;
  using atomic_flags64_t = atomic_flags<jem_u64_t>;

  using atomic_u8_t  = std::atomic_uint8_t;
  using atomic_u16_t = std::atomic_uint16_t;
  using atomic_u32_t = std::atomic_uint32_t;
  using atomic_u64_t = std::atomic_uint64_t;

  using atomic_size_t    = std::atomic_size_t;
  using atomic_ptrdiff_t = std::atomic_ptrdiff_t;
}

#endif//JEMSYS_ATOMICUTILS_INTERNAL_HPP
