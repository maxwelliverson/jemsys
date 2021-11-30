//
// Created by Maxwell on 2021-06-17.
//


#include "common.hpp"


using namespace agt;

namespace {
  inline agt_cookie_t get_free_slot(local_spsc_mailbox* mailbox) noexcept {

    agt_cookie_t msg = mailbox->producerNextFreeSlot;
    mailbox->producerNextFreeSlot = msg->next.address;
    return msg;
  }
  inline agt_cookie_t get_queued_message(local_spsc_mailbox* mailbox) noexcept {
    agt_cookie_t message = mailbox->consumerPreviousMsg->next.address;
    free_hold(mailbox, mailbox->consumerPreviousMsg);
    mailbox->consumerPreviousMsg = message;
    return message;
  }
}






#define mailbox ((local_spsc_mailbox*)_mailbox)



agt_status_t JEM_stdcall agt::impl::local_spsc_connect(agt_mailbox_t _mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  AGT_connect(mailbox, action, timeout_us);
  return AGT_SUCCESS;
}
agt_status_t JEM_stdcall agt::impl::local_spsc_acquire_slot(agt_mailbox_t _mailbox, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us) JEM_noexcept {
  if ( slotSize > mailbox->slotSize ) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  AGT_acquire_semaphore(mailbox->slotSemaphore, timeout_us, AGT_ERROR_MAILBOX_IS_FULL);
  AGT_init_slot(slot, get_free_slot(mailbox), slotSize);
  return AGT_SUCCESS;
}
agt_signal_t JEM_stdcall agt::impl::local_spsc_send(agt_mailbox_t _mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  auto message = slot->cookie;
  AGT_prep_slot(slot);
  message->signal.flags.set(flags | agt::message_in_use);
  mailbox->producerPreviousQueuedMsg->next.address = message;
  mailbox->producerPreviousQueuedMsg = message;

  mailbox->queuedMessages.release();

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_status_t JEM_stdcall agt::impl::local_spsc_receive(agt_mailbox_t _mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  AGT_acquire_semaphore(mailbox->queuedMessages, timeout_us, AGT_ERROR_MAILBOX_IS_EMPTY);
  AGT_read_msg(message, get_queued_message(mailbox));
  return AGT_SUCCESS;
}
void         JEM_stdcall agt::impl::local_spsc_return_message(agt_mailbox_t _mailbox, agt_cookie_t message) JEM_noexcept {
  auto lastFreeSlot = mailbox->lastFreeSlot.exchange(message);
  lastFreeSlot->next.address = message;
  mailbox->slotSemaphore.release();
}


agt_status_t JEM_stdcall impl::shared_spsc_connect(agt_mailbox_t _mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t JEM_stdcall impl::shared_spsc_acquire_slot(agt_mailbox_t _mailbox, agt_slot_t* slot, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_signal_t JEM_stdcall impl::shared_spsc_send(agt_mailbox_t _mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_status_t JEM_stdcall impl::shared_spsc_receive(agt_mailbox_t _mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void         JEM_stdcall impl::shared_spsc_return_message(agt_mailbox_t mailbox_, agt_cookie_t message) JEM_noexcept {}

