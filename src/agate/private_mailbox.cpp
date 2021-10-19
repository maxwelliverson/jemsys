//
// Created by maxwe on 2021-10-18.
//

#include "common.hpp"


using namespace agt;

#define mailbox ((private_mailbox*)_mailbox)


agt_status_t impl::private_attach(agt_mailbox_t _mailbox, bool isSender, jem_u64_t timeout_us) noexcept {
  if ( isSender ) {
    if ( std::exchange(mailbox->isSenderClaimed, true) )
      return AGT_ERROR_TOO_MANY_SENDERS;
  } else {
    if ( std::exchange(mailbox->isReceiverClaimed, true) )
      return AGT_ERROR_TOO_MANY_RECEIVERS;
  }
  return AGT_SUCCESS;
}
void impl::private_detach(agt_mailbox_t _mailbox, bool isSender) noexcept {
  (isSender ? mailbox->isSenderClaimed : mailbox->isReceiverClaimed) = false;
  /*if ( isSender )
    mailbox->isSenderClaimed = false;
  else
    mailbox->isReceiverClaimed = false;*/
}

agt_slot_t impl::private_acquire_slot(agt_mailbox_t _mailbox, jem_size_t slot_size, jem_u64_t timeout_us) noexcept {
  if ( mailbox->availableSlotCount == 0 || slot_size > mailbox->slotSize)
    return nullptr;
  --mailbox->availableSlotCount;
  agt_message_t acquiredMsg = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = acquiredMsg->next.address;
  acquiredMsg->payloadSize = slot_size;
  return (agt_slot_t)acquiredMsg;
}
void impl::private_release_slot(agt_mailbox_t _mailbox, agt_slot_t slot) noexcept {
  auto msg = to_message(slot);
  ++mailbox->availableSlotCount;
  msg->next.address = mailbox->nextFreeSlot;
  mailbox->nextFreeSlot = msg;
}
agt_signal_t impl::private_send(agt_mailbox_t _mailbox, agt_slot_t slot, agt_send_flags_t flags) noexcept {
  auto msg = to_message(slot);
  msg->signal.flags.set(agt::message_in_use | flags);
  mailbox->prevQueuedMessage->next.address = msg;
  mailbox->prevQueuedMessage = msg;
  ++mailbox->queuedMessageCount;
  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &msg->signal;
}
agt_message_t impl::private_receive(agt_mailbox_t _mailbox, jem_u64_t timeout_us) noexcept {
  if ( mailbox->queuedMessageCount == 0 )
    return nullptr;
  agt_message_t message = mailbox->prevReceivedMessage->next.address;
  free_hold(mailbox, mailbox->prevReceivedMessage);
  mailbox->prevReceivedMessage = message;
  return message;
}

void impl::private_return_message(agt_mailbox_t _mailbox, agt_message_t message) noexcept {
  private_release_slot(_mailbox, (agt_slot_t)message);
}