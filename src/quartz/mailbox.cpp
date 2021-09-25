//
// Created by maxwe on 2021-08-20.
//

#include "internal.hpp"



#define AGT_INVALID_GLOBAL_MESSAGE 0xFFFFFFFFu






qtz_message_action_t qtz_mailbox::proc_noop(qtz_request_t request) noexcept {
  return QTZ_ACTION_DISCARD;
}
qtz_message_action_t qtz_mailbox::proc_alloc_pages(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_free_pages(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}

qtz_message_action_t qtz_mailbox::proc_alloc_mailbox(qtz_request_t request) noexcept {

  using dict_iter_t = typename decltype(namedHandles)::iterator;

  auto payload = request->payload_as<qtz::alloc_mailbox_request>();
  void* mailboxAddress;
  std::string_view name{ payload->name, std::strlen(payload->name) };
  dict_iter_t dictEntry;

  if ( payload->isShared ) {
    request->status = QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE;
    goto exit;
  }

  if ( !name.empty() ) {
    bool isUniqueName;
    std::tie(dictEntry, isUniqueName) = namedHandles.try_emplace(name);
    if ( !isUniqueName ) {
      request->status = QTZ_ERROR_NAME_ALREADY_IN_USE;
      goto exit;
    }
  }

  mailboxAddress = this->mailboxPool.alloc_block();

  if ( !mailboxAddress ) {
    request->status = QTZ_ERROR_BAD_ALLOC;
    if ( !name.empty() )
      namedHandles.erase(dictEntry);
    goto exit;
  }

  if ( !name.empty() ) {
    auto& desc = dictEntry->get();
    desc.address = mailboxAddress;
    desc.isShared = payload->isShared;
  }

  *payload->handle = mailboxAddress;
  request->status = QTZ_SUCCESS;


  exit:
  return QTZ_ACTION_NOTIFY_LISTENER;
}

qtz_message_action_t qtz_mailbox::proc_free_mailbox(qtz_request_t request) noexcept {
  auto payload = request->payload_as<qtz::free_mailbox_request>();

  auto handleEntry = this->namedHandles.find(payload->name);



  return QTZ_ACTION_NOTIFY_LISTENER;
}


qtz_message_action_t qtz_mailbox::proc_open_ipc_link(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_close_ipc_link(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_send_ipc_message(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_open_deputy(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_close_deputy(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_attach_thread(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_detach_thread(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_register_agent(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}
qtz_message_action_t qtz_mailbox::proc_unregister_agent(qtz_request_t request) noexcept {
  request->status = QTZ_ERROR_NOT_IMPLEMENTED;
  return QTZ_ACTION_NOTIFY_LISTENER;
}


namespace {

  inline jem_size_t buffer_length(qtz_local_id_t msgId, const void* buffer = nullptr) noexcept {
    /*if ( msgId & GLOBAL_MESSAGE_KIND_DYNAMIC_BIT )
      return *static_cast<const size_t*>(buffer);

    static constexpr jem_size_t lengths[] = {
      0,
      sizeof(qtz::alloc_pages_request),
      sizeof(qtz::free_pages_request),
      sizeof(qtz::alloc_mailbox_request),
      sizeof(qtz::free_mailbox_request)
    };

    return lengths[msgId];*/
    return *static_cast<const size_t*>(buffer);
  }

  inline void fill_request(qtz_request_t request, qtz_local_id_t msgId, const void* buffer) noexcept {
    request->isRealMessage = true;
    request->messageKind = static_cast<qtz_message_kind_t>(msgId);

    if ( msgId != GLOBAL_MESSAGE_KIND_NOOP ) {
      std::memcpy(request->payload, buffer, buffer_length(msgId, buffer));
    }
  }
}

extern "C" qtz_exit_code_t JEM_stdcall qtz_mailbox_main_thread_proc(void*) {

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
    if ( mailbox->should_close() ) {
      return mailbox->exitCode;
    }
  }
}



extern "C" {


JEM_api qtz_status_t  JEM_stdcall qtz_request_status(qtz_request_t message) {
  auto flags = message->flags.fetch();
  if ( flags & QTZ_MESSAGE_IS_READY )
    return message->status;
  if ( flags & QTZ_MESSAGE_IS_DISCARDED )
    return QTZ_DISCARDED;
  return QTZ_NOT_READY;
}
JEM_api qtz_status_t  JEM_stdcall qtz_request_wait(qtz_request_t message, jem_u64_t timeout_us) {

  using clock = std::chrono::high_resolution_clock;
  using duration_t = clock::duration;
  using time_point_t = clock::time_point;

  auto waitFlags = 0;

  auto deadline = deadline_t::from_timeout_us(timeout_us);

  if ( !message->flags.wait_any_until(QTZ_MESSAGE_IS_READY | QTZ_MESSAGE_IS_DISCARDED, deadline))
    return QTZ_ERROR_TIMED_OUT;
  auto result = qtz_request_status(message);
  qtz_request_discard(message);
  return result;
}
JEM_api void          JEM_stdcall qtz_request_discard(qtz_request_t message) {
  if ( message->is_real_message() ) [[likely]] {
    auto mailbox = message->parent_mailbox();
    mailbox->discard_request(message);
  }
}

JEM_api qtz_request_t JEM_stdcall qtz_send(qtz_local_id_t messageId, const void* messageBuffer) JEM_noexcept {

  if ( !g_qtzGlobalMailbox ) [[unlikely]] {
    return nullptr;
  }

  auto mailbox = g_qtzGlobalMailbox;

  auto request = mailbox->get_free_request_slot();

  fill_request(request, messageId, messageBuffer);

  mailbox->enqueue_request(request);

  return request;
}
JEM_api qtz_request_t JEM_stdcall qtz_try_send(qtz_local_id_t messageId, const void* messageBuffer, jem_u64_t us_timeout) JEM_noexcept {

  qtz_request_t request;

  if ( !g_qtzGlobalMailbox ) [[unlikely]] {
    return nullptr;
  }

  auto mailbox = g_qtzGlobalMailbox;

  switch ( us_timeout ) {
    case JEM_DO_NOT_WAIT:
      request = mailbox->try_acquire_free_request_slot();
      break;
    case JEM_WAIT:
      request = mailbox->acquire_free_request_slot();
      break;
    default:
      request = mailbox->try_acquire_free_request_slot_for(us_timeout);
  }

  if ( !request )
    return nullptr;

  fill_request(request, messageId, messageBuffer);

  mailbox->enqueue_request(request);

  return request;
}
JEM_api qtz_request_t JEM_stdcall qtz_send_ex(qtz_process_t process, qtz_global_id_t messageId, const void* messageBuffer, qtz_send_flags_t flags) JEM_noexcept {
  return nullptr;
}


}