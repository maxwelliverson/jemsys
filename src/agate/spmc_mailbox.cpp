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

agt_status_t   JEM_stdcall agt::impl::local_spmc_try_operation(agt_mailbox_t _mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept {
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
agt_mailslot_t JEM_stdcall agt::impl::local_spmc_acquire_slot(agt_mailbox_t _mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
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
agt_signal_t   JEM_stdcall agt::impl::local_spmc_send(agt_mailbox_t _mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept {
  auto message = to_message(slot);
  message->signal.flags.set(flags | agt::message_in_use);
  mailbox->lastQueuedSlot->next.address = message;
  mailbox->lastQueuedSlot = message;
  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_message_t  JEM_stdcall agt::impl::local_spmc_receive(agt_mailbox_t _mailbox, jem_u64_t timeout_us) JEM_noexcept {
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
void           JEM_stdcall agt::impl::local_spmc_return_message(agt_mailbox_t _mailbox, agt_message_t message) JEM_noexcept {
  auto lastFreeSlot = mailbox->lastFreeSlot.exchange(message);
  lastFreeSlot->next.address = message;
  mailbox->slotSemaphore.release();
}


agt_status_t   JEM_stdcall impl::shared_spmc_try_operation(agt_mailbox_t mailbox_, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_mailslot_t JEM_stdcall impl::shared_spmc_acquire_slot(agt_mailbox_t mailbox_, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}
agt_signal_t   JEM_stdcall impl::shared_spmc_send(agt_mailbox_t mailbox_, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_message_t  JEM_stdcall impl::shared_spmc_receive(agt_mailbox_t mailbox_, jem_u64_t timeout_us) JEM_noexcept {
  return nullptr;
}
void           JEM_stdcall impl::shared_spmc_return_message(agt_mailbox_t mailbox_, agt_message_t message) noexcept {}


/*void          JEM_stdcall agt::impl::local_spmc_release_slot(agt_mailbox_t _mailbox, agt_mailslot_t slot) JEM_noexcept {
  auto message = to_message(slot);
  message->next.address = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = message;
  mailbox->slotSemaphore.release();
}*/