//
// Created by Maxwell on 2021-06-17.
//


#include "common.hpp"


using namespace agt;


namespace {

  inline agt_message_t get_free_slot(local_mpsc_mailbox* mailbox) noexcept {
    agt_message_t thisMsg = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisMsg, thisMsg->next.address) );
    return thisMsg;
  }
  inline agt_message_t get_queued_message(local_mpsc_mailbox* mailbox) noexcept {
    agt_message_t message = mailbox->previousReceivedMessage->next.address;
    if ( message->signal.)
    agt_discard_message_mpsc(mailbox, mailbox->previousMessage);
    mailbox->previousMessage = message;
    return message;
  }
}



#define mailbox ((local_mpsc_mailbox*)mailbox_)



agt_status_t  JEM_stdcall impl::local_mpsc_attach(agt_mailbox_t mailbox_, bool isSender, jem_u64_t timeout_us) JEM_noexcept {
  if ( isSender ) {
    if ( !mailbox->producerSemaphore.try_acquire_for(timeout_us) )
      return AGT_ERROR_TOO_MANY_SENDERS;
  }
  else {
    if ( !mailbox->consumerSemaphore.try_acquire_for(timeout_us) )
      return AGT_ERROR_TOO_MANY_RECEIVERS;
  }
  return AGT_SUCCESS;
}
void          JEM_stdcall impl::local_mpsc_detach(agt_mailbox_t mailbox_, bool isSender) JEM_noexcept {
  if ( isSender)
    mailbox->producerSemaphore.release();
  else
    mailbox->consumerSemaphore.release();
}

agt_slot_t    JEM_stdcall impl::local_mpsc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
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
  return (agt_slot_t)msg;
}
void          JEM_stdcall impl::local_mpsc_release_slot(agt_mailbox_t mailbox_, agt_slot_t slot) JEM_noexcept {
  agt_message_t message = (agt_message_t)slot;
  agt_message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
  do {
    message->next.address = newNextSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, message) );
  mailbox->slotSemaphore.release();
}
agt_signal_t  JEM_stdcall impl::local_mpsc_send(agt_mailbox_t mailbox_, agt_slot_t slot) JEM_noexcept {
  agt_message_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
  agt_message_t message = (agt_message_t)slot;
  do {
    lastQueuedMessage->next.address = message;
  } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  mailbox->queuedMessageCount.increase(1);
  return &message->signal;
}
agt_message_t JEM_stdcall impl::local_mpsc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
  switch ( timeout_us ) {
    case JEM_WAIT:
      mailbox->queuedMessageCount.decrease(1);
      break;
    case JEM_DO_NOT_WAIT:
      if ( !mailbox->queuedMessageCount.try_decrease(1) )
        return nullptr;
      break;
    default:
      if ( !mailbox->queuedMessageCount.try_decrease_for(1, timeout_us) )
        return nullptr;
  }
  return get_queued_message(mailbox);
}

void JEM_stdcall impl::local_mpsc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {
  mailbox->lastFreeSlot.
}



/*

namespace {



  JEM_forceinline void enqueue_slot(agt::mpsc_mailbox* mailbox, agt_message_t message) noexcept {
    jem_size_t    currentLastQueuedSlot = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
    agt_message_t lastQueuedMessage;
    jem_size_t    messageSlot = get_slot(mailbox, message);
    do {
      lastQueuedMessage = get_message(mailbox, currentLastQueuedSlot);
      lastQueuedMessage->nextSlot = messageSlot;
    } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(currentLastQueuedSlot, messageSlot) );
    mailbox->queuedSinceLastCheck.fetch_add(1, std::memory_order_relaxed);
    mailbox->queuedSinceLastCheck.notify_one();
  }
  JEM_forceinline void release_slot(agt::mpsc_mailbox* mailbox, agt_message_t message) noexcept {
    jem_size_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    jem_size_t messageSlot = get_slot(mailbox, message);
    do {
      message->nextSlot = newNextSlot;
    } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, messageSlot) );
    mailbox->slotSemaphore.release();
  }

  JEM_forceinline void*         message_to_payload(const agt::mpsc_mailbox* mailbox, agt_message_t message) noexcept {
    return reinterpret_cast<jem_u8_t*>(message) + mailbox->payloadOffset;
  }
  JEM_forceinline agt_message_t payload_to_message(const agt::mpsc_mailbox* mailbox, void* payload) noexcept {
    return reinterpret_cast<agt_message_t>(static_cast<std::byte*>(payload) - mailbox->payloadOffset);
  }


  JEM_forceinline agt_message_t get_free_slot(agt::mpsc_mailbox* mailbox) noexcept {
    jem_size_t    thisSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    jem_size_t    nextSlot;
    agt_message_t message;
    do {
      message = get_message(mailbox, thisSlot);
      nextSlot = message->nextSlot;
    } while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot, std::memory_order_acq_rel) );
    return message;
  }
  JEM_forceinline agt_message_t acquire_free_slot(agt::mpsc_mailbox* mailbox) noexcept {
    mailbox->slotSemaphore.acquire();
    return get_free_slot(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_free_slot(agt::mpsc_mailbox* mailbox) noexcept {
    if ( !mailbox->slotSemaphore.try_acquire() )
      return nullptr;
    return get_free_slot(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_free_slot_for(agt::mpsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
    if ( !mailbox->slotSemaphore.try_acquire_for(timeout_us) )
      return nullptr;
    return get_free_slot(mailbox);
  }
  JEM_forceinline agt_message_t try_acquire_free_slot_until(agt::mpsc_mailbox* mailbox, deadline_t deadline) noexcept {
    if ( !mailbox->slotSemaphore.try_acquire_until(deadline) )
      return nullptr;
    return get_free_slot(mailbox);
  }

  JEM_forceinline agt_message_t acquire_next_queued_message(agt::mpsc_mailbox* mailbox) noexcept {
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
  JEM_forceinline agt_message_t try_acquire_next_queued_message(agt::mpsc_mailbox* mailbox) noexcept {
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
  JEM_forceinline agt_message_t try_acquire_next_queued_message_for(agt::mpsc_mailbox* mailbox, jem_u64_t timeout_us) noexcept {
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


extern "C" {

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpsc(agt_mailbox_t mailbox_, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us) {
  auto* mailbox = static_cast<agt::mpsc_mailbox*>(mailbox_);
  agt_message_t message;

  if ( messageSize > mailbox->msgMemory.size ) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;

  if ( timeout_us == JEM_WAIT ) [[likely]] {
    message = acquire_free_slot(mailbox);
  }
  else {
    message = timeout_us == JEM_DO_NOT_WAIT ? try_acquire_free_slot(mailbox) : try_acquire_free_slot_for(mailbox, timeout_us);
    if ( !message )
      return AGT_ERROR_MAILBOX_IS_FULL;
  }
  __assume( message != nullptr );

  *pMessagePayload = message_to_payload(mailbox, message);
  return AGT_SUCCESS;
}
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpsc(agt_mailbox_t mailbox_, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags) {

  constexpr static agt_send_message_flags_t InvalidFlagMask   = ~(AGT_SEND_MESSAGE_CANCEL | AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT);
  constexpr static agt_send_message_flags_t InvalidFlagCombo0 = AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT;
  if ( flags & InvalidFlagMask) [[unlikely]]
    return AGT_ERROR_INVALID_FLAGS;
  if ( (flags & InvalidFlagCombo0 ) == InvalidFlagCombo0 )
    return AGT_ERROR_INVALID_FLAGS;

  auto*         mailbox = static_cast<agt::mpsc_mailbox*>(mailbox_);
  agt_message_t message = payload_to_message(mailbox, messagePayload);

  if ( flags & AGT_SEND_MESSAGE_CANCEL ) [[unlikely]] {
    release_slot(mailbox, message);
    *pMessage = nullptr;
    return AGT_CANCELLED;
  }

  message->flags.set(flags | agt::message_in_use);
  enqueue_slot(mailbox, message);

  *pMessage = message;

  return AGT_SUCCESS;
}
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpsc(agt_mailbox_t mailbox_, agt_message_t* pMessage, jem_u64_t timeout_us) {
  agt_message_t message;
  auto* mailbox = static_cast<agt::mpsc_mailbox*>(mailbox_);
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
JEM_api void         JEM_stdcall agt_discard_message_mpsc(agt_mailbox_t mailbox, agt_message_t message) {
  if ( message->flags.test_and_set(agt::message_result_is_discarded) )
    release_slot(static_cast<agt::mpsc_mailbox*>(mailbox), message);
}
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpsc(agt_mailbox_t mailbox, agt_message_t message) {}

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpsc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);

}*/
