//
// Created by maxwe on 2021-06-18.
//


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