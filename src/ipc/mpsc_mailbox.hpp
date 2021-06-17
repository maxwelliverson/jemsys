//
// Created by Maxwell on 2021-06-14.
//

#ifndef JEMSYS_IPC_MPSC_MAILBOX_INTERNAL_HPP
#define JEMSYS_IPC_MPSC_MAILBOX_INTERNAL_HPP

#include "offset_ptr.hpp"
#include "segment.hpp"

#include <jemsys.h>

#include <semaphore>
#include <atomic>

#define IPC_INVALID_MESSAGE ((jem_size_t)-1)


namespace ipc{

  using time_point = std::chrono::high_resolution_clock::time_point;
  using duration   = std::chrono::high_resolution_clock::duration;


  enum message_flags_t {
    message_in_use              = 0x01,
    message_result_is_ready     = 0x02,
    message_result_is_discarded = 0x04,
    message_is_cancelled        = 0x08,
    message_needs_cleanup       = 0x10
  };

  class atomic_bitflags{
  public:

    using flag_type = jem_u32_t;

    JEM_nodiscard bool test(flag_type flags) const noexcept {
      return test_any(flags);
    }

    JEM_nodiscard bool test_any(flag_type flags) const noexcept {
      return bits.load() & flags;
    }
    JEM_nodiscard bool test_all(flag_type flags) const noexcept {
      return (bits.load() & flags) == flags;
    }
    JEM_nodiscard bool test_any() const noexcept {
      return bits.load();
    }

    JEM_nodiscard bool test_and_set(flag_type flags) noexcept {
      return test_any_and_set(flags);
    }
    JEM_nodiscard bool test_any_and_set(flag_type flags) noexcept {
      return std::atomic_fetch_or(&bits, flags) & flags;
    }
    JEM_nodiscard bool test_all_and_set(flag_type flags) noexcept {
      return (std::atomic_fetch_or(&bits, flags) & flags) == flags;
    }

    JEM_nodiscard bool test_and_reset(flag_type flags) noexcept {
      return test_any_and_reset(flags);
    }
    JEM_nodiscard bool test_any_and_reset(flag_type flags) noexcept {
      return std::atomic_fetch_and(&bits, ~flags) & flags;
    }
    JEM_nodiscard bool test_all_and_reset(flag_type flags) noexcept {
      return (std::atomic_fetch_and(&bits, ~flags) & flags) == flags;
    }

    JEM_nodiscard bool test_and_flip(flag_type flags) noexcept {
      return test_any_and_flip(flags);
    }
    JEM_nodiscard bool test_any_and_flip(flag_type flags) noexcept {
      return std::atomic_fetch_xor(&bits, flags) & flags;
    }
    JEM_nodiscard bool test_all_and_flip(flag_type flags) noexcept {
      return (std::atomic_fetch_xor(&bits, flags) & flags) == flags;
    }


    JEM_nodiscard flag_type fetch_and_set(flag_type flags) noexcept {
      return std::atomic_fetch_or(&bits, flags);
    }
    JEM_nodiscard flag_type fetch_and_reset(flag_type flags) noexcept {
      return std::atomic_fetch_and(&bits, ~flags);
    }
    JEM_nodiscard flag_type fetch_and_flip(flag_type flags) noexcept {
      return std::atomic_fetch_xor(&bits, flags);
    }

    void set(flag_type flags) noexcept {
      std::atomic_fetch_or(&bits, flags);
    }
    void reset(flag_type flags) noexcept {
      std::atomic_fetch_and(&bits, ~flags);
    }
    void flip(flag_type flags) noexcept {
      std::atomic_fetch_xor(&bits, flags);
    }
    void reset() noexcept {
      bits.store(0);
    }

  private:
    std::atomic<flag_type> bits = 0;
  };

  struct memory_desc{
    jem_size_t size;
    jem_size_t alignment;
  };

  template <typename T>
  inline constexpr static memory_desc default_memory_requirements = { sizeof(T), alignof(T) };



  namespace impl{
    struct message_base{
      jem_size_t      nextSlot;
      jem_size_t      thisSlot;
      atomic_bitflags flags;
    };


    struct mailbox_base{
      // Cacheline 0 - Semaphore for claiming slots [writer]
      JEM_cache_aligned

      std::counting_semaphore<> slotSemaphore{};


      // Cacheline 1 - Index of a free slot [writer]
      // JEM_cache_aligned

      std::atomic<jem_size_t> nextFreeSlot = 0;


      // Cacheline 2 - End of the queue [writer]
      // JEM_cache_aligned

      std::atomic<jem_size_t> lastQueuedSlot = 0;


      // Cacheline 3 - Queued message counter [writer,reader]
      // JEM_cache_aligned

      std::atomic<jem_u32_t>  queuedSinceLastCheck = 0;


      // Cacheline 4 - Mailbox state [reader]
      // JEM_cache_aligned

      jem_u32_t            minQueuedMessages = 0;

      memory_desc          messageSlot;
      jem_size_t           totalSlotCount;
      /*offset_ptr<jem_u8_t> messageSlotsOffset;
      jem_u8_t*            messageSlotsAddress;*/



      void init(jem_size_t slotCount, memory_desc memoryDesc) noexcept {
        new (&slotSemaphore) std::counting_semaphore<>{static_cast<jem_ptrdiff_t>(slotCount)};
        minQueuedMessages = 0;
        messageSlot = memoryDesc;
        totalSlotCount = slotCount;
        jem_size_t slotIndex = 0;

        while ( slotIndex != slotCount - 1 ) {
          message_base* slot = get_message(slotIndex);
          slot->thisSlot = slotIndex;
          slot->nextSlot = ++slotIndex;
        }

        message_base* lastSlot = get_message(slotCount - 1);

        lastSlot->thisSlot = slotCount - 1;
        lastSlot->nextSlot = 0;
      }



      JEM_forceinline message_base* get_message_slots() const noexcept {
        return const_cast<message_base*>(reinterpret_cast<const message_base*>(reinterpret_cast<const jem_u8_t*>(this + 1)));
      }
      JEM_forceinline message_base* get_message(const jem_size_t slot) const noexcept {
        assert( slot != IPC_INVALID_MESSAGE );
        return const_cast<message_base*>(reinterpret_cast<const message_base*>(reinterpret_cast<const jem_u8_t*>(this + 1) + (slot * messageSlot.size)));
      }
      template <typename Msg>
      JEM_forceinline jem_size_t    get_slot_from_message(const Msg* message) const noexcept {
        assert( message != nullptr );
        return message - reinterpret_cast<const Msg*>(this + 1);
      }



      inline message_base* get_free_slot() noexcept {
        jem_size_t    thisSlot = nextFreeSlot.load(std::memory_order_acquire);
        jem_size_t    nextSlot;
        message_base* message;
        do {
          message = get_message(thisSlot);
          nextSlot = message->nextSlot;
          // assert(atomic_load(&message->isFree, relaxed));
        } while ( !nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot, std::memory_order_acq_rel) );

        /*[[maybe_unused]] message_flags_t prevFlags = atomic_fetch_or_explicit(&message->flags, message_in_use, std::memory_order_release);
        assert( !(prevFlags & message_in_use) );*/
        return message;
      }
      inline message_base* acquire_free_slot() noexcept {
        slotSemaphore.acquire();
        return get_free_slot();
      }
      inline message_base* try_acquire_free_slot() noexcept {
        if ( !slotSemaphore.try_acquire() )
          return nullptr;
        return get_free_slot();
      }
      inline message_base* try_acquire_free_slot_for(duration timeout) noexcept {
        if ( !slotSemaphore.try_acquire_for(timeout) )
          return nullptr;
        return get_free_slot();
      }
      inline message_base* try_acquire_free_slot_until(time_point deadline) noexcept {
        if ( !slotSemaphore.try_acquire_until(deadline) )
          return nullptr;
        return get_free_slot();
      }



      inline jem_size_t get_free_message_slot() noexcept {
        jem_size_t    thisSlot = nextFreeSlot.load(std::memory_order_acquire);
        jem_size_t    nextSlot;
        message_base* message;
        do {
          message = get_message(thisSlot);
          nextSlot = message->nextSlot;
          // assert(atomic_load(&message->isFree, relaxed));
        } while ( !nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot, std::memory_order_acq_rel) );

        /*[[maybe_unused]] message_flags_t prevFlags = atomic_fetch_or_explicit(&message->flags, message_in_use, std::memory_order_release);
        assert( !(prevFlags & message_in_use) );*/
        return thisSlot;
      }
      inline jem_size_t acquire_free_message_slot() noexcept {
        slotSemaphore.acquire();
        return get_free_message_slot();
      }
      inline bool   try_acquire_free_message_slot(jem_size_t& slot) noexcept {
        if ( !slotSemaphore.try_acquire() )
          return false;
        slot = get_free_message_slot();
        return true;
      }
      inline bool   try_acquire_free_message_slot_for(duration timeout, jem_size_t& slot) noexcept {
        if ( !slotSemaphore.try_acquire_for(timeout) )
          return false;
        slot = get_free_message_slot();
        return true;
      }
      inline bool   try_acquire_free_message_slot_until(time_point deadline, jem_size_t& slot) noexcept {
        if ( !slotSemaphore.try_acquire_until(deadline) )
          return false;
        slot = get_free_message_slot();
        return true;
      }


      inline message_base* wait_on_queued_message(const message_base* prev) noexcept {
        queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
        return get_message(prev->nextSlot);
      }
      inline message_base* acquire_first_queued_message() noexcept {
        queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
        --minQueuedMessages;
        return get_message_slots();
      }
      inline message_base* acquire_next_queued_message(const message_base* prev) noexcept {
        message_base* slot;
        if ( minQueuedMessages == 0 )
          slot = wait_on_queued_message(prev);
        else
          slot = get_message(prev->nextSlot);
        --minQueuedMessages;
        return slot;
      }


      inline void enqueue_slot(message_base* message) noexcept {
        jem_size_t currentLastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
        do {
          message->nextSlot = currentLastQueuedSlot;
        } while ( !lastQueuedSlot.compare_exchange_weak(currentLastQueuedSlot, message->thisSlot) );
        queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
        queuedSinceLastCheck.notify_one();
      }
      inline void discard_slot(message_base* message) noexcept {
        if ( message->flags.test_and_set(message_result_is_discarded) )
          release_slot(message);
      }
      inline void release_slot(message_base* message) noexcept {
        jem_size_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
        do {
          message->nextSlot = newNextSlot;
        } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, message->thisSlot) );
        message->flags.reset();
        slotSemaphore.release();
      }
    };



    /*struct dynamic_mailbox_base{
      jem_size_t              mailboxSize;
      offset_ptr<jem_u8_t>    mailboxDataOffset;
      jem_u8_t*               mailboxDataAddress;

      std::atomic<jem_size_t> nextWriteOffset;
      std::atomic<jem_size_t> lastWriteOffset;
      jem_size_t              nextReadOffset;
    };*/



    /*struct any_slot {
      jem_size_t       nextSlot;
      std::atomic_flag isReady;
      std::atomic_flag isDiscarded;
      std::byte        payload[];
    };

    struct dynamic_slot{
      jem_size_t       messageSize;
      std::atomic_flag isReady;
      std::atomic_flag isDiscarded;
      std::byte        payload[];
    };*/


    template <typename Msg>
    struct slot_offset{
      message_base base;
      Msg          message;
    };
  }



  template <typename Msg>
  class slot : public impl::message_base, public Msg{
  public:
    template <typename ...Args>
    slot(Args&& ...args) noexcept : Msg(std::forward<Args>(args)...){}
  };



  template <typename Msg>
  class mpsc_mailbox{

    inline static void align_size(memory_desc& mem) noexcept {
      mem.size = ((mem.size - 1) | (mem.alignment - 1)) + 1;
    }
    inline static void get_aligned_mailbox_size(memory_desc& mem) noexcept {
      mem.size = sizeof(impl::mailbox_base);
      mem.alignment = std::max(mem.alignment, alignof(impl::mailbox_base));
      align_size(mem);
    }
    inline static void get_aligned_slot_size(memory_desc& msg) noexcept {
      msg.alignment = std::max(alignof(slot_type), msg.alignment);
      msg.size += offsetof(impl::slot_offset<message_type>, message);
      align_size(msg);
    }

  public:

    using slot_type    = slot<Msg>;
    using message_type = Msg;


    static memory_desc get_memory_requirements(jem_size_t slotCount, memory_desc msgDesc = default_memory_requirements<message_type> ) noexcept {
      get_aligned_slot_size(msgDesc);
      memory_desc result{
        .alignment = msgDesc.alignment
      };
      get_aligned_mailbox_size(result);
      result.size += (slotCount * msgDesc.size);
      return result;
    }

    explicit mpsc_mailbox(jem_size_t slotCount, memory_desc msgDesc = default_memory_requirements<message_type>) noexcept{
      get_aligned_slot_size(msgDesc);
      mailbox.init(slotCount, msgDesc);
    }

    JEM_nodiscard JEM_forceinline slot_type*     begin_write() noexcept {
      return static_cast<slot_type*>(mailbox.acquire_free_slot());
    }
    JEM_nodiscard JEM_forceinline slot_type* try_begin_write() noexcept {
      return static_cast<slot_type*>(mailbox.try_acquire_free_slot());
    }
    JEM_nodiscard JEM_forceinline slot_type* try_begin_write_for(duration timeout) noexcept {
      return static_cast<slot_type*>(mailbox.try_acquire_free_slot_for(timeout));
    }
    JEM_nodiscard JEM_forceinline slot_type* try_begin_write_until(time_point deadline) noexcept {
      return static_cast<slot_type*>(mailbox.try_acquire_free_slot_until(deadline));
    }
    JEM_forceinline void                         end_write(slot_type* message) noexcept {
      mailbox.enqueue_slot(message);
    }

    JEM_nodiscard JEM_forceinline slot_type* read_next() noexcept {
      return static_cast<slot_type*>(mailbox.acquire_first_queued_message());
    }
    JEM_nodiscard JEM_forceinline slot_type* read_next(slot_type* previous) noexcept {
      return static_cast<slot_type*>(mailbox.acquire_next_queued_message(previous));
    }



    JEM_forceinline void discard(slot_type* slot) noexcept {
      mailbox.discard_slot(slot);
    }
    JEM_forceinline void release(slot_type* slot) noexcept {
      mailbox.release_slot(slot);
    }

    JEM_nodiscard jem_size_t total_memory_size() const noexcept {
      return get_memory_requirements(mailbox.totalSlotCount, mailbox.messageSlot).size;
    }


  private:

    impl::mailbox_base mailbox;
  };



  /*template <>
  class mpsc_mailbox<any_message_sized>{

  public:

    using slot_type = impl::dynamic_slot;

    explicit mpsc_mailbox(jem_size_t slotCount) noexcept
    : mailbox{
      .slotSemaphore        = std::counting_semaphore<>(static_cast<jem_ptrdiff_t>(slotCount)),
      .nextFreeSlot         = 0,
      .lastQueuedSlot       = 0,
      .queuedSinceLastCheck = 0,
      .minQueuedMessages    = 0,
      .messageSlotSize      = sizeof(slot_type)
    }{ }



  private:
    impl::mailbox_base mailbox;
  };


  template <>
  class mpsc_mailbox<variable_length_messages>{
  public:

    using slot_type = impl::dynamic_slot;

    explicit mpsc_mailbox(jem_size_t size) noexcept
        : mailbox {
            .mailboxSize = size,
            .mailboxDataOffset = nullptr,
            .mailboxDataAddress = nullptr,
            .nextWriteOffset = 0,
            .lastWriteOffset = 0,
            .nextReadOffset  = 0
        }{ }



  private:
    impl::dynamic_mailbox_base mailbox;
  };

  template <>
  class mpsc_mailbox<any_message>{
  public:

    using slot_type = impl::dynamic_slot;

    explicit mpsc_mailbox(jem_size_t size) noexcept
        : mailbox {
            .mailboxSize = size,
            .mailboxDataOffset = nullptr,
            .mailboxDataAddress = nullptr,
            .nextWriteOffset = 0,
            .lastWriteOffset = 0,
            .nextReadOffset  = 0
          }{ }



  private:
    impl::dynamic_mailbox_base mailbox;
  };*/
}

#endif //JEMSYS_QUARTZ_IPC_MPSC_MAILBOX_INTERNAL_HPP
