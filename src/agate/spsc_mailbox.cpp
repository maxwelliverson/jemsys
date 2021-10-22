//
// Created by Maxwell on 2021-06-17.
//


#include "common.hpp"

using namespace agt;

namespace {
  inline agt_message_t get_free_slot(local_spsc_mailbox* mailbox) noexcept {

    agt_message_t msg = mailbox->producerNextFreeSlot;
    mailbox->producerNextFreeSlot = msg->next.address;
    return msg;
  }
  inline agt_message_t get_queued_message(local_spsc_mailbox* mailbox) noexcept {
    agt_message_t message = mailbox->consumerPreviousMsg->next.address;
    free_hold(mailbox, mailbox->consumerPreviousMsg);
    mailbox->consumerPreviousMsg = message;
    return message;
  }
}






#define mailbox ((local_spsc_mailbox*)_mailbox)


agt_status_t  JEM_stdcall agt::impl::local_spsc_try_operation(agt_mailbox_t _mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept {
  switch (opKind) {
    case MAILBOX_OP_KIND_ATTACH:
      if ( role == MAILBOX_ROLE_SENDER ) {
        if ( !mailbox->producerSemaphore.try_acquire_for(timeout_us) )
          return AGT_ERROR_TOO_MANY_SENDERS;
      }
      else {
        if ( !mailbox->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_ERROR_TOO_MANY_RECEIVERS;
      }
      return AGT_SUCCESS;
    case MAILBOX_OP_KIND_DETACH:
      if ( role == MAILBOX_ROLE_SENDER )
        mailbox->producerSemaphore.release();
      else
        mailbox->consumerSemaphore.release();
      return AGT_SUCCESS;
    JEM_no_default;
  }
}


agt_mailslot_t    JEM_stdcall agt::impl::local_spsc_acquire_slot(agt_mailbox_t _mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  if ( slot_size > mailbox->slotSize ) [[unlikely]]
    return nullptr;
  switch ( timeout_us ) {
    case JEM_WAIT:
      mailbox->slotSemaphore.acquire();
      break;
    case JEM_DO_NOT_WAIT:
      if ( !mailbox->slotSemaphore.try_acquire() )
        return nullptr;
      break;
    default:
      if ( !mailbox->slotSemaphore.try_acquire_for(timeout_us) )
        return nullptr;
  }
  agt_message_t msg = get_free_slot(mailbox);
  msg->payloadSize = slot_size;
  return (agt_mailslot_t)msg;
}
/*void          JEM_stdcall agt::impl::local_spsc_release_slot(agt_mailbox_t _mailbox, agt_mailslot_t slot) JEM_noexcept {
  *//*agt_message_t message = to_message(slot);
  agt_message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
  do {
    message->next.address = newNextSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, message) );*//*
  auto message = to_message(slot);
  message->next.address = mailbox->producerNextFreeSlot;
  mailbox->producerNextFreeSlot = message;
  mailbox->slotSemaphore.release();
}*/
agt_signal_t  JEM_stdcall agt::impl::local_spsc_send(agt_mailbox_t _mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept {
  auto message = to_message(slot);
  message->signal.flags.set(flags | agt::message_in_use);
  mailbox->producerPreviousQueuedMsg->next.address = message;
  mailbox->producerPreviousQueuedMsg = message;

  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_message_t JEM_stdcall agt::impl::local_spsc_receive(agt_mailbox_t _mailbox, jem_u64_t timeout_us) JEM_noexcept {
  switch ( timeout_us ) {
    case JEM_WAIT:
      mailbox->queuedMessages.acquire();
      break;
    case JEM_DO_NOT_WAIT:
      if ( !mailbox->queuedMessages.try_acquire() )
        return nullptr;
      break;
    default:
      if ( !mailbox->queuedMessages.try_acquire_for(timeout_us) )
        return nullptr;
  }
  return get_queued_message(mailbox);
}

void          JEM_stdcall agt::impl::local_spsc_return_message(agt_mailbox_t _mailbox, agt_message_t message) JEM_noexcept {
  auto lastFreeSlot = mailbox->lastFreeSlot.exchange(message);
  lastFreeSlot->next.address = message;
  mailbox->slotSemaphore.release();
}



agt_status_t  JEM_stdcall agt::impl::shared_spsc_try_operation(agt_mailbox_t _mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept {
  /*switch (opKind) {
    case MAILBOX_OP_KIND_ATTACH:
      if ( role == MAILBOX_ROLE_SENDER ) {
        if ( !mailbox->producerSemaphore.try_acquire_for(timeout_us) )
          return AGT_ERROR_TOO_MANY_SENDERS;
      }
      else {
        if ( !mailbox->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_ERROR_TOO_MANY_RECEIVERS;
      }
      return AGT_SUCCESS;
    case MAILBOX_OP_KIND_DETACH:
      if ( role == MAILBOX_ROLE_SENDER )
        mailbox->producerSemaphore.release();
      else
        mailbox->consumerSemaphore.release();
      return AGT_SUCCESS;
      JEM_no_default;
  }*/
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}

agt_mailslot_t    JEM_stdcall impl::shared_spsc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}
agt_signal_t  JEM_stdcall impl::shared_spsc_send(agt_mailbox_t mailbox_, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_message_t JEM_stdcall impl::shared_spsc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}

void          JEM_stdcall impl::shared_spsc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {}




/*
#include <agate/spsc_mailbox.h>

#include "internal.hpp"



namespace {
  JEM_forceinline void*         message_to_payload(const agt::spsc_mailbox* mailbox, agt_message_t message) noexcept {
    return reinterpret_cast<std::byte*>(message) + mailbox->payloadOffset;
  }
  JEM_forceinline agt_message_t payload_to_message(const agt::spsc_mailbox* mailbox, void* payload) noexcept {
    return reinterpret_cast<agt_message_t>(static_cast<std::byte*>(payload) - mailbox->payloadOffset);
  }

  JEM_forceinline agt_message_t get_free_slot(agt::spsc_mailbox* mailbox) noexcept {
    agt_message_t slot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    mailbox->nextFreeSlot.store(reinterpret_cast<agt_message_t>(mailbox->producerSlotsAddress + slot->nextSlot), std::memory_order_release);
    return slot;
  }

  JEM_forceinline agt_message_t acquire_free_slot(agt::spsc_mailbox* mailbox) noexcept {
    mailbox->availableSlotSema.acquire();
    return get_free_slot(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_free_slot(agt::spsc_mailbox* mailbox) noexcept {
    if ( !mailbox->availableSlotSema.try_acquire() ) [[unlikely]]
      return nullptr;
    return get_free_slot(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_free_slot_for(agt::spsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
    if ( !mailbox->availableSlotSema.try_acquire_for(timeout_us) ) [[unlikely]]
      return nullptr;
    return get_free_slot(mailbox);
  }

  JEM_forceinline agt_message_t get_queued_message(agt::spsc_mailbox* mailbox) noexcept {
    agt_message_t lastMessage = mailbox->consumerPreviousMsg;
    auto          message     = reinterpret_cast<agt_message_t>(mailbox->consumerSlotsAddress + lastMessage->nextSlot);
    agt_discard_message_spsc(mailbox, lastMessage);
    mailbox->consumerPreviousMsg = message;
    return message;
  }

  JEM_forceinline agt_message_t acquire_next_queued_message(agt::spsc_mailbox* mailbox) noexcept {
    mailbox->queuedMessageSema.acquire();
    return get_queued_message(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_next_queued_message(agt::spsc_mailbox* mailbox) noexcept {
    if ( !mailbox->queuedMessageSema.try_acquire() ) [[unlikely]]
      return nullptr;
    return get_queued_message(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_next_queued_message_for(agt::spsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
    if (!mailbox->queuedMessageSema.try_acquire_for(timeout_us) ) [[unlikely]]
      return nullptr;
    return get_queued_message(mailbox);
  }

  JEM_forceinline void enqueue_slot(agt::spsc_mailbox* mailbox, agt_message_t message) noexcept {
    mailbox->producerPreviousMsg->nextSlot = get_slot(mailbox, message);
    mailbox->producerPreviousMsg = message;
    mailbox->queuedMessageSema.release();
  }
  JEM_forceinline void release_slot(agt::spsc_mailbox* mailbox, agt_message_t message) noexcept {
    mailbox->consumerLastFreeSlot->nextSlot = get_slot(mailbox, message);
    mailbox->consumerLastFreeSlot = message;
    mailbox->availableSlotSema.release();
  }
}


extern "C" {

JEM_api agt_status_t JEM_stdcall agt_start_send_message_spsc(agt_mailbox_t mailbox_, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us) {
  auto* mailbox = static_cast<agt::spsc_mailbox*>(mailbox_);
  agt_message_t message;

  if ( messageSize > mailbox->messageSize ) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;

  if ( timeout_us == JEM_WAIT ) [[likely]] {
    message = acquire_free_slot(mailbox);
  }
  else {
    message = timeout_us == JEM_DO_NOT_WAIT ? try_acquire_free_slot(mailbox) : try_acquire_free_slot_for(mailbox, timeout_us);
    if ( !message )
      return AGT_ERROR_MAILBOX_IS_FULL;
  }

  *pMessagePayload = message_to_payload(mailbox, message);
  return AGT_SUCCESS;
}
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_spsc(agt_mailbox_t mailbox_, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags) {
  constexpr static agt_send_message_flags_t InvalidFlagMask   = ~(AGT_SEND_MESSAGE_CANCEL | AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT);
  constexpr static agt_send_message_flags_t InvalidFlagCombo0 = AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT;
  if ( flags & InvalidFlagMask) [[unlikely]]
  return AGT_ERROR_INVALID_FLAGS;
  if ( (flags & InvalidFlagCombo0 ) == InvalidFlagCombo0 )
    return AGT_ERROR_INVALID_FLAGS;

  auto*         mailbox = static_cast<agt::spsc_mailbox*>(mailbox_);
  agt_message_t message = payload_to_message(mailbox, messagePayload);

  if ( flags & AGT_SEND_MESSAGE_CANCEL ) [[unlikely]] {
    release_slot(mailbox, message);
    *pMessage = nullptr;
    return AGT_CANCELLED;
  }

  message->flags.set(flags | agt::signal_in_use);
  enqueue_slot(mailbox, message);

  *pMessage = message;

  return AGT_SUCCESS;
}
JEM_api agt_status_t JEM_stdcall agt_receive_message_spsc(agt_mailbox_t mailbox_, agt_message_t* pMessage, jem_u64_t timeout_us) {
  agt_message_t message;
  auto* mailbox = static_cast<agt::spsc_mailbox*>(mailbox_);
  if ( timeout_us == JEM_WAIT ) [[likely]] {
    message = acquire_next_queued_message(mailbox);
  }
  else {
    if ( timeout_us == JEM_DO_NOT_WAIT ) {
      message = try_acquire_next_queued_message(mailbox);
    }
    else {
      message = try_acquire_next_queued_message_for(mailbox, timeout_us);
    }
    if ( !message )
      return AGT_ERROR_MAILBOX_IS_EMPTY;
  }
  *pMessage = message;
  return AGT_SUCCESS;
}
JEM_api void         JEM_stdcall agt_discard_message_spsc(agt_mailbox_t mailbox, agt_message_t message) {
  if ( message->flags.test_and_set(agt::signal_result_is_discarded) )
    release_slot(static_cast<agt::spsc_mailbox*>(mailbox), message);
}
JEM_api agt_status_t JEM_stdcall agt_cancel_message_spsc(agt_mailbox_t mailbox, agt_message_t message) {
  if ( message->flags.fetch_and_set(agt::message_is_cancelled) & agt::message_has_been_received )
    return AGT_ERROR_ALREADY_RECEIVED;
  return AGT_SUCCESS;
}

JEM_api agt_status_t JEM_stdcall agt_start_send_message_spsc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);

}*/
