//
// Created by maxwe on 2021-10-04.
//

#include "common.hpp"


#include <quartz.h>


namespace {

  struct create_mailbox_params{
    void*               slotsAddress;
    jem_size_t          slotSize;
    jem_size_t          slotCount;
    jem_u32_t           maxSenders;
    jem_u32_t           maxReceivers;
    agt_mailbox_scope_t scope;
  };

  inline void init_slots(agt_mailbox_t mailbox, const create_mailbox_params& params) noexcept {
    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);

    std::memset(messageSlots, 0, slotSize * slotCount);

    for ( jem_size_t i = 0; i < slotCount; ++i ) {
      auto message = new(messageSlots + (slotSize * i)) agt_message;
      message->next.address = (agt_message_t)((std::byte*)message + slotSize);
      message->mailbox.address = mailbox;
      message->thisIndex = i;
      message->signal.openHandles.store(1);
      message->signal.status = AGT_ERROR_STATUS_NOT_SET;
    }
  }

  inline agt_mailbox_t make_local_spsc_mailbox(void* memory, const create_mailbox_params& params) noexcept {
    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);
    auto firstMessage = (agt_message_t)messageSlots;
    auto lastMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 1)));
    auto penultimateMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 2)));


    auto mailbox = new(memory) agt::local_spsc_mailbox{
      .consumerSemaphore         = binary_semaphore_t{(jem_ptrdiff_t)1},
      .producerSemaphore         = binary_semaphore_t{(jem_ptrdiff_t)1},
      .slotSemaphore             = semaphore_t{(jem_ptrdiff_t)slotCount - 1},
      .producerPreviousQueuedMsg = lastMessage,
      .producerNextFreeSlot      = firstMessage,
      .queuedMessages            = semaphore_t{(jem_ptrdiff_t)0},
      .consumerPreviousMsg       = lastMessage,
      .lastFreeSlot              = penultimateMessage
    };

    mailbox->refCount            = 1;
    mailbox->kind                = agt::mailbox_kind_local_spsc;
    mailbox->messageSlots        = messageSlots;
    mailbox->slotCount           = slotCount;
    mailbox->slotSize            = slotSize;

    init_slots(mailbox, params);

    return mailbox;
  }
  inline agt_mailbox_t make_local_mpsc_mailbox(void* memory, const create_mailbox_params& params) noexcept {

    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);
    auto firstMessage = (agt_message_t)messageSlots;
    auto lastMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 1)));
    auto penultimateMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 2)));


    auto mailbox = new(memory) agt::local_mpsc_mailbox{
      .slotSemaphore           = semaphore_t{(jem_ptrdiff_t)slotCount - 2},
      .nextFreeSlot            = firstMessage,
      .lastQueuedSlot          = lastMessage,
      .lastFreeSlot            = penultimateMessage,
      .previousReceivedMessage = lastMessage,
      .queuedMessageCount      = {},
      .maxProducers            = params.maxSenders,
      .consumerSemaphore       = binary_semaphore_t{(jem_ptrdiff_t)1},
      .producerSemaphore       = semaphore_t{(jem_ptrdiff_t)params.maxSenders}
    };

    mailbox->refCount            = 1;
    mailbox->kind                = agt::mailbox_kind_local_mpsc;
    mailbox->messageSlots        = messageSlots;
    mailbox->slotCount           = slotCount;
    mailbox->slotSize            = slotSize;

    init_slots(mailbox, params);

    return mailbox;
  }
  inline agt_mailbox_t make_local_spmc_mailbox(void* memory, const create_mailbox_params& params) noexcept {
    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);
    auto firstMessage = (agt_message_t)messageSlots;
    auto lastMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 1)));
    auto penultimateMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 2)));


    auto mailbox = new(memory) agt::local_spmc_mailbox{
      .slotSemaphore           = semaphore_t{(jem_ptrdiff_t)slotCount - 2},
      .lastFreeSlot            = penultimateMessage,
      .nextFreeSlot            = firstMessage,
      .lastQueuedSlot          = lastMessage,
      .previousReceivedMessage = lastMessage,
      .queuedMessages          = semaphore_t{(jem_ptrdiff_t)0},
      .maxConsumers            = params.maxReceivers,
      .producerSemaphore       = binary_semaphore_t{(jem_ptrdiff_t)1},
      .consumerSemaphore       = semaphore_t{(jem_ptrdiff_t)params.maxReceivers}
    };

    mailbox->refCount            = 1;
    mailbox->kind                = agt::mailbox_kind_local_spmc;
    mailbox->messageSlots        = messageSlots;
    mailbox->slotCount           = slotCount;
    mailbox->slotSize            = slotSize;

    init_slots(mailbox, params);

    return mailbox;
  }
  inline agt_mailbox_t make_local_mpmc_mailbox(void* memory, const create_mailbox_params& params) noexcept {
    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);
    auto firstMessage = (agt_message_t)messageSlots;
    auto lastMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 1)));
    auto penultimateMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 2)));


    auto mailbox = new(memory) agt::local_mpmc_mailbox{
      .slotSemaphore           = semaphore_t{(jem_ptrdiff_t)slotCount - 1},
      .nextFreeSlot            = firstMessage,
      .lastQueuedSlot          = lastMessage,
      .previousReceivedMessage = lastMessage,
      .queuedMessages          = semaphore_t{(jem_ptrdiff_t)0},
      .maxProducers            = params.maxSenders,
      .maxConsumers            = params.maxReceivers,
      .producerSemaphore       = semaphore_t{(jem_ptrdiff_t)params.maxSenders},
      .consumerSemaphore       = semaphore_t{(jem_ptrdiff_t)params.maxReceivers}
    };

    mailbox->refCount            = 1;
    mailbox->kind                = agt::mailbox_kind_local_mpmc;
    mailbox->messageSlots        = messageSlots;
    mailbox->slotCount           = slotCount;
    mailbox->slotSize            = slotSize;

    init_slots(mailbox, params);

    return mailbox;
  }
  inline agt_mailbox_t make_private_mailbox(void* memory, const create_mailbox_params& params) noexcept {

    jem_size_t slotCount = params.slotCount;
    jem_size_t slotSize = params.slotSize;
    auto messageSlots = static_cast<std::byte*>(params.slotsAddress);
    auto firstMessage = (agt_message_t)messageSlots;
    auto lastMessage  = (agt_message_t)(messageSlots + (slotSize * (slotCount - 1)));

    auto mailbox = new(memory) agt::private_mailbox{
      .availableSlotCount = slotCount - 1,
      .queuedMessageCount = 0,
      .nextFreeSlot = firstMessage,
      .prevReceivedMessage = lastMessage,
      .prevQueuedMessage = lastMessage,
      .isSenderClaimed = false,
      .isReceiverClaimed = false
    };

    mailbox->refCount            = 1;
    mailbox->kind                = agt::mailbox_kind_private;
    mailbox->messageSlots        = messageSlots;
    mailbox->slotCount           = slotCount;
    mailbox->slotSize            = slotSize;

    init_slots(mailbox, params);

    return mailbox;
  }


  inline auto select_init_routine(const create_mailbox_params& params) noexcept {
    switch (params.scope) {
      case AGT_MAILBOX_SCOPE_LOCAL: {
        bool singleProducer = params.maxSenders == 1;
        bool singleConsumer = params.maxReceivers == 1;

        if (singleProducer || singleConsumer) {
          if (!singleProducer)
            return &make_local_mpsc_mailbox;
          if (!singleConsumer)
            return &make_local_spmc_mailbox;
          return &make_local_spsc_mailbox;
        }
        return &make_local_mpmc_mailbox;
      }
      case AGT_MAILBOX_SCOPE_PRIVATE:
        return &make_private_mailbox;
      case AGT_MAILBOX_SCOPE_SHARED:
      JEM_no_default;
    }
  }

  inline agt_mailbox_t init_mailbox(void* memory, const create_mailbox_params& params) {
    return select_init_routine(params)(memory, params);
  }

  inline jem_size_t align_to(jem_size_t value, jem_size_t align) {
    return ((value - 1) | (align - 1)) + 1;
  }

  inline jem_size_t get_slot_size(jem_size_t min_slot_size) noexcept {
    return (min_slot_size ? align_to(min_slot_size, JEM_CACHE_LINE) : JEM_CACHE_LINE) + JEM_CACHE_LINE;
  }
  inline jem_size_t get_slot_count(jem_size_t min_slot_count) noexcept {
    return align_to(min_slot_count + 2, 16);
  }



  inline qtz_request_t alloc_mailbox(agt_agent_t self, agt_mailbox_t* pMailbox, const char name[], create_mailbox_params& params) {

    params.slotSize = get_slot_size(params.slotSize);
    params.slotCount = get_slot_count(params.slotCount);

    struct qtz_alloc_mailbox_args {
      size_t         structSize;
      agt_mailbox_t* handleAddress;
      jem_size_t     slotSize;
      jem_size_t     slotCount;
      bool           isShared;
      char           name[JEM_CACHE_LINE*2];
    } alloc_mailbox_args{
      .structSize    = sizeof(qtz_alloc_mailbox_args),
      .handleAddress = pMailbox,
      .slotSize      = params.slotSize,
      .slotCount     = params.slotCount,
      .isShared      = params.scope == AGT_MAILBOX_SCOPE_SHARED,
      .name          = {},
    };
    std::memcpy(alloc_mailbox_args.name, name, sizeof(alloc_mailbox_args.name));

    return qtz_send(self, 3, &alloc_mailbox_args, 0, JEM_WAIT);
  }


  inline void async_create_mailbox(agt_mailbox_t* pMailbox, const agt_create_mailbox_params_t& params) {

    agt_agent_t self = agt_get_self();

    struct callback_args{
      agt_mailbox_t*        mailboxAddress;
      create_mailbox_params createParams;
    };

    struct qtz_execute_callback_args {
      jem_size_t structSize;
      void(*callback)(callback_args*);
      callback_args args;
    } callbackArgs{
      .structSize = sizeof(qtz_execute_callback_args),
      .callback = [](callback_args* args){
        args->createParams.slotsAddress = **(void***)args->mailboxAddress;
        (*args->mailboxAddress) = init_mailbox(*args->mailboxAddress, args->createParams);
      },
      .args = {
        .mailboxAddress = pMailbox,
        .createParams   = {
          .slotsAddress = nullptr,
          .slotSize     = params.min_slot_size,
          .slotCount    = params.min_slot_count,
          .maxSenders   = params.max_senders,
          .maxReceivers = params.max_receivers,
          .scope        = params.scope
        }
      }
    };

    qtz_request_discard(alloc_mailbox(self, pMailbox, params.name, callbackArgs.args.createParams));

    *(qtz_request_t*)params.async_handle_address = qtz_send(self, 22, &callbackArgs, 0, JEM_WAIT);
  }
  inline agt_status_t sync_create_mailbox(agt_mailbox_t* pMailbox, const agt_create_mailbox_params_t& params) {

    agt_agent_t self = agt_get_self();

    create_mailbox_params mailboxParams{
      .slotsAddress = nullptr,
      .slotSize     = params.min_slot_size,
      .slotCount    = params.min_slot_count,
      .maxSenders   = params.max_senders,
      .maxReceivers = params.max_receivers,
      .scope        = params.scope
    };

    switch (qtz_request_wait(alloc_mailbox(self, pMailbox, params.name, mailboxParams), JEM_WAIT)) {
      case QTZ_SUCCESS: break;
      case QTZ_ERROR_NOT_INITIALIZED: return AGT_ERROR_INITIALIZATION_FAILED;
      case QTZ_ERROR_INTERNAL: return AGT_ERROR_UNKNOWN;
      case QTZ_ERROR_BAD_SIZE: return AGT_ERROR_BAD_SIZE;
      case QTZ_ERROR_INVALID_ARGUMENT: return AGT_ERROR_INVALID_ARGUMENT;
      case QTZ_ERROR_BAD_ENCODING_IN_NAME: return AGT_ERROR_BAD_ENCODING_IN_NAME;
      case QTZ_ERROR_NAME_TOO_LONG: return AGT_ERROR_NAME_TOO_LONG;
      case QTZ_ERROR_NAME_ALREADY_IN_USE: return AGT_ERROR_NAME_ALREADY_IN_USE;
      case QTZ_ERROR_BAD_ALLOC: return AGT_ERROR_BAD_ALLOC;
      case QTZ_ERROR_NOT_IMPLEMENTED: return AGT_ERROR_NOT_YET_IMPLEMENTED;
      default: return AGT_ERROR_UNKNOWN;
    }

    // agt_mailbox_t mailbox = *pMailbox;
    std::memcpy(&mailboxParams.slotsAddress, *pMailbox, sizeof(void*));

    // mailboxParams.slotsAddress = **(void***)pMailbox;
    *pMailbox = init_mailbox(*pMailbox, mailboxParams);
    return AGT_SUCCESS;
  }

  inline void destroy_mailbox(agt_mailbox_t mailbox_) {
    switch (mailbox_->kind) {
      case agt::mailbox_kind_local_spsc:
      case agt::mailbox_kind_local_spmc:
      case agt::mailbox_kind_local_mpsc:
      case agt::mailbox_kind_local_mpmc:
      case agt::mailbox_kind_private: {
        auto mailbox = static_cast<agt::local_mailbox*>(mailbox_);
        struct free_mailbox_args{
          size_t     structSize;
          void*      handle;
          void*      slotsAddress;
          jem_size_t slotSize;
          jem_size_t slotCount;
          char       name[JEM_CACHE_LINE*2];
        } args = {
          .structSize   = sizeof(free_mailbox_args),
          .handle       = mailbox,
          .slotsAddress = mailbox->messageSlots,
          .slotSize     = mailbox->slotSize,
          .slotCount    = mailbox->slotCount,
          .name         = {}
        };
        qtz_request_discard(qtz_send(agt_get_self(), 4, &args, 0, JEM_WAIT));
      }
        break;
        // TODO: Add IPC support
      case agt::mailbox_kind_shared_spsc:
      case agt::mailbox_kind_shared_spmc:
      case agt::mailbox_kind_shared_mpsc:
      case agt::mailbox_kind_shared_mpmc:
      JEM_no_default;
    }
  }


  inline qtz_init_params_t g_initParams = {
    .kernel_version     = 0,
    .kernel_mode        = QTZ_KERNEL_INIT_OPEN_ALWAYS,
    .kernel_access_code = "agate",
    .message_slot_count = 4095,
    .process_name       = "",
    .module_count       = 0,
    .modules            = nullptr
  };

  inline constinit thread_local agt_agent_t tl_self = nullptr;


  inline bool agt_init() noexcept {
    return qtz_init(&g_initParams) == QTZ_SUCCESS;
  }

}


extern "C" {

JEM_api void          JEM_stdcall agt_set_self(agt_agent_t self) JEM_noexcept {
  tl_self = self;
}
JEM_api agt_agent_t   JEM_stdcall agt_get_self() JEM_noexcept {
  return tl_self;
}

JEM_api agt_status_t  JEM_stdcall agt_create_mailbox(agt_mailbox_t* pMailbox, const agt_create_mailbox_params_t* params) JEM_noexcept {

  if (!pMailbox || !params)
    return AGT_ERROR_INVALID_ARGUMENT;

  if (!agt_init())
    return AGT_ERROR_INITIALIZATION_FAILED;


  if ( params->async_handle_address != nullptr ) {
    async_create_mailbox(pMailbox, *params);
    return AGT_DEFERRED;
  }
  return sync_create_mailbox(pMailbox, *params);
}
JEM_api agt_mailbox_t JEM_stdcall agt_copy_mailbox(agt_mailbox_t mailbox) JEM_noexcept {
  ++mailbox->refCount;
  return mailbox;
}
JEM_api void          JEM_stdcall agt_close_mailbox(agt_mailbox_t mailbox) JEM_noexcept {
  if (!--mailbox->refCount)
    destroy_mailbox(mailbox);
}


// TODO: Implement agt_create_agent
// TODO: Implement agt_destroy_agent
JEM_api agt_status_t  JEM_stdcall agt_create_agent(agt_agent_t* pAgent) JEM_noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
JEM_api void          JEM_stdcall agt_destroy_agent(agt_agent_t agent) JEM_noexcept {}


}