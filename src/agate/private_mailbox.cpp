//
// Created by maxwe on 2021-10-18.
//

#include "common.hpp"


using namespace agt;

#define mailbox ((private_mailbox*)_mailbox)


agt_status_t JEM_stdcall impl::private_connect(agt_mailbox_t _mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  switch (action) {
    case AGT_ATTACH_SENDER:
      if ( std::exchange(mailbox->isSenderClaimed, true) )
        return AGT_ERROR_TOO_MANY_SENDERS;
      break;
    case AGT_DETACH_SENDER:
      mailbox->isSenderClaimed = false;
      break;
    case AGT_ATTACH_RECEIVER:
      if ( std::exchange(mailbox->isReceiverClaimed, true) )
        return AGT_ERROR_TOO_MANY_RECEIVERS;
      break;
    case AGT_DETACH_RECEIVER:
      mailbox->isReceiverClaimed = false;
      break;
  }
  return AGT_SUCCESS;
}
agt_status_t JEM_stdcall impl::private_acquire_slot(agt_mailbox_t _mailbox, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us) JEM_noexcept {
  if ( mailbox->availableSlotCount == 0 ) [[unlikely]]
    return AGT_ERROR_MAILBOX_IS_FULL;
  if ( slotSize > mailbox->slotSize)
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  --mailbox->availableSlotCount;
  agt_cookie_t acquiredMsg = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = acquiredMsg->next.address;
  AGT_init_slot(slot, acquiredMsg, slotSize);
  return AGT_SUCCESS;
}
agt_signal_t JEM_stdcall impl::private_send(agt_mailbox_t _mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  auto msg = slot->cookie;
  msg->signal.flags.set(agt::message_in_use | flags);
  mailbox->prevQueuedMessage->next.address = msg;
  mailbox->prevQueuedMessage = msg;
  ++mailbox->queuedMessageCount;
  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &msg->signal;
}
agt_status_t JEM_stdcall impl::private_receive(agt_mailbox_t _mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  if ( mailbox->queuedMessageCount == 0 )
    return AGT_ERROR_MAILBOX_IS_EMPTY;
  agt_cookie_t kookie = mailbox->prevReceivedMessage->next.address;
  free_hold(mailbox, mailbox->prevReceivedMessage);
  mailbox->prevReceivedMessage = kookie;
  AGT_read_msg(message, kookie);
  return AGT_SUCCESS;
}
void         JEM_stdcall impl::private_return_message(agt_mailbox_t _mailbox, agt_cookie_t message) JEM_noexcept {
  ++mailbox->availableSlotCount;
  message->next.address = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = message;
}