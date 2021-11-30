//
// Created by maxwe on 2021-11-03.
//

#ifndef JEMSYS_AGATE2_INTERNAL_HPP
#define JEMSYS_AGATE2_INTERNAL_HPP

#include <agate2.h>

#include <quartz.h>

#include "handle.hpp"
#include "objects.hpp"
#include "ipc_block.hpp"
#include "pool.hpp"

namespace agt{

  enum class async_state {
    empty,
    ready,
    waiting
  };
  enum class async_data_type {
    message,
    quartz_request
  };

  enum async_flags {
    async_data_is_external = 0x1,
    async_is_copy          = 0x2
  };

  enum message_flags {
    message_must_check_result = 0x1,
    message_no_async_waiter   = 0x2,
    message_fast_cleanup      = 0x4,
    message_in_use            = 0x8,
    message_has_been_checked  = 0x10,
    message_is_condemned      = 0x20,
    MESSAGE_HIGH_BIT_PLUS_ONE,
    message_all_flags = (((MESSAGE_HIGH_BIT_PLUS_ONE - 1) << 1) - 1)
  };
  enum slot_flags {
    channel_is_shared      = 0x1,
    slot_is_in_use         = 0x2 // not currently used...
  };

  enum mailbox_flag_bits_e{
    mailbox_has_many_consumers      = 0x0001,
    mailbox_has_many_producers      = 0x0002,
    mailbox_is_interprocess_capable = 0x0004,
    mailbox_is_private              = 0x0008,
    // mailbox_variable_message_sizes  = 0x0008,
    mailbox_max_flag                = 0x0010
  };

  enum channel_kind {
    channel_kind_local_spsc,
    channel_kind_local_spmc,
    channel_kind_local_mpsc,
    channel_kind_local_mpmc,
    channel_kind_shared_spsc,
    channel_kind_shared_spmc,
    channel_kind_shared_mpsc,
    channel_kind_shared_mpmc,
    channel_kind_private,
    CHANNEL_KIND_MAX_ENUM
  };


  struct process;
  struct channel;

  /*enum object_type_id_t {
    object_type_mpsc_mailbox,
    object_type_spsc_mailbox,
    object_type_mpmc_mailbox,
    object_type_spmc_mailbox,
    object_type_thread_deputy,
    object_type_thread_pool_deputy,
    object_type_lazy_deputy,
    object_type_proxy_deputy,
    object_type_virtual_deputy,
    object_type_collective_deputy,
    object_type_private_mailbox
  };

  enum object_kind_t {
    object_kind_local_mpsc_mailbox,
    object_kind_dynamic_local_mpsc_mailbox,
    object_kind_ipc_mpsc_mailbox,
    object_kind_dynamic_ipc_mpsc_mailbox,

    object_kind_local_spsc_mailbox,
    object_kind_dynamic_local_spsc_mailbox,
    object_kind_ipc_spsc_mailbox,
    object_kind_dynamic_ipc_spsc_mailbox,

    object_kind_local_mpmc_mailbox,
    object_kind_dynamic_local_mpmc_mailbox,
    object_kind_ipc_mpmc_mailbox,
    object_kind_dynamic_ipc_mpmc_mailbox,

    object_kind_local_spmc_mailbox,
    object_kind_dynamic_local_spmc_mailbox,
    object_kind_ipc_spmc_mailbox,
    object_kind_dynamic_ipc_spmc_mailbox,

    object_kind_local_thread_deputy,
    object_kind_dynamic_local_thread_deputy,
    object_kind_ipc_thread_deputy,
    object_kind_dynamic_ipc_thread_deputy,

    object_kind_local_thread_pool_deputy,
    object_kind_dynamic_local_thread_pool_deputy,
    object_kind_ipc_thread_pool_deputy,
    object_kind_dynamic_ipc_thread_pool_deputy,

    object_kind_local_lazy_deputy,
    object_kind_dynamic_local_lazy_deputy,
    object_kind_ipc_lazy_deputy,
    object_kind_dynamic_ipc_lazy_deputy,

    object_kind_local_proxy_deputy,
    object_kind_dynamic_local_proxy_deputy,
    object_kind_ipc_proxy_deputy,
    object_kind_dynamic_ipc_proxy_deputy,

    object_kind_local_virtual_deputy,
    object_kind_dynamic_local_virtual_deputy,
    object_kind_ipc_virtual_deputy,
    object_kind_dynamic_ipc_virtual_deputy,

    object_kind_local_collective_deputy,
    object_kind_dynamic_local_collective_deputy,
    object_kind_ipc_collective_deputy,
    object_kind_dynamic_ipc_collective_deputy,

    object_kind_private_mailbox,
    object_kind_dynamic_private_mailbox
  };*/
}


namespace Agt{

  enum class AsyncState {
    empty,
    ready,
    waiting
  };
  enum class AsyncDataType {
    message,
    quartz_request
  };

  enum AsyncFlagBits {
    async_data_is_external = 0x1,
    async_is_copy          = 0x2
  };

  enum MessageFlags {
    message_must_check_result = 0x1,
    message_no_async_waiter   = 0x2,
    message_fast_cleanup      = 0x4,
    message_in_use            = 0x8,
    message_has_been_checked  = 0x10,
    message_is_condemned      = 0x20,
    MESSAGE_HIGH_BIT_PLUS_ONE,
    message_all_flags = (((MESSAGE_HIGH_BIT_PLUS_ONE - 1) << 1) - 1)
  };
  enum SlotFlags {
    channel_is_shared      = 0x1,
    slot_is_in_use         = 0x2 // not currently used...
  };

  enum MailboxFlagBits{
    mailbox_has_many_consumers      = 0x0001,
    mailbox_has_many_producers      = 0x0002,
    mailbox_is_interprocess_capable = 0x0004,
    mailbox_is_private              = 0x0008,
    // mailbox_variable_message_sizes  = 0x0008,
    mailbox_max_flag                = 0x0010
  };

  enum ChannelKind {
    channel_kind_local_spsc,
    channel_kind_local_spmc,
    channel_kind_local_mpsc,
    channel_kind_local_mpmc,
    channel_kind_shared_spsc,
    channel_kind_shared_spmc,
    channel_kind_shared_mpsc,
    channel_kind_shared_mpmc,
    channel_kind_private,
    CHANNEL_KIND_MAX_ENUM
  };


  struct Process;
  struct Channel;

  /*enum object_type_id_t {
    object_type_mpsc_mailbox,
    object_type_spsc_mailbox,
    object_type_mpmc_mailbox,
    object_type_spmc_mailbox,
    object_type_thread_deputy,
    object_type_thread_pool_deputy,
    object_type_lazy_deputy,
    object_type_proxy_deputy,
    object_type_virtual_deputy,
    object_type_collective_deputy,
    object_type_private_mailbox
  };

  enum object_kind_t {
    object_kind_local_mpsc_mailbox,
    object_kind_dynamic_local_mpsc_mailbox,
    object_kind_ipc_mpsc_mailbox,
    object_kind_dynamic_ipc_mpsc_mailbox,

    object_kind_local_spsc_mailbox,
    object_kind_dynamic_local_spsc_mailbox,
    object_kind_ipc_spsc_mailbox,
    object_kind_dynamic_ipc_spsc_mailbox,

    object_kind_local_mpmc_mailbox,
    object_kind_dynamic_local_mpmc_mailbox,
    object_kind_ipc_mpmc_mailbox,
    object_kind_dynamic_ipc_mpmc_mailbox,

    object_kind_local_spmc_mailbox,
    object_kind_dynamic_local_spmc_mailbox,
    object_kind_ipc_spmc_mailbox,
    object_kind_dynamic_ipc_spmc_mailbox,

    object_kind_local_thread_deputy,
    object_kind_dynamic_local_thread_deputy,
    object_kind_ipc_thread_deputy,
    object_kind_dynamic_ipc_thread_deputy,

    object_kind_local_thread_pool_deputy,
    object_kind_dynamic_local_thread_pool_deputy,
    object_kind_ipc_thread_pool_deputy,
    object_kind_dynamic_ipc_thread_pool_deputy,

    object_kind_local_lazy_deputy,
    object_kind_dynamic_local_lazy_deputy,
    object_kind_ipc_lazy_deputy,
    object_kind_dynamic_ipc_lazy_deputy,

    object_kind_local_proxy_deputy,
    object_kind_dynamic_local_proxy_deputy,
    object_kind_ipc_proxy_deputy,
    object_kind_dynamic_ipc_proxy_deputy,

    object_kind_local_virtual_deputy,
    object_kind_dynamic_local_virtual_deputy,
    object_kind_ipc_virtual_deputy,
    object_kind_dynamic_ipc_virtual_deputy,

    object_kind_local_collective_deputy,
    object_kind_dynamic_local_collective_deputy,
    object_kind_ipc_collective_deputy,
    object_kind_dynamic_ipc_collective_deputy,

    object_kind_private_mailbox,
    object_kind_dynamic_private_mailbox
  };*/
}



enum class object_type : jem_u32_t {

};




extern "C" {






struct AgtAsyncData_st {
  atomic_flag_t readyFlag;
  atomic_u32_t  refCount;
  AgtStatus     status;
};

struct JEM_cache_aligned AgtAsync_st {
  agt::async_state     state;
  agt::async_data_type type;
  jem_flags32_t        flags;
  AgtStatus            status;
  union {
    AgtAsyncData_st* data;
    qtz_request_t    quartzRequest;
  };
};

struct AgtActor_st {
  AgtTypeId               type;
  PFN_agtActorMessageProc pfnMessageProc;
  PFN_agtActorDtor        pfnDtor;
  void*                   state;
};

struct AgtProcess_st {};
struct AgtThread_st {};
struct AgtAgency_st {};
struct AgtSender_st {};
struct AgtReceiver_st {};
struct AgtAgent_st {};

struct JEM_cache_aligned AgtMessage_st {
  union {
    jem_size_t index;
    AgtMessage address;
  }      next;
  union {
    jem_size_t    offset;
    agt::channel* address;
  }      channel;
  jem_size_t       thisIndex;
  AgtSize          payloadSize;
  AgtAsyncData_st  asyncData;
  atomic_flags32_t flags;
  AgtAgent         sender;
  jem_u64_t        id;
  JEM_cache_aligned
  char             payload[];
};

struct JEM_alignas(JEM_CACHE_LINE / 2) AgtHandle_st {
  AgtObjectKind kind;
  AgtObject_st* object;
  agt::process* handleOwner;
  agt::process* objectOwner;
};

}






namespace {

  JEM_forceinline agt::channel* to_channel(AgtMessage message) noexcept {
    return (message->slotFlags & agt::channel_is_shared)
             ? (agt::channel*)(((uintptr_t)message) + message->channel.offset)
             : message->channel.address;
  }
  JEM_forceinline agt::channel* to_channel(const AgtStagedMessage* stagedMessage) noexcept {
    return to_channel((AgtMessage)stagedMessage->cookie);
  }

  inline void cleanup_message(AgtMessage message) noexcept {
    message->signal.isReady.reset();
    message->signal.openHandles.store(1, std::memory_order_relaxed);
    if ( !(message->signal.flags.fetch_and_clear() & agt::message_fast_cleanup) ) {
      std::memset(message->payload, 0, message->payloadSize);
      message->sender = nullptr;
      message->id     = 0;
      message->signal.status = AGT_ERROR_STATUS_NOT_SET;
    }
  }
  inline void destroy_message(agt::channel* channel, AgtMessage message) noexcept {
    cleanup_message(message);
    agt::vtable_table[mailbox->kind].pfn_return_message(mailbox, message);
  }

  inline void condemn(agt::channel* channel, AgtMessage message) noexcept {
    if ( !(message->messageFlags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(channel, message);
  }
  inline void condemn(AgtMessage message) noexcept {
    if ( !(message->messageFlags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(to_channel(message), message);
  }

  inline void destroy(AgtAsyncData_st* data) noexcept {
    if (!data->isInline)
      delete data;
  }

  inline void clearData(AgtAsyncData_st* data, agt::async_data_type type) noexcept {
    switch (type) {
      case agt::async_data_type::message: {
        auto oldMessage = std::exchange(data->direct.message, nullptr);
        auto oldFlags = oldMessage->messageFlags.fetch_and_set(agt::message_no_async_waiter | agt::message_is_condemned);
        if (!(oldFlags & agt::message_in_use))
          destroy_message(to_channel(oldMessage), oldMessage);
      }
      case agt::async_data_type::quartz_request:
        qtz_discard(data->quartzRequest);
        break;
      JEM_no_default;
    }
  }

  inline void detach(AgtAsyncData_st* data, agt::async_data_type type) noexcept {
    if (--data->refCount == 0) {
      clearData(data, type);
      destroy(data);
    }
  }

  inline void clear(AgtAsync async) noexcept {

    if (async->state == agt::async_state::waiting) {

      if (async->flags & agt::async_data_is_external) {
        if (--async->data->refCount == 0) {
          clearData(async->data, async->type);
        }
        if (async->flags & agt::async_is_copy)
          detach(async->data, async->type);
        else {
          if (async->inlineData.refCount == 0) {

          }
        }
      }
      else if (async->data->refCount == 1)
        clearData(async->data, async->type);
    }

    switch (async->state) {
      case agt::async_state::empty: return;
      case agt::async_state::ready: break;
      case agt::async_state::waiting:
        break;
      JEM_no_default;
    }

    switch (async->type) {
      case agt::async_data_type::message:
        break;
      case agt::async_data_type::quartz_request:
        qtz_discard(async->quartzRequest);
        break;
    }

    switch (async->state) {
      case agt::async_state::quartz_request:

        qtz_discard(async->quartzRequest);
        [[fallthrough]];
      case agt::async_state::ready:
        break;
      case agt::async_state::empty:
        return;

      case agt::async_state::direct:
        break;
      case agt::async_state::indirect: {
        auto instance = async->indirectHandle;
        if (!--instance->waiters) {

        }
      }

        break;
      JEM_no_default;
    }

    if (!async->dataIsInline) {
      detach(async->data);
    }

    async->state = agt::async_state::empty;
  }
}

static_assert(sizeof(AgtMessage_st) == JEM_CACHE_LINE);
static_assert(sizeof(AgtHandle_st) == (JEM_CACHE_LINE / 2));
static_assert(sizeof(AgtAsync_st) == JEM_CACHE_LINE);




#endif//JEMSYS_AGATE2_INTERNAL_HPP
