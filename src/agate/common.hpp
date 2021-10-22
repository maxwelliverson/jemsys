//
// Created by maxwe on 2021-10-04.
//

#ifndef JEMSYS_AGATE_COMMON_HPP
#define JEMSYS_AGATE_COMMON_HPP

#define JEM_SHARED_LIB_EXPORT
#include <agate.h>

#include "atomicutils.hpp"
#include "ipc/offset_ptr.hpp"


namespace agt{
  enum message_flags_t {
    message_must_check_result = 0x1,
    message_ignore_result     = 0x2,
    message_fast_cleanup      = 0x4,
    message_in_use            = 0x8,
    message_has_been_checked  = 0x10,
    message_is_condemned      = 0x20,
    MESSAGE_HIGH_BIT_PLUS_ONE,
    message_all_flags = (((MESSAGE_HIGH_BIT_PLUS_ONE - 1) << 1) - 1)
  };
  enum slot_flags_t {
    slot_is_shared         = 0x1,
    slot_is_in_use         = 0x2 // not currently used...
  };





  using PFN_mailbox_attach_sender   = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_detach_sender   = void(JEM_stdcall*)(agt_mailbox_t mailbox) noexcept;
  using PFN_mailbox_attach_receiver = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_detach_receiver = void(JEM_stdcall*)(agt_mailbox_t mailbox) noexcept;
  using PFN_mailbox_acquire_slot    = agt_mailslot_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_size_t messageSize, jem_u64_t timeout_us) noexcept;
  using PFN_mailbox_release_slot    = void(JEM_stdcall*)(agt_mailbox_t mailbox, agt_mailslot_t slot) noexcept;
  using PFN_mailbox_send            = agt_signal_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_mailslot_t slot) noexcept;
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
    mailbox_kind_private,
    MAILBOX_KIND_MAX_ENUM
  };

  enum mailbox_role_t {
    MAILBOX_ROLE_SENDER,
    MAILBOX_ROLE_RECEIVER
  };

  enum mailbox_op_kind_t {
    MAILBOX_OP_KIND_ATTACH,
    MAILBOX_OP_KIND_DETACH
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
    atomic_flag_t    isReady;
    atomic_u32_t     openHandles;
    agt_status_t     status;
  };

  struct JEM_cache_aligned agt_message{
    union {
      jem_size_t   index;
      agt_message* address;
    }   next;
    union {
      jem_ptrdiff_t offset;
      agt_mailbox_t address;
    }   mailbox;
    jem_size_t    thisIndex;
    jem_flags32_t flags;
    jem_u32_t     payloadSize;
    agt_signal    signal;
    agt_agent_t   sender;
    jem_u64_t     id;
    JEM_cache_aligned
    char          payload[];
  };



  struct agt_mailbox {
    atomic_u32_t        refCount;
    agt::mailbox_kind_t kind;
  };



  struct agt_agent {

    agt_actor actor;


  };



}

namespace agt{
  struct local_mailbox : agt_mailbox {
    std::byte*         messageSlots;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
  };
  struct local_spsc_mailbox : local_mailbox {
    binary_semaphore_t         consumerSemaphore;    // [2]: 1;  (8,   9)
    binary_semaphore_t         producerSemaphore;    // [1]: 1;  (8,   9)
    JEM_cache_aligned
    semaphore_t                slotSemaphore;    // [1]: 8;  (0,   8)
    agt_message_t              producerPreviousQueuedMsg;  // [1]: 8;  (16, 24)
    agt_message_t              producerNextFreeSlot;         // [3]: 8;  (0,   8)

    JEM_cache_aligned
    semaphore_t                queuedMessages;    // [2]: 8;  (0,   8)
    agt_message_t              consumerPreviousMsg;  // [2]: 8;  (16, 24)

    JEM_cache_aligned
    std::atomic<agt_message_t> lastFreeSlot; // [2]: 8;  (24, 32)
  };
  struct local_mpsc_mailbox : local_mailbox {

    semaphore_t                slotSemaphore;
    JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;

    JEM_cache_aligned
    std::atomic<agt_message_t> lastQueuedSlot;
    std::atomic<agt_message_t> lastFreeSlot;
    agt_message_t              previousReceivedMessage;
    JEM_cache_aligned
    mpsc_counter_t     queuedMessageCount;
    jem_u32_t          maxProducers;
    binary_semaphore_t consumerSemaphore;
    semaphore_t        producerSemaphore;
  };
  struct local_spmc_mailbox : local_mailbox {
    semaphore_t        slotSemaphore;
    JEM_cache_aligned
    std::atomic<agt_message_t> lastFreeSlot;
    agt_message_t              nextFreeSlot;
    agt_message_t              lastQueuedSlot;
    JEM_cache_aligned
    std::atomic<agt_message_t> previousReceivedMessage;
    JEM_cache_aligned                          // 192 - alignment
    semaphore_t        queuedMessages;
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                         // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
  };
  struct local_mpmc_mailbox : local_mailbox {
    semaphore_t                slotSemaphore;
    JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;
    std::atomic<agt_message_t> lastQueuedSlot;
    JEM_cache_aligned
    std::atomic<agt_message_t> previousReceivedMessage;
    JEM_cache_aligned
    semaphore_t                queuedMessages; // 196
    jem_u32_t                  maxProducers;         // 220
    jem_u32_t                  maxConsumers;         // 224
    semaphore_t                producerSemaphore;    // 240
    semaphore_t                consumerSemaphore;    // 256
  };


  struct shared_mailbox : agt_mailbox {
    ipc::offset_ptr<std::byte> messageSlots;
    jem_size_t                 slotCount;
    jem_size_t                 slotSize;
  };
  struct shared_spsc_mailbox : shared_mailbox {
    std::byte*         producerSlotAddr;
    std::byte*         consumerSlotAddr;
    JEM_cache_aligned
    semaphore_t        availableSlotSema;    // [1]: 8;  (0,   8)
    binary_semaphore_t producerSemaphore;    // [1]: 1;  (8,   9)
                                         // [1]: 7;  (9,  16) - alignment
    agt_message_t      producerPreviousMsg;  // [1]: 8;  (16, 24)
    JEM_cache_aligned
    semaphore_t        queuedMessageSema;    // [2]: 8;  (0,   8)
    binary_semaphore_t consumerSemaphore;    // [2]: 1;  (8,   9)
                                         // [2]: 7;  (9,  16) - alignment
    agt_message_t      consumerPreviousMsg;  // [2]: 8;  (16, 24)
    agt_message_t      consumerLastFreeSlot; // [2]: 8;  (24, 32)
    JEM_cache_aligned
    atomic_size_t      nextFreeSlotIndex;         // [3]: 8;  (0,   8)
    JEM_cache_aligned
    std::byte          slots[];
  };
  struct shared_mpsc_mailbox : shared_mailbox {
    shared_semaphore_t slotSemaphore;
    JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;
    jem_size_t                 payloadOffset;
    JEM_cache_aligned
    std::atomic<agt_message_t> lastQueuedSlot;
    agt_message_t              previousReceivedMessage;
    JEM_cache_aligned
    mpsc_counter_t     queuedMessageCount;
    jem_u32_t          maxProducers;
    binary_semaphore_t consumerSemaphore;
    semaphore_t        producerSemaphore;
    JEM_cache_aligned
    std::byte          slots[];
  };
  struct shared_spmc_mailbox : shared_mailbox {
    semaphore_t        slotSemaphore;
    JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                         // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
    JEM_cache_aligned
    std::byte          slots[];
  };
  struct shared_mpmc_mailbox : shared_mailbox {
    semaphore_t      slotSemaphore;
    JEM_cache_aligned
    atomic_size_t    nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t    lastQueuedSlot;
    JEM_cache_aligned
    atomic_u32_t     queuedSinceLastCheck;
    jem_u32_t        minQueuedMessages;
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256
    JEM_cache_aligned
    std::byte          slots[];
  };

  struct private_mailbox : agt_mailbox {
    std::byte*               messageSlots;
    jem_size_t               slotCount;
    jem_size_t               slotSize;
    // JEM_cache_aligned
    jem_size_t               availableSlotCount;
    jem_size_t               queuedMessageCount;
    agt_message_t            nextFreeSlot;
    agt_message_t            prevReceivedMessage;
    agt_message_t            prevQueuedMessage;
    bool                     isSenderClaimed;
    bool                     isReceiverClaimed;
  };

  struct local_agent : agt_agent {
    virtual ~local_agent();

    virtual agt_mailslot_t   acquire_slot(jem_size_t slot_size, jem_u64_t timeout_us) noexcept = 0;
    virtual void         release_slot(agt_mailslot_t slot) noexcept = 0;
    virtual agt_signal_t send(agt_mailslot_t slot) noexcept = 0;
  };
  struct shared_agent : agt_agent {

  };


  struct JEM_cache_aligned mailbox_vtable{
    agt_mailslot_t(   JEM_stdcall* pfn_acquire_slot)(agt_mailbox_t mailbox, jem_size_t messageSize, jem_u64_t timeout_us) JEM_noexcept;
    agt_signal_t( JEM_stdcall* pfn_send)(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t(JEM_stdcall* pfn_receive)(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    agt_status_t( JEM_stdcall* pfn_try_operation)(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) JEM_noexcept;
    void(         JEM_stdcall* pfn_return_message)(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;
  };
  
  
  
  extern const mailbox_vtable vtable_table[];
  
  namespace impl{

    agt_status_t  JEM_stdcall local_spsc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_mailslot_t    JEM_stdcall local_spsc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    agt_signal_t  JEM_stdcall local_spsc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall local_spsc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_spsc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall local_mpsc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall local_mpsc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpsc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall local_mpsc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpsc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall local_mpsc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall local_mpsc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpsc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall local_spmc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall local_spmc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_spmc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall local_spmc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_spmc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall local_spmc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall local_spmc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_spmc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall local_mpmc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall local_mpmc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpmc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall local_mpmc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpmc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall local_mpmc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall local_mpmc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall local_mpmc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall shared_spsc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall shared_spsc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spsc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall shared_spsc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spsc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall shared_spsc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall shared_spsc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spsc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall shared_mpsc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall shared_mpsc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpsc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall shared_mpsc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpsc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall shared_mpsc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall shared_mpsc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpsc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall shared_spmc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall shared_spmc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spmc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall shared_spmc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spmc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall shared_spmc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall shared_spmc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_spmc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall shared_mpmc_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall shared_mpmc_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpmc_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall shared_mpmc_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpmc_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall shared_mpmc_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall shared_mpmc_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall shared_mpmc_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;


    agt_status_t  JEM_stdcall private_try_operation(agt_mailbox_t mailbox, mailbox_role_t role, mailbox_op_kind_t opKind, jem_u64_t timeout_us) noexcept;
    agt_status_t  JEM_stdcall private_attach(agt_mailbox_t mailbox, bool isSender, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall private_detach(agt_mailbox_t mailbox, bool isSender) JEM_noexcept;
    agt_mailslot_t    JEM_stdcall private_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall private_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept;
    agt_signal_t  JEM_stdcall private_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept;
    agt_message_t JEM_stdcall private_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
    void          JEM_stdcall private_return_message(agt_mailbox_t mailbox, agt_message_t message) JEM_noexcept;

  }

}




namespace {

  inline agt_message_t to_message(agt_mailslot_t slot) noexcept {
    return (agt_message_t)slot;
  }
  inline agt_message_t to_message(agt_signal_t signal) noexcept {
    return (agt_message_t)(((std::byte*)signal) - offsetof(agt_message, signal));
  }
  inline agt_mailbox_t to_mailbox(agt_message_t message) noexcept {
    return (message->flags & agt::slot_is_shared)
             ? (agt_mailbox_t)(((std::byte*)message) + message->mailbox.offset)
             : message->mailbox.address;
  }
  inline agt_mailbox_t to_mailbox(agt_mailslot_t slot) noexcept {
    return to_mailbox(to_message(slot));
  }

  inline void cleanup_message(agt_message_t message) noexcept {
    message->signal.isReady.reset();
    message->signal.openHandles.store(1, std::memory_order_relaxed);
    if ( !(message->signal.flags.fetch_and_clear() & agt::message_fast_cleanup) ) {
      std::memset(message->payload, 0, message->payloadSize);
      message->sender = nullptr;
      message->id     = 0;
      message->signal.status = AGT_ERROR_STATUS_NOT_SET;
    }
  }
  inline void destroy_message(agt_mailbox_t mailbox, agt_message_t message) noexcept {
    cleanup_message(message);
    agt::vtable_table[mailbox->kind].pfn_return_message(mailbox, message);
  }
  
  template <typename Mailbox>
  void return_message(Mailbox* mailbox, agt_message_t message) noexcept;
  
  template <> JEM_forceinline void return_message(agt::local_mpsc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::local_mpsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_spsc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::local_spsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_mpmc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::local_mpmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_spmc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::local_spmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_mpsc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::shared_mpsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_spsc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::shared_spsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_mpmc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::shared_mpmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_spmc_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::shared_spmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::private_mailbox* mailbox, agt_message_t message) noexcept {
    agt::impl::private_return_message(mailbox, message);
  }

  inline void place_hold(agt_message_t message) noexcept {
    message->signal.flags.set(agt::message_in_use);
  }
  template <typename Mailbox>
  inline void free_hold(Mailbox* mailbox, agt_message_t message) noexcept {
    if ( message->signal.flags.fetch_and_reset(agt::message_in_use) & agt::message_is_condemned ) {
      cleanup_message(message);
      return_message(mailbox, message);
    }
  }

  inline void condemn(agt_mailbox_t mailbox, agt_message_t message) noexcept {
    if ( !(message->signal.flags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(mailbox, message);
  }
  inline void condemn(agt_message_t message) noexcept {
    if ( !(message->signal.flags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(to_mailbox(message), message);
  }
  inline void condemn(agt_signal_t signal) noexcept {
    condemn(to_message(signal));
  }
}

static_assert(sizeof(agt_message) == JEM_CACHE_LINE);


#endif//JEMSYS_AGATE_COMMON_HPP
