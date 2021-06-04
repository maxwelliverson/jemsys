//
// Created by maxwe on 2021-06-01.
//

#include "internal.h"

namespace {
  using PFN_request_proc = qtz_message_action_t(qtz_mailbox::*)(qtz_request_t) noexcept;

  inline constexpr PFN_request_proc global_dispatch_table[] = {
      &qtz_mailbox::proc_alloc_pages,
      &qtz_mailbox::proc_free_pages,
      &qtz_mailbox::proc_alloc_static_mailbox,
      &qtz_mailbox::proc_free_static_mailbox,
      &qtz_mailbox::proc_alloc_dynamic_mailbox,
      &qtz_mailbox::proc_free_dynamic_mailbox,
      &qtz_mailbox::proc_alloc_static_mailbox_shared,
      &qtz_mailbox::proc_free_static_mailbox_shared,
      &qtz_mailbox::proc_alloc_dynamic_mailbox_shared,
      &qtz_mailbox::proc_free_dynamic_mailbox_shared,
      &qtz_mailbox::proc_open_ipc_link,
      &qtz_mailbox::proc_close_ipc_link,
      &qtz_mailbox::proc_send_ipc_message,
      &qtz_mailbox::proc_open_deputy,
      &qtz_mailbox::proc_close_deputy,
      &qtz_mailbox::proc_attach_thread,
      &qtz_mailbox::proc_detach_thread,
      &qtz_mailbox::proc_register_agent,
      &qtz_mailbox::proc_unregister_agent
  };
}




#define AGT_INVALID_GLOBAL_MESSAGE 0xFFFFFFFFu







extern "C" JEM_stdcall qtz_exit_code_t qtz_mailbox_main_thread_proc(void*) {

  qtz_request_t previousMsg = nullptr;
  const auto    mailbox = globalMailbox;


  qtz_request_t message = mailbox->acquire_first_queued_request();

  for ( ;; ) {
    switch ( (mailbox->*(global_dispatch_table[message->kind]))(message) ) {
      case QTZ_DISCARD:
        message->discard();
        break;
        case QTZ_NOTIFY_LISTENER:
          message->notify_listener();
          break;
          case QTZ_DEFERRED:
            break;
          JEM_no_default;
    }
    mailbox->discard_request(previousMsg);
    previousMsg = message;
    if ( mailbox->should_close() ) {
      return mailbox->exitCode;
    }
    message = mailbox->acquire_next_queued_request(previousMsg);
  }
}




