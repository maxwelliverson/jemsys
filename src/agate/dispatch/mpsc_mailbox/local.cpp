//
// Created by maxwe on 2021-06-25.
//

#include <agate/dispatch/mpsc_mailbox/local.h>

#include "agate/internal.hpp"


namespace impl{
  namespace {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;


    JEM_forceinline void enqueue_slot(mailbox_t mailbox, message_t message) noexcept {


    }
    JEM_forceinline void release_slot(mailbox_t mailbox, message_t message) noexcept {
      message_t  newNextMsg = mailbox->nextFreeSlot.load(std::memory_order_acquire);
      do {
        message->nextSlot = newNextMsg;
      } while ( !mailbox->nextFreeSlot.compare_exchange_weak(newNextMsg, message));
      mailbox->slotSemaphore.release();
    }

    JEM_forceinline void*         message_to_payload(const agt::local_mpsc_mailbox* mailbox, agt_message_t message) noexcept {
      return reinterpret_cast<jem_u8_t*>(message) + mailbox->payloadOffset;
    }
    JEM_forceinline agt_message_t payload_to_message(const agt::local_mpsc_mailbox* mailbox, void* payload) noexcept {
      return reinterpret_cast<agt_message_t>(static_cast<std::byte*>(payload) - mailbox->payloadOffset);
    }


    JEM_forceinline agt_message_t get_free_slot(agt::local_mpsc_mailbox* mailbox) noexcept {
      jem_size_t    thisSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
      jem_size_t    nextSlot;
      agt_message_t message;
      do {
        message = get_message(mailbox, thisSlot);
        nextSlot = message->nextSlot;
      } while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot, std::memory_order_acq_rel) );
      return message;
    }
    JEM_forceinline agt_message_t acquire_free_slot(agt::local_mpsc_mailbox* mailbox) noexcept {
      mailbox->slotSemaphore.acquire();
      return get_free_slot(mailbox);
    }
    JEM_forceinline agt_message_t try_acquire_free_slot(agt::local_mpsc_mailbox* mailbox) noexcept {
      if ( !mailbox->slotSemaphore.try_acquire() )
        return nullptr;
      return get_free_slot(mailbox);
    }
    JEM_forceinline agt_message_t try_acquire_free_slot_for(agt::local_mpsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
      if ( !mailbox->slotSemaphore.try_acquire_for(timeout_us) )
        return nullptr;
      return get_free_slot(mailbox);
    }
    JEM_forceinline agt_message_t try_acquire_free_slot_until(agt::local_mpsc_mailbox* mailbox, deadline_t deadline) noexcept {
      if ( !mailbox->slotSemaphore.try_acquire_until(deadline) )
        return nullptr;
      return get_free_slot(mailbox);
    }

    JEM_forceinline agt_message_t acquire_next_queued_message(agt::local_mpsc_mailbox* mailbox) noexcept {
      agt_message_t message;
      if ( mailbox->minQueuedMessages == 0 ) {
        mailbox->queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        mailbox->minQueuedMessages = mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
      }
      message = get_message(mailbox, mailbox->previousMessage->nextSlot);
      --mailbox->minQueuedMessages;
      agt_discard_message_mpsc(mailbox, mailbox->previousMessage);
      mailbox->previousMessage = message;
      return message;
    }
    JEM_forceinline agt_message_t try_acquire_next_queued_message(agt::local_mpsc_mailbox* mailbox) noexcept {
      agt_message_t message;
      if ( mailbox->minQueuedMessages == 0 ) {
        mailbox->minQueuedMessages = mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
        if ( mailbox->minQueuedMessages == 0 )
          return nullptr;
      }
      message = get_message(mailbox, mailbox->previousMessage->nextSlot);
      --mailbox->minQueuedMessages;
      agt_discard_message_mpsc(mailbox, mailbox->previousMessage);
      mailbox->previousMessage = message;
      return message;
    }
    JEM_forceinline agt_message_t try_acquire_next_queued_message_for(agt::local_mpsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
      agt_message_t message;
      if ( mailbox->minQueuedMessages == 0 ) {
        if ( !atomic_wait(mailbox->queuedSinceLastCheck, deadline_t::from_timeout_us(timeout_us),
                          [](jem_u32_t a) noexcept { return a > 0; }) )
          return nullptr;
        mailbox->minQueuedMessages = mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
      }
      message = get_message(mailbox, mailbox->previousMessage->nextSlot);
      --mailbox->minQueuedMessages;
      agt_discard_message_mpsc(mailbox, mailbox->previousMessage);
      mailbox->previousMessage = message;
      return message;
    }
  }
}

namespace {

}

extern "C" {

  JEM_api void*               JEM_stdcall agt_local_mpsc_mailbox_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;


    const auto mailbox = cast<mailbox_t>(handle);

    mailbox->slotSemaphore.acquire();

    message_t  message = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    message_t  nextMessage;

    do {
      nextMessage = message->nextSlot;
    } while ( !mailbox->nextFreeSlot.compare_exchange_weak(message, nextMessage) );

    return message;
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_release_slot(agt_handle_t handle, void* slot) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto      mailbox = cast<mailbox_t>(handle);
    const message_t message = static_cast<message_t>(impl::payload_to_message(mailbox, slot));
    message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = newNextSlot;
    } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, message) );
    mailbox->slotSemaphore.release();
  }
  JEM_api agt_message_t       JEM_stdcall agt_local_mpsc_mailbox_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;


    const auto mailbox = cast<mailbox_t>(handle);
    const auto message = static_cast<message_t>(impl::payload_to_message(mailbox, messageSlot));

    message->flags.set(agt::message_in_use | flags);

    message_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
    do {
      lastQueuedMessage->nextSlot = message;
    } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );

    mailbox->queuedSinceLastCheck.fetch_add(1, std::memory_order_relaxed);
    mailbox->queuedSinceLastCheck.notify_one();

    return message;
  }
  JEM_api agt_message_t       JEM_stdcall agt_local_mpsc_mailbox_receive(agt_handle_t handle) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);
    message_t message;

    if ( mailbox->minQueuedMessages == 0 ) {
      mailbox->queuedSinceLastCheck.wait(0, std::memory_order_acquire);
      mailbox->minQueuedMessages = mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
    }
    message = static_cast<message_t >(mailbox->previousReceivedMessage)->nextSlot;
    --mailbox->minQueuedMessages;
    agt_local_mpsc_mailbox_discard(mailbox->previousReceivedMessage);
    mailbox->previousReceivedMessage = message;
    return message;
  }

  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto      mailbox = cast<mailbox_t>(handle);

    if ( slotCount > mailbox->slotCount )
      return AGT_ERROR_INSUFFICIENT_SLOTS;

    for ( ; slotCount != 0; --slotCount ) {
      mailbox->slotSemaphore.acquire();

      message_t  message = mailbox->nextFreeSlot.load(std::memory_order_acquire);
      message_t  nextMessage;

      do {
        nextMessage = message->nextSlot;
      } while ( !mailbox->nextFreeSlot.compare_exchange_weak(message, nextMessage) );

    }
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept;
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept;


  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept;

  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept;


  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;


  JEM_api bool                JEM_stdcall agt_local_mpsc_mailbox_discard(agt_message_t message) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_cancel(agt_message_t message) JEM_noexcept;
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) JEM_noexcept;

}
