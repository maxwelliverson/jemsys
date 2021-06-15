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

  class any_message;



  namespace impl{

    struct message_base{
      jem_size_t       nextSlot;
      std::atomic_flag isReady;
      std::atomic_flag isDiscarded;
    };

    struct mailbox_base{
      // Cacheline 0 - Semaphore for claiming slots [writer]
      JEM_cache_aligned

      std::counting_semaphore<> slotSemaphore;


      // Cacheline 1 - Index of a free slot [writer]
      // JEM_cache_aligned

      std::atomic<jem_size_t> nextFreeSlot;


      // Cacheline 2 - End of the queue [writer]
      // JEM_cache_aligned

      std::atomic<jem_size_t> lastQueuedSlot;


      // Cacheline 3 - Queued message counter [writer,reader]
      // JEM_cache_aligned

      std::atomic<jem_u32_t> queuedSinceLastCheck;


      // Cacheline 4 - Mailbox state [reader]
      // JEM_cache_aligned

      jem_u32_t       minQueuedMessages;

      jem_size_t           messageSlotSize;
      offset_ptr<jem_u8_t> messageSlots;


      JEM_forceinline message_base* get_message(const jem_size_t slot) const noexcept {
        return reinterpret_cast<message_base*>(static_cast<jem_u8_t*>(messageSlots.get()) + (slot * messageSlotSize));
      }
      template <typename Msg>
      JEM_forceinline jem_size_t    get_slot_from_message(const Msg* message) const noexcept {
        return message - reinterpret_cast<const Msg*>(messageSlots.get());
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

        return thisSlot;
      }
      inline jem_size_t acquire_free_message_slot() noexcept {
        slotSemaphore.acquire();
        return get_free_message_slot();
      }
      inline jem_size_t try_acquire_free_message_slot() noexcept {
        if ( !slotSemaphore.try_acquire() )
          return IPC_INVALID_MESSAGE;
        return get_free_message_slot();
      }
      inline jem_size_t try_acquire_free_message_slot_for(duration timeout) noexcept {
        if ( !slotSemaphore.try_acquire_for(timeout) )
          return IPC_INVALID_MESSAGE;
        return get_free_message_slot();
      }
      inline jem_size_t try_acquire_free_message_slot_until(time_point deadline) noexcept {
        if ( !slotSemaphore.try_acquire_until(deadline) )
          return IPC_INVALID_MESSAGE;
        return get_free_message_slot();
      }


      inline jem_size_t wait_on_queued_message(const message_base* prev) noexcept {
        queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
        return prev->nextSlot;
      }
      inline jem_size_t acquire_first_queued_message() noexcept {
        queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
        --minQueuedMessages;
        return 0;
      }
      inline jem_size_t acquire_next_queued_message(const message_base* prev) noexcept {
        jem_size_t messageSlot;
        if ( minQueuedMessages == 0 )
          messageSlot = wait_on_queued_message(prev);
        else
          messageSlot = prev->nextSlot;
        --minQueuedMessages;
        return messageSlot;
      }

      template <typename Msg>
      JEM_forceinline void enqueue_message(Msg* message) noexcept {
        assert(sizeof(Msg) == messageSlotSize);
        enqueue_message(get_slot_from_message(message), message);
      }
      template <typename Msg>
      JEM_forceinline void release_message(Msg* message) noexcept {
        assert(sizeof(Msg) == messageSlotSize);
        release_message(get_slot_from_message(message), message);
      }
      template <typename Msg>
      JEM_forceinline void discard_message(Msg* message) noexcept {
        assert(sizeof(Msg) == messageSlotSize);
        discard_message(get_slot_from_message(message), message);
      }

      inline void       enqueue_message(const jem_size_t slot,     message_base* message) noexcept {
        jem_size_t currentLastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
        do {
          message->nextSlot = currentLastQueuedSlot;
        } while ( !lastQueuedSlot.compare_exchange_weak(currentLastQueuedSlot, slot) );
        queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
        queuedSinceLastCheck.notify_one();
      }
      inline void       release_message(const jem_size_t thisSlot, message_base* message) noexcept {
        jem_size_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
        do {
          message->nextSlot = newNextSlot;
        } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, thisSlot) );
        slotSemaphore.release();
      }
      inline void       discard_message(const jem_size_t msgSlot,  message_base* message) noexcept {
        if ( atomic_flag_test_and_set(&message->isDiscarded) ) {
          release_message(msgSlot, message);
        }
      }
    };
    struct dynamic_mailbox_base{
      jem_size_t              mailboxSize;
      offset_ptr<jem_u8_t>    mailboxData;

      std::atomic<jem_size_t> nextWriteOffset;
      //std::atomic<jem_size_t>
    };

    template <typename Msg>
    class slot : public message_base, public Msg{
    public:
      template <typename ...Args>
      slot(Args&& ...args) noexcept : Msg(std::forward<Args>(args)...){}
    };
  }




  template <typename Msg>
  class mpsc_mailbox{
  public:

    using slot_type = impl::slot<Msg>;

    mpsc_mailbox(jem_size_t slotCount) noexcept
        : mailbox{
            .slotSemaphore        = std::counting_semaphore<>(slotCount),
            .nextFreeSlot         = 0,
            .lastQueuedSlot       = 0,
            .queuedSinceLastCheck = 0,
            .minQueuedMessages    = 0,
            .messageSlotSize      = sizeof(slot_type),
            .messageSlots         = nullptr
          } { }

  private:
    impl::mailbox_base mailbox;
  };

  template <>
  class mpsc_mailbox<any_message>{
  public:

    mpsc_mailbox(jem_size_t size) noexcept
        : mailbox {
            .mailboxSize = size,
            .mailboxData = nullptr,
            .nextWriteOffset = 0
          }{}



  private:
    impl::dynamic_mailbox_base mailbox;
  };
}

#endif //JEMSYS_QUARTZ_IPC_MPSC_MAILBOX_INTERNAL_HPP
