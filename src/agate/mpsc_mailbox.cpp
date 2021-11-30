//
// Created by Maxwell on 2021-06-17.
//


#include "common.hpp"



using namespace agt;


namespace {

  inline agt_cookie_t get_free_slot(local_mpsc_mailbox* mailbox) noexcept {
    agt_cookie_t thisMsg = mailbox->nextFreeSlot.load(std::memory_order_acquire);
    while ( !mailbox->nextFreeSlot.compare_exchange_weak(thisMsg, thisMsg->next.address) );
    return thisMsg;
  }
  inline agt_cookie_t get_queued_message(local_mpsc_mailbox* mailbox) noexcept {
    agt_cookie_t message = mailbox->previousReceivedMessage->next.address;
    free_hold(mailbox, mailbox->previousReceivedMessage);
    mailbox->previousReceivedMessage = message;
    return message;
  }
}



#define mailbox ((local_mpsc_mailbox*)mailbox_)



agt_status_t JEM_stdcall impl::local_mpsc_connect(agt_mailbox_t mailbox_, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  AGT_connect(mailbox, action, timeout_us);
  return AGT_SUCCESS;
}
agt_status_t JEM_stdcall impl::local_mpsc_acquire_slot(agt_mailbox_t mailbox_, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us) JEM_noexcept {
  if ( slotSize > mailbox->slotSize ) [[unlikely]]
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  AGT_acquire_semaphore(mailbox->slotSemaphore, timeout_us, AGT_ERROR_MAILBOX_IS_FULL);
  AGT_init_slot(slot, get_free_slot(mailbox), slotSize);
  return AGT_SUCCESS;
}
agt_signal_t JEM_stdcall impl::local_mpsc_send(agt_mailbox_t mailbox_, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  AGT_prep_slot(slot);
  agt_cookie_t lastQueuedMessage = mailbox->lastQueuedSlot.load(std::memory_order_acquire);
  agt_cookie_t message = slot->cookie;
  message->signal.flags.set(flags | agt::message_in_use);
  do {
    lastQueuedMessage->next.address = message;
  } while ( !mailbox->lastQueuedSlot.compare_exchange_weak(lastQueuedMessage, message) );
  mailbox->queuedMessageCount.increase(1);

  if ( flags & AGT_IGNORE_RESULT )
    return nullptr;
  return &message->signal;
}
agt_status_t JEM_stdcall impl::local_mpsc_receive(agt_mailbox_t mailbox_, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  switch ( timeout_us ) {
    case JEM_WAIT:
      mailbox->queuedMessageCount.decrease(1);
      break;
    case JEM_DO_NOT_WAIT:
      if ( !mailbox->queuedMessageCount.try_decrease(1) )
        return AGT_ERROR_MAILBOX_IS_EMPTY;
      break;
    default:
      if ( !mailbox->queuedMessageCount.try_decrease_for(1, timeout_us) )
        return AGT_ERROR_MAILBOX_IS_EMPTY;
  }
  AGT_read_msg(message, get_queued_message(mailbox));
  return AGT_SUCCESS;
  // return get_queued_message(mailbox);
}
void         JEM_stdcall impl::local_mpsc_return_message(agt_mailbox_t mailbox_, agt_cookie_t message) JEM_noexcept {
  auto lastFreeSlot = mailbox->lastFreeSlot.load();
  do {
    lastFreeSlot->next.address = message;
  } while( !mailbox->lastFreeSlot.compare_exchange_weak(lastFreeSlot, message) );
  // FIXME: I think this is still a potential bug, though it might be one thats hard to detect....
  lastFreeSlot->next.address = message; // while this might seem redundant, it's needed for this to be totally robust.
  /*
   * Scenario where a message (M3) is lost:
   *
   * T1: lastFreeSlot = M1, message = M2
   * T2: lastFreeSlot = M1, message = M3
   * T2: M1->next = M3
   * T1: M1->next = M2
   * T2: lastFreeSlot = M3
   * T1: loop fail
   * T1: M3->next = M2
   * T1: lastFreeSlot = M2
   * T1: freeSlots += 1
   *
   *
   * */

  mailbox->slotSemaphore.release(1);
}



agt_status_t JEM_stdcall impl::shared_mpsc_connect(agt_mailbox_t _mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_status_t JEM_stdcall impl::shared_mpsc_acquire_slot(agt_mailbox_t _mailbox, agt_slot_t* slot, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
agt_signal_t JEM_stdcall impl::shared_mpsc_send(agt_mailbox_t _mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  return nullptr;
}
agt_status_t JEM_stdcall impl::shared_mpsc_receive(agt_mailbox_t _mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void         JEM_stdcall impl::shared_mpsc_return_message(agt_mailbox_t mailbox_, agt_cookie_t message) JEM_noexcept {}