//
// Created by Maxwell on 2021-06-17.
//






#include "common.hpp"

using namespace agt;

namespace {
  inline agt_message_t get_free_slot(local_spmc_mailbox* mailbox) noexcept {
    auto msg = mailbox->nextFreeSlot;
    mailbox->nextFreeSlot = msg->next.address;
    return msg;
  }
  inline agt_message_t get_queued_message(local_spmc_mailbox* mailbox) noexcept {
    agt_message_t previousReceivedMessage = mailbox->previousReceivedMessage.load();
    agt_message_t message;
    do {
      message = previousReceivedMessage->next.address;
    } while( !mailbox->previousReceivedMessage.compare_exchange_weak(previousReceivedMessage, message));
    free_hold(mailbox, previousReceivedMessage);
    return message;
  }
}






#define mailbox ((local_spmc_mailbox*)_mailbox)


agt_status_t  JEM_stdcall agt::impl::local_spmc_attach(agt_mailbox_t _mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept {
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
void          JEM_stdcall agt::impl::local_spmc_detach(agt_mailbox_t _mailbox, bool isSender) JEM_noexcept {
  if ( isSender)
    mailbox->producerSemaphore.release();
  else
    mailbox->consumerSemaphore.release();
}


agt_slot_t    JEM_stdcall agt::impl::local_spmc_acquire_slot(agt_mailbox_t _mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
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
void          JEM_stdcall agt::impl::local_spmc_release_slot(agt_mailbox_t _mailbox, agt_slot_t slot) JEM_noexcept {
  /*agt_message_t message = to_message(slot);
  agt_message_t newNextSlot = mailbox->nextFreeSlot.load(std::memory_order_acquire);
  do {
    message->next.address = newNextSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(newNextSlot, message) );*/
  auto message = to_message(slot);
  message->next.address = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = message;
  mailbox->slotSemaphore.release();
}
agt_signal_t  JEM_stdcall agt::impl::local_spmc_send(agt_mailbox_t _mailbox, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept {
  auto message = to_message(slot);
  message->signal.flags.set(flags | agt::message_in_use);
  mailbox->lastQueuedSlot->next.address = message;
  mailbox->lastQueuedSlot = message;
  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_message_t JEM_stdcall agt::impl::local_spmc_receive(agt_mailbox_t _mailbox, jem_u64_t timeout_us) JEM_noexcept {
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


void          JEM_stdcall agt::impl::local_spmc_return_message(agt_mailbox_t _mailbox, agt_message_t message) JEM_noexcept {
  auto lastFreeSlot = mailbox->lastFreeSlot.exchange(message);
  lastFreeSlot->next.address = message;
  mailbox->slotSemaphore.release();
}




agt_status_t  JEM_stdcall impl::shared_spmc_attach(agt_mailbox_t mailbox_, bool isSender, jem_u64_t timeout_us) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void          JEM_stdcall impl::shared_spmc_detach(agt_mailbox_t mailbox_, bool isSender) noexcept {}

agt_slot_t    JEM_stdcall impl::shared_spmc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}
void          JEM_stdcall impl::shared_spmc_release_slot(agt_mailbox_t mailbox_, agt_slot_t slot) JEM_noexcept {}
agt_signal_t  JEM_stdcall impl::shared_spmc_send(agt_mailbox_t mailbox_, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_message_t JEM_stdcall impl::shared_spmc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}

void          JEM_stdcall impl::shared_spmc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {}






/*

#include <agate/spmc_mailbox.h>

extern "C" {

JEM_api agt_status_t JEM_stdcall agt_start_send_message_spmc(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_spmc(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_spmc(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_spmc(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_spmc(agt_mailbox_t mailbox, agt_message_t message);

JEM_api agt_status_t JEM_stdcall agt_start_send_message_spmc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_spmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_spmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_spmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_spmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);

}*/
