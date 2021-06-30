//
// Created by maxwe on 2021-06-25.
//

#include <agate/dispatch/mpsc_mailbox/local.h>

#include "agate/internal.hpp"

#include <ranges>

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


    JEM_forceinline void* get_free_slot(agt::local_mpsc_mailbox* mailbox) noexcept {
      message_t  message = mailbox->nextFreeSlot.load(std::memory_order_acquire);
      message_t  nextMessage;

      do {
        nextMessage = message->nextSlot;
      } while ( !mailbox->nextFreeSlot.compare_exchange_weak(message, nextMessage) );

      return message_to_payload(mailbox, message);
    }

    JEM_forceinline void wait_on_queued_messages(agt::local_mpsc_mailbox* mailbox, jem_size_t count) noexcept {
      while ( mailbox->minQueuedMessages < count ) {
        mailbox->queuedSinceLastCheck.wait(0, std::memory_order_acquire);
        mailbox->minQueuedMessages += mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
      }
    }
    JEM_forceinline bool try_wait_on_queued_messages(agt::local_mpsc_mailbox* mailbox, jem_size_t count) noexcept {
      if ( mailbox->minQueuedMessages < count ) {
        mailbox->minQueuedMessages += mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
        if ( mailbox->minQueuedMessages < count )
          return false;
      }
      return true;
    }
    JEM_forceinline bool try_wait_on_queued_messages_for(agt::local_mpsc_mailbox* mailbox, jem_size_t count, jem_u64_t timeout_us) noexcept {
      if ( mailbox->minQueuedMessages < count ) {
        atomic_wait(mailbox->queuedSinceLastCheck, 0, deadline_t::from_timeout_us(timeout_us));
        mailbox->minQueuedMessages += mailbox->queuedSinceLastCheck.exchange(0, std::memory_order_acquire);
        if ( mailbox->minQueuedMessages < count )
          return false;
      }
      return true;
    }


    JEM_forceinline void discard_slot(mailbox_t mailbox, message_t message) noexcept {
      if ( message->flags.test_and_set(agt::message_result_is_discarded) )
        release_slot(mailbox, message);
    }

    JEM_forceinline void get_many_free_slots(mailbox_t mailbox, size_t count, void** slots) noexcept {
      message_t prevMessage = nullptr;

      for (jem_size_t i = 0; i != count; ++i ) {

        message_t  message = mailbox->nextFreeSlot.load(std::memory_order_acquire);
        message_t  nextMessage;

        do {
          nextMessage = message->nextSlot;
        } while ( !mailbox->nextFreeSlot.compare_exchange_weak(message, nextMessage) );

        if ( prevMessage != nullptr ) {
          prevMessage->nextSlot = message;
          prevMessage = message;
        }

        slots[i] = impl::message_to_payload(mailbox, message);
      }

      prevMessage->nextSlot = nullptr;
    }

  }
}


extern "C" {

  JEM_api void*               JEM_stdcall agt_local_mpsc_mailbox_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;


    const auto mailbox = cast<mailbox_t>(handle);

    mailbox->slotSemaphore.acquire();
    return impl::get_free_slot(mailbox);
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_release_slot(agt_handle_t handle, void* slot) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);
    const auto message = static_cast<message_t>(impl::payload_to_message(mailbox, slot));
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


    impl::wait_on_queued_messages(mailbox, 1);

    message = static_cast< message_t >(mailbox->previousReceivedMessage)->nextSlot;
    --mailbox->minQueuedMessages;
    impl::discard_slot(mailbox, mailbox->previousReceivedMessage);
    mailbox->previousReceivedMessage = message;

    return message;
  }

  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    assert( agt_internal_get_object_kind(handle) == agt::object_kind_local_mpsc_mailbox );
    assert( slotCount == 0 || slots != nullptr );

    const auto      mailbox = cast<mailbox_t>(handle);

    if ( !handle_has_permissions(handle, agt::handle_has_send_permission) ) [[unlikely]]
      return AGT_ERROR_INSUFFICIENT_PERMISSIONS;
    if ( slotCount > mailbox->slotCount ) [[unlikely]]
      return AGT_ERROR_INSUFFICIENT_SLOTS;
    if ( slotSize > mailbox->slotSize ) [[unlikely]]
      return AGT_ERROR_MESSAGE_TOO_LARGE;


    if ( slotCount != 0 ) [[likely]] {
      mailbox->slotSemaphore.acquire(static_cast<jem_ptrdiff_t>(slotCount));
      impl::get_many_free_slots(mailbox, slotCount, slots);
    }

    return AGT_SUCCESS;
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept {
    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    assert(slotCount <= (std::numeric_limits<ptrdiff_t>::max)());

    if ( slotCount != 0 ) [[likely]] {
      const auto mailbox = cast<mailbox_t>(handle);
      const auto firstMessage = static_cast<message_t>(impl::payload_to_message(mailbox, slots[0]));
      const auto lastMessage  = static_cast<message_t>(impl::payload_to_message(mailbox, slots[slotCount - 1]));
      message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
      do {
        lastMessage->nextSlot = newNextSlot;
      } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, firstMessage) );
      mailbox->slotSemaphore.release(static_cast<ptrdiff_t>(slotCount));
    }
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_send_many(agt_handle_t handle, jem_size_t slotCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    if ( slotCount != 0 ) [[likely]] {
      const auto mailbox = cast<mailbox_t>(handle);

      std::ranges::for_each(std::span{ messageSlots, slotCount }, [mailbox, flags](void* slot){
        static_cast<message_t>(impl::payload_to_message(mailbox, slot))->flags.set(agt::message_in_use | flags);
      });

      const auto firstMessage = static_cast<message_t>(impl::payload_to_message(mailbox, messageSlots[0]));
      const auto lastMessage  = static_cast<message_t>(impl::payload_to_message(mailbox, messageSlots[slotCount - 1]));
      message_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);

      do {
        lastQueuedMessage->nextSlot = firstMessage;
      } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, lastMessage) );

      mailbox->queuedSinceLastCheck.fetch_add(slotCount, std::memory_order_relaxed);
      mailbox->queuedSinceLastCheck.notify_one();
    }
  }
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept {

    return AGT_ERROR_NOT_YET_IMPLEMENTED;
    /*using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);
    message_t message;

    if ( count > mailbox->slotCount ) [[unlikely]]
      return AGT_ERROR_INSUFFICIENT_SLOTS;


    impl::wait_on_n_queued_messages(mailbox, count);



    message = mailbox->previousReceivedMessage->nextSlot;
    --mailbox->minQueuedMessages;
    agt_local_mpsc_mailbox_discard(mailbox->previousReceivedMessage);
    mailbox->previousReceivedMessage = message;

    return message;*/
  }


  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);

    if ( !handle_has_permissions(handle, agt::handle_has_send_permission) ) [[unlikely]]
      return AGT_ERROR_INSUFFICIENT_PERMISSIONS;
    if ( slotSize > mailbox->slotSize ) [[unlikely]]
      return AGT_ERROR_MESSAGE_TOO_LARGE;

    if ( timeout_us == JEM_WAIT ) [[unlikely]] {
      mailbox->slotSemaphore.acquire();
    }
    else if ( !(timeout_us == JEM_DO_NOT_WAIT ? mailbox->slotSemaphore.try_acquire() : mailbox->slotSemaphore.try_acquire_for(timeout_us)) ) {
      return AGT_ERROR_MAILBOX_IS_FULL;
    }

    *pSlot = impl::get_free_slot(mailbox);
    return AGT_SUCCESS;
  }
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);

    if ( timeout_us == JEM_WAIT ) [[unlikely]] {
      impl::wait_on_queued_messages(mailbox, 1);
    }
    else if ( !(timeout_us == JEM_DO_NOT_WAIT ? impl::try_wait_on_queued_messages(mailbox, 1) : impl::try_wait_on_queued_messages_for(mailbox, 1, timeout_us)) ) {
      return AGT_ERROR_MAILBOX_IS_EMPTY;
    }

    *pMessage = mailbox->previousReceivedMessage->nextSlot;
    --mailbox->minQueuedMessages;
    impl::discard_slot(mailbox, mailbox->previousReceivedMessage);
    mailbox->previousReceivedMessage = static_cast<message_t>(*pMessage);
    return AGT_SUCCESS;
  }

  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept {

    using message_t = agt::local_message*;
    using mailbox_t = agt::local_mpsc_mailbox*;

    const auto mailbox = cast<mailbox_t>(handle);

    if ( timeout_us == JEM_WAIT ) [[unlikely]]
      return agt_local_mpsc_mailbox_acquire_many_slots(handle, slotSize, slotCount, slots);
    if ( slotCount == 0 ) [[unlikely]]
      return AGT_SUCCESS;


    if ( timeout_us == JEM_DO_NOT_WAIT ) {
      if ( !mailbox->slotSemaphore.try_acquire(static_cast<jem_ptrdiff_t>(slotCount)))
        return AGT_ERROR_MAILBOX_IS_FULL;
    }
    else {
      if ( !mailbox->slotSemaphore.try_acquire_for(static_cast<jem_ptrdiff_t>(slotCount), timeout_us) )
        return AGT_ERROR_MAILBOX_IS_FULL;
    }

    impl::get_many_free_slots(mailbox, slotCount, slots);

    return AGT_SUCCESS;
  }
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }


  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;


  JEM_api bool                JEM_stdcall agt_local_mpsc_mailbox_discard(agt_message_t message) JEM_noexcept {
    if ( message->flags.fetch( agt::message_result_cannot_discard | agt::message_result_was_checked ) == agt::message_result_cannot_discard )
      return false;
    impl::discard_slot(cast<impl::mailbox_t>(message->parent), static_cast<impl::message_t>(message));
    return true;
  }
  JEM_api agt_status_t        JEM_stdcall agt_local_mpsc_mailbox_cancel(agt_message_t message) JEM_noexcept {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }
  JEM_api void                JEM_stdcall agt_local_mpsc_mailbox_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) JEM_noexcept {

    const auto mailbox = cast<impl::mailbox_t>(handle);

    for ( jem_size_t i = 0; i < attributeCount; ++i ) {
      switch ( attributeTypes[i] ) {
        case AGT_HANDLE_ATTRIBUTE_TYPE_MAX_MESSAGE_SIZE:
        case AGT_HANDLE_ATTRIBUTE_TYPE_DEFAULT_MESSAGE_SIZE:
          attributes[i].u64 = mailbox->slotSize;
          break;
        case AGT_HANDLE_ATTRIBUTE_TYPE_MAX_CONSUMERS:
          attributes[i].u32 = 1;
          break;
        case AGT_HANDLE_ATTRIBUTE_TYPE_MAX_PRODUCERS:
          attributes[i].u32 = mailbox->maxProducers;
          break;
        case AGT_HANDLE_ATTRIBUTE_TYPE_IS_INTERPROCESS:
          attributes[i].boolean = false;
          break;
      }
    }
  }

}
