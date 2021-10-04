//
// Created by maxwe on 2021-10-04.
//

#ifndef JEMSYS_AGATE_COMMON_HPP
#define JEMSYS_AGATE_COMMON_HPP

#define JEM_SHARED_LIB_EXPORT
#include <agate.h>

#include "atomicutils.hpp"


namespace agt{
  enum message_flags_t {
    message_in_use                = 0x0001,
    message_result_is_ready       = 0x0002,
    message_result_is_discarded   = 0x0004,
    message_result_cannot_discard = 0x0008,
  };





  using PFN_mailbox_attach_sender   = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_detach_sender   = void(JEM_stdcall*)(agt_mailbox_t mailbox) noexcept;
  using PFN_mailbox_attach_receiver = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_detach_receiver = void(JEM_stdcall*)(agt_mailbox_t mailbox) noexcept;
  using PFN_mailbox_acquire_slot    = agt_slot_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_size_t messageSize, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_release_slot    = void(JEM_stdcall*)(agt_mailbox_t mailbox, agt_slot_t slot) noexcept;
  using PFN_mailbox_send            = agt_signal_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_slot_t slot) noexcept;
  using PFN_mailbox_receive         = agt_message_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_u64_t timeout_us) noexcept;


  enum mailbox_flag_bits_e{
    mailbox_has_many_consumers      = 0x0001,
    mailbox_has_many_producers      = 0x0002,
    mailbox_is_interprocess_capable = 0x0004,
    mailbox_is_private              = 0x0008,
    // mailbox_variable_message_sizes  = 0x0008,
    mailbox_max_flag                = 0x0010
  };

  enum mailbox_kind_t {
    mailbox_kind_local_spsc,
    mailbox_kind_local_spmc,
    mailbox_kind_local_mpsc,
    mailbox_kind_local_mpmc,
    mailbox_kind_shared_spsc,
    mailbox_kind_shared_spmc,
    mailbox_kind_shared_mpsc,
    mailbox_kind_shared_mpmc,
    mailbox_kind_private
  };


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

extern "C" {
  struct agt_signal{
    atomic_flags32_t flags;
    agt_status_t     status;
  };

  struct JEM_cache_aligned agt_message{
    jem_flags32_t flags;
    union {
      jem_size_t   index;
      agt_message* address;
    }   next;
    union {
      jem_ptrdiff_t offset;
      agt_mailbox_t address;
    }   mailbox;
    jem_size_t    thisIndex;
    jem_size_t    payloadSize;
    agt_signal    signal;
    agt_agent_t   sender;
    jem_u64_t     id;
    JEM_cache_aligned
    char          payload[];
  };



  struct agt_mailbox {
    agt::mailbox_kind_t kind;
  };



  struct agt_agent {

    agt_actor actor;


  };



}

namespace agt{
  struct local_mailbox : agt_mailbox {

  };
  struct shared_mailbox : agt_mailbox {

  };

  struct local_agent : agt_agent {
    virtual ~local_agent();

    virtual agt_slot_t   acquire_slot(jem_size_t slot_size, jem_u64_t timeout_us) noexcept = 0;
    virtual void         release_slot(agt_slot_t slot) noexcept = 0;
    virtual agt_signal_t send(agt_slot_t slot) noexcept = 0;
  };
  struct shared_agent : agt_agent {

  };
}

static_assert(sizeof(agt_message) == JEM_CACHE_LINE);


#endif//JEMSYS_AGATE_COMMON_HPP
