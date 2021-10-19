//
// Created by Maxwell on 2021-06-17.
//

#include "common.hpp"


using namespace agt;


namespace {

  inline agt_message_t get_free_slot(local_mpmc_mailbox* mailbox) noexcept {
    agt_message_t thisMsg = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisMsg, thisMsg->next.address) );
    return thisMsg;
  }
  inline agt_message_t get_queued_message(local_mpmc_mailbox* mailbox) noexcept {
    agt_message_t previousReceivedMessage = mailbox->previousReceivedMessage.load();
    agt_message_t message;
    do {
      message = previousReceivedMessage->next.address;
    } while( !mailbox->previousReceivedMessage.compare_exchange_weak(previousReceivedMessage, message));
    free_hold(mailbox, previousReceivedMessage);
    return message;
  }
}



#define mailbox ((local_mpmc_mailbox*)mailbox_)



agt_status_t  JEM_stdcall impl::local_mpmc_attach(agt_mailbox_t mailbox_, bool isSender, jem_u64_t timeout_us) JEM_noexcept {
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
void          JEM_stdcall impl::local_mpmc_detach(agt_mailbox_t mailbox_, bool isSender) JEM_noexcept {
  if ( isSender)
    mailbox->producerSemaphore.release();
  else
    mailbox->consumerSemaphore.release();
}

agt_slot_t    JEM_stdcall impl::local_mpmc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
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
void          JEM_stdcall impl::local_mpmc_release_slot(agt_mailbox_t mailbox_, agt_slot_t slot) JEM_noexcept {
  agt_message_t message = to_message(slot);
  agt_message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
  do {
    message->next.address = newNextSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, message) );
  mailbox->slotSemaphore.release();
}
agt_signal_t  JEM_stdcall impl::local_mpmc_send(agt_mailbox_t mailbox_, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept {
  agt_message_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
  agt_message_t message = to_message(slot);
  message->signal.flags.set(flags | agt::message_in_use);
  do {
    lastQueuedMessage->next.address = message;
  } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_message_t JEM_stdcall impl::local_mpmc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
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

void          JEM_stdcall impl::local_mpmc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {
  auto nextFreeSlot = mailbox->nextFreeSlot.load();
  do {
    message->next.address = nextFreeSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(nextFreeSlot, message) );
  mailbox->slotSemaphore.release();
}


agt_status_t JEM_stdcall impl::shared_mpmc_attach(agt_mailbox_t mailbox_, bool isSender, jem_u64_t timeout_us) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void         JEM_stdcall impl::shared_mpmc_detach(agt_mailbox_t mailbox_, bool isSender) noexcept {}

agt_slot_t    JEM_stdcall impl::shared_mpmc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}
void          JEM_stdcall impl::shared_mpmc_release_slot(agt_mailbox_t mailbox_, agt_slot_t slot) JEM_noexcept {}
agt_signal_t  JEM_stdcall impl::shared_mpmc_send(agt_mailbox_t mailbox_, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_message_t JEM_stdcall impl::shared_mpmc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}

void          JEM_stdcall impl::shared_mpmc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {}

/*

#include <agate.h>

extern "C" {

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpmc(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpmc(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpmc(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpmc(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpmc(agt_mailbox_t mailbox, agt_message_t message);

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpmc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);

}*/
