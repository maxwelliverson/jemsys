//
// Created by Maxwell on 2021-06-17.
//

#include "common.hpp"


using namespace agt;


namespace {

  inline agt_cookie_t get_free_slot(local_mpmc_mailbox* mailbox) noexcept {
    agt_cookie_t thisMsg = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisMsg, thisMsg->next.address) );
    return thisMsg;
  }
  inline agt_cookie_t get_queued_message(local_mpmc_mailbox* mailbox) noexcept {
    agt_cookie_t previousReceivedMessage = mailbox->previousReceivedMessage.load();
    agt_cookie_t message;
    do {
      message = previousReceivedMessage->next.address;
    } while( !mailbox->previousReceivedMessage.compare_exchange_weak(previousReceivedMessage, message));
    free_hold(mailbox, previousReceivedMessage);
    return message;
  }
}



#define mailbox ((local_mpmc_mailbox*)mailbox_)

agt_status_t JEM_stdcall impl::local_mpmc_connect(agt_mailbox_t mailbox_, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  AGT_connect(mailbox, action, timeout_us);
  return AGT_SUCCESS;
}
agt_status_t JEM_stdcall impl::local_mpmc_acquire_slot(agt_mailbox_t mailbox_, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us) JEM_noexcept {
  if ( slotSize > mailbox->slotSize ) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  AGT_acquire_semaphore(mailbox->slotSemaphore, timeout_us, AGT_ERROR_MAILBOX_IS_FULL);
  AGT_init_slot(slot, get_free_slot(mailbox), slotSize);
  return AGT_SUCCESS;
}
agt_signal_t JEM_stdcall impl::local_mpmc_send(agt_mailbox_t mailbox_, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  AGT_prep_slot(slot);
  agt_cookie_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
  agt_cookie_t message = slot->cookie;
  message->signal.flags.set(flags | agt::message_in_use);
  do {
    lastQueuedMessage->next.address = message;
  } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_status_t JEM_stdcall impl::local_mpmc_receive(agt_mailbox_t mailbox_, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  AGT_acquire_semaphore(mailbox->queuedMessages, timeout_us, AGT_ERROR_MAILBOX_IS_EMPTY);
  AGT_read_msg(message, get_queued_message(mailbox));
  return AGT_SUCCESS;
}
void         JEM_stdcall impl::local_mpmc_return_message(agt_mailbox_t mailbox_, agt_cookie_t message) noexcept {
  auto nextFreeSlot = mailbox->nextFreeSlot.load();
  do {
    message->next.address = nextFreeSlot;
  } while( !mailbox->nextFreeSlot.compare_exchange_weak(nextFreeSlot, message) );
  mailbox->slotSemaphore.release();
}

agt_status_t JEM_stdcall impl::shared_mpmc_connect(agt_mailbox_t _mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t JEM_stdcall impl::shared_mpmc_acquire_slot(agt_mailbox_t _mailbox, agt_slot_t* slot, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_signal_t JEM_stdcall impl::shared_mpmc_send(agt_mailbox_t _mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_status_t JEM_stdcall impl::shared_mpmc_receive(agt_mailbox_t _mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void         JEM_stdcall impl::shared_mpmc_return_message(agt_mailbox_t mailbox_, agt_cookie_t message) JEM_noexcept {}
