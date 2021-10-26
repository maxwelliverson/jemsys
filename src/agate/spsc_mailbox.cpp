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

