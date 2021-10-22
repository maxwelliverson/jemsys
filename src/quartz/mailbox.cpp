//
// Created by maxwe on 2021-08-20.
//



#include "internal.hpp"

#include <agate.h>

#include <iostream>

#define AGT_INVALID_GLOBAL_MESSAGE 0xFFFFFFFFu



inline std::chrono::microseconds us_since(const std::chrono::high_resolution_clock::time_point& start) noexcept {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
}



qtz_message_action_t qtz_mailbox::proc_noop(qtz_request_t request) noexcept {
  return QTZ_MESSAGE_ACTION_DESTROY;
}
qtz_message_action_t qtz_mailbox::proc_kill(qtz_request_t request) noexcept {
  struct kill_request {
    size_t          structLength;
    qtz_exit_code_t exitCode;
  };
  auto payload = request->payload_as<kill_request>();
  this->shouldCloseMailbox.test_and_set();
  this->exitCode = payload->exitCode;
  killRequest = request;
  return QTZ_MESSAGE_ACTION_DO_NOTHING;
}
qtz_message_action_t qtz_mailbox::proc_placeholder2(qtz_request_t request) noexcept {
  /*request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;*/
  // FIXME: remove dummy implementation lmao
  struct payload_t{
    size_t structLength;
    std::chrono::high_resolution_clock::time_point startTime;
  };
  auto payload = request->payload_as<payload_t>();
  this->startTime = payload->startTime;
  return QTZ_MESSAGE_ACTION_DESTROY;
}

qtz_message_action_t qtz_mailbox::proc_alloc_mailbox(qtz_request_t request) noexcept {

  // using dict_iter_t = typename decltype(namedHandles)::iterator;

  auto payload = request->payload_as<qtz::alloc_mailbox_request>();
  void* mailboxAddress;
  void* slotsAddress;
  std::string_view name{ payload->name, std::strlen(payload->name) };
  // dict_iter_t dictEntry;

  if ( payload->isShared ) {
    request->status = QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE;
    goto exit;
  }

  if ( payload->slotSize == 0 ) {
    request->status = QTZ_ERROR_BAD_SIZE;
    goto exit;
  }

  /*if ( !name.empty() ) {
    bool isUniqueName;
    std::tie(dictEntry, isUniqueName) = namedHandles.try_emplace(name);
    if ( !isUniqueName ) {
      request->status = QTZ_ERROR_NAME_ALREADY_IN_USE;
      goto exit;
    }
  }*/

  mailboxAddress = this->mailboxPool.alloc_block();


  if ( !mailboxAddress ) {
    request->status = QTZ_ERROR_BAD_ALLOC;
    /*if ( !name.empty() )
      namedHandles.erase(dictEntry);*/
    goto exit;
  }


  slotsAddress = _aligned_malloc(payload->slotSize * payload->slotCount, JEM_CACHE_LINE);
  std::memset(slotsAddress, 0, payload->slotSize * payload->slotCount);

  if ( !slotsAddress ) {
    this->mailboxPool.free_block(mailboxAddress);
    request->status = QTZ_ERROR_BAD_ALLOC;
    goto exit;
  }

  /*if ( !name.empty() ) {
    auto& desc = dictEntry->get();
    desc.address = mailboxAddress;
    desc.isShared = payload->isShared;
  }*/

  *payload->handle       = mailboxAddress;
  std::memcpy(mailboxAddress, &slotsAddress, sizeof(void*));

  // assert( std::memcmp(mailboxAddress, &slotsAddress, sizeof(void*)) == 0 );
  // *(void**)mailboxAddress = slotsAddress;
  // *payload->slotsAddress = slotsAddress;
  request->status        = QTZ_SUCCESS;


  exit:
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}

qtz_message_action_t qtz_mailbox::proc_free_mailbox(qtz_request_t request) noexcept {
  auto payload = request->payload_as<qtz::free_mailbox_request>();

  // auto handleEntry = this->namedHandles.find(payload->name);

  this->mailboxPool.free_block(payload->handle);
  // Note: this is only valid for local mailboxes. Fix when implementing IPC support
  _aligned_free(payload->slotsAddress);

  return QTZ_MESSAGE_ACTION_DESTROY;
}


qtz_message_action_t qtz_mailbox::proc_placeholder3(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder4(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder5(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder6(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder7(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder8(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder9(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder10(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_placeholder11(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}

qtz_message_action_t qtz_mailbox::proc_register_object(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_unregister_object(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_link_objects(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_unlink_objects(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_open_object_handle(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_destroy_object(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_log_message(qtz_request_t request) noexcept {
  //TODO: Implement a proper logging mechanism
  auto payload = request->payload_as<qtz::log_message_request>();
  log.write("> (@").write(us_since(startTime)).write(") [[");
  log.write_hex((size_t)request->senderObject).write("]]{ \"");
  log.write(std::string_view(payload->message, payload->structLength - offsetof(qtz::log_message_request, message) - 1));
  log.write("\"}").newline();
  /*") " << "[[" << request->senderObject << "]]{ \""<< std::string_view(payload->message, payload->structLength - offsetof(qtz::log_message_request, message) - 1) << "\" }\n";
  std::cout << "> (@" << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime) << ") " << "[[" << request->senderObject << "]]{ \""<< std::string_view(payload->message, payload->structLength - offsetof(qtz::log_message_request, message) - 1) << "\" }\n";
  */
  return QTZ_MESSAGE_ACTION_DESTROY;
}
qtz_message_action_t qtz_mailbox::proc_execute_callback(qtz_request_t request) noexcept {
  auto payload = request->payload_as<qtz::execute_callback_request>();
  assert( payload->structLength == sizeof(qtz::execute_callback_request) );
  payload->callback(payload->userData);
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}
qtz_message_action_t qtz_mailbox::proc_execute_callback_with_buffer(qtz_request_t request) noexcept {
  auto payload = request->payload_as<qtz::execute_callback_with_buffer_request>();
  payload->callback(payload->userData);
  return QTZ_MESSAGE_ACTION_NOTIFY_SENDER;
}






extern "C" {

  qtz_exit_code_t       JEM_stdcall qtz_mailbox_main_thread_proc(void*) {

    /*using PFN_request_proc = qtz_message_action_t(qtz_mailbox::*)(qtz_request_t) noexcept;

  constexpr static PFN_request_proc global_dispatch_table[] = {
    &qtz_mailbox::proc_noop,
    &qtz_mailbox::proc_alloc_pages,
    &qtz_mailbox::proc_free_pages,
    &qtz_mailbox::proc_alloc_mailbox,
    &qtz_mailbox::proc_free_mailbox,
    &qtz_mailbox::proc_open_ipc_link,
    &qtz_mailbox::proc_close_ipc_link,
    &qtz_mailbox::proc_send_ipc_message,
    &qtz_mailbox::proc_open_deputy,
    &qtz_mailbox::proc_close_deputy,
    &qtz_mailbox::proc_attach_thread,
    &qtz_mailbox::proc_detach_thread,
    &qtz_mailbox::proc_register_agent,
    &qtz_mailbox::proc_unregister_agent
  };*/

    /*qtz_request_t previousMsg = nullptr;
  const auto    mailbox     = g_qtzGlobalMailbox;


  qtz_request_t message = mailbox->acquire_first_queued_request();

  for ( ;; ) {
    switch ( (mailbox->*(global_dispatch_table[message->messageKind]))(message) ) {
      case QTZ_ACTION_DISCARD:
        message->discard();
        break;
      case QTZ_ACTION_NOTIFY_LISTENER:
        message->finalize_and_return();
        break;
      case QTZ_ACTION_DEFERRED:
        break;
      JEM_no_default;
    }
    mailbox->discard_request(previousMsg);
    previousMsg = message;
    if ( mailbox->should_close() ) {
      return mailbox->exitCode;
    }
    message = mailbox->acquire_next_queued_request(previousMsg);
  }*/

    const auto    mailbox     = g_qtzGlobalMailbox;

    for (;;) {
      mailbox->process_next_message();
      if ( mailbox->should_close() ) [[unlikely]] {
        auto exitCode    = mailbox->exitCode;
        auto killRequest = mailbox->killRequest;
        mailbox->~qtz_mailbox();
        // TODO: send message to kernel indicating the process has disconnected
        killRequest->flags.set(QTZ_MESSAGE_IS_READY);
        killRequest->flags.notify_all();
        return exitCode;
      }
    }
  }

  JEM_api qtz_status_t  JEM_stdcall qtz_wait(qtz_request_t message, jem_u64_t timeout_us) noexcept {

  using clock = std::chrono::high_resolution_clock;
  using duration_t = clock::duration;
  using time_point_t = clock::time_point;

  switch ( timeout_us ) {
    case JEM_DO_NOT_WAIT:
      if ( !message->flags.test(QTZ_MESSAGE_IS_READY) )
        return QTZ_NOT_READY;
      break;
    case JEM_WAIT:
      message->flags.wait_any(QTZ_MESSAGE_IS_READY);
      break;
    default:
      if ( !message->flags.wait_any_until(QTZ_MESSAGE_IS_READY, deadline_t::from_timeout_us(timeout_us)))
        return QTZ_NOT_READY;
  }

  auto result = message->status;
  message->discard();
  return result;
}

  JEM_api void          JEM_stdcall qtz_discard(qtz_request_t message) noexcept {
  message->discard();
}

  JEM_api qtz_request_t JEM_stdcall qtz_send(qtz_process_t process, jem_u32_t priority, jem_u32_t messageId, const void* messageBuffer, jem_u64_t us_timeout) noexcept {

  qtz_request_t request;
  qtz_mailbox_t mailbox;
  qtz_mailbox_t localMailbox = g_qtzGlobalMailbox;

  if ( !localMailbox ) [[unlikely]] {
    return nullptr;
  }

  mailbox = process ? (qtz_mailbox_t)process : localMailbox;
  request = mailbox->acquire_slot(us_timeout);

  if ( !request )
    return nullptr;

  request->messageKind = static_cast<qtz_message_kind_t>(messageId);
  request->senderObject = agt_self();
  request->queuePriority = priority ? priority : mailbox->defaultPriority;
  request->fromForeignProcess = mailbox != localMailbox;

  if ( messageId != GLOBAL_MESSAGE_KIND_NOOP ) [[likely]] {
    std::memcpy(request->payload, messageBuffer, *static_cast<const size_t*>(messageBuffer));
  }

  mailbox->enqueue_request(request);

  return request;
}

  JEM_api qtz_process_t JEM_stdcall qtz_get_process() JEM_noexcept {
    return (qtz_process_t)g_qtzGlobalMailbox;
  }

}