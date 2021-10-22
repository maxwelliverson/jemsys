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
  if ( signal == nullptr ) [[unlikely]]
    return nullptr;

  ++signal->openHandles;
  return signal;
}
JEM_api agt_status_t JEM_stdcall agt_signal_wait(agt_signal_t signal, jem_u64_t timeout_us) JEM_noexcept {

  if ( signal == nullptr ) [[unlikely]]
    return AGT_ERROR_INVALID_SIGNAL;

  switch ( timeout_us ) {
    case JEM_WAIT:
      signal->isReady.wait();
      break;
    case JEM_DO_NOT_WAIT:
      if ( !signal->isReady.test() )
        return AGT_NOT_READY;
      break;
    default:
      if ( !signal->isReady.wait_for(timeout_us) )
        return AGT_NOT_READY;
  }

  agt_status_t result = signal->status;
  // TODO: Implement AGT_MUST_CHECK_RESULT
  // signal->flags.set(agt::message_has_been_checked);
  if ( !--signal->openHandles )
    condemn(signal);
  return result;
}
JEM_api void         JEM_stdcall agt_signal_discard(agt_signal_t signal) JEM_noexcept {
  if ( signal != nullptr ) [[likely]] {
    // TODO: Implement AGT_MUST_CHECK_RESULT

    if ( !--signal->openHandles )
      condemn(signal);
  }
}


/*
JEM_api void         JEM_stdcall agt_message_raise_signal(agt_message_t message, agt_status_t status) JEM_noexcept {
  if ( message->signal.flags.test(agt::message_ignore_result) ) {
    condemn(message);
    return;
  }

  message->signal.status = status;
  message->signal.isReady.set();
}
JEM_api void*        JEM_stdcall agt_message_get_payload(agt_message_t message, jem_global_id_t* pMessageId) JEM_noexcept {
  if ( pMessageId )
    *pMessageId = message->payloadSize;
  return message->payload;
}
*/

JEM_api void JEM_stdcall agt_return_message(agt_message_t message, agt_status_t status) JEM_noexcept {

  JEM_assert( message != nullptr );
  JEM_assume( message != nullptr );

  if ( message->signal.flags.test(agt::message_ignore_result) ) {
    condemn(message);
    return;
  }

  message->signal.status = status;
  message->signal.isReady.set();
}
JEM_api void JEM_stdcall agt_read_message(agt_message_t message, agt_message_info_t* messageInfo) JEM_noexcept {
  JEM_assert( message != nullptr );
  JEM_assert( messageInfo != nullptr );
  JEM_assume( message != nullptr );
  JEM_assume( messageInfo != nullptr );

  messageInfo->messageId = message->id;
  messageInfo->size      = message->payloadSize;
  messageInfo->payload   = message->payload;
}




}