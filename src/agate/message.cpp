//
// Created by maxwe on 2021-06-18.
//

/*#include <agate/message.h>

#include "internal.hpp"


extern "C" {

  JEM_api agt_status_t        JEM_stdcall agt_get_status(agt_message_t message) {
    auto flags = message->flags.fetch();
    if ( flags & agt::message_result_is_ready )
      return message->status;
    if ( flags & agt::message_is_cancelled )
      return AGT_CANCELLED;
    return AGT_NOT_READY;
  }
  JEM_api void                JEM_stdcall agt_set_status(agt_message_t message, agt_status_t status) {
    message->status = status;
  }
  JEM_api agt_status_t        JEM_stdcall agt_notify_sender(agt_message_t message) {
    message->flags.set(agt::message_result_is_ready);
    message->flags.notify_all();
  }
  JEM_api agt_status_t        JEM_stdcall agt_wait(agt_message_t message, jem_u64_t timeout_us) {
    if ( timeout_us == JEM_WAIT ) [[likely]] {
      message->flags.wait_any(agt::message_result_is_ready);
      return message->status;
    }
    if ( message->flags.wait_any_until(agt::message_result_is_ready, deadline_t::from_timeout_us(timeout_us)))
      return message->status;
    return AGT_TIMED_OUT;
  }
  JEM_api void*               JEM_stdcall agt_get_payload(agt_message_t message, jem_size_t* pMessageSize) {
    if ( pMessageSize != nullptr )
      *pMessageSize = message->payloadSize;
    return reinterpret_cast<address_t>(message) + message->payloadOffset;
  }

}*/

#include "common.hpp"


extern "C" {

JEM_api agt_signal_t JEM_stdcall agt_signal_copy(agt_signal_t signal) JEM_noexcept {
  ++signal->openHandles;
  return signal;
}

JEM_api agt_status_t JEM_stdcall agt_signal_wait(agt_signal_t signal, jem_u64_t timeout_us) JEM_noexcept {
  switch ( timeout_us ) {
    case JEM_WAIT:
      signal->flags.wait_any();
    case JEM_DO_NOT_WAIT:
    default:

  }
}

JEM_api void         JEM_stdcall agt_signal_ignore(agt_signal_t signal) JEM_noexcept {
  if ( !--signal->openHandles ) {
    auto message = (agt_message_t)(((std::byte*)signal) - offsetof(agt_message, signal));
    auto mailbox = (signal->messageFlags & agt::message_is_shared) ? (agt_mailbox_t)(((std::byte*)message) + message->mailbox.offset) : message->mailbox.address;
    message->id     = 0;
    message->sender = nullptr;
    agt::vtable_table[mailbox->kind].pfn_return_message(mailbox, message);
  }
}

JEM_api void         JEM_stdcall agt_message_raise_signal(agt_message_t message, agt_status_t status) JEM_noexcept {
  message->signal.status = status;
  message->signal.flags.set(agt::signal_result_is_ready);
  message->signal.flags.notify_all();
}
JEM_api void*        JEM_stdcall agt_message_get_payload(agt_message_t message, jem_size_t* pMessageSize) JEM_noexcept {
  if ( pMessageSize )
    *pMessageSize = message->payloadSize;
  return message->payload;
}



}