//
// Created by maxwe on 2021-05-22.
//

#ifndef JEMSYS_AGATE_INTERNAL_HPP
#define JEMSYS_AGATE_INTERNAL_HPP

#include <agate/mailbox.h>

#include <atomicutils.hpp>
#include <ipc/offset_ptr.hpp>
#include <memutils.hpp>

#include <bit>


namespace {
  using address_t     = std::byte*;
  using ipc_address_t = ipc::offset_ptr<std::byte>;
}


namespace agt{

  inline constexpr static jem_u32_t PseudoMessageSlotId = static_cast<jem_u32_t>(-1);

  enum message_flags_t {
    message_in_use                = 0x0001,
    message_result_is_ready       = 0x0002,
    message_result_is_discarded   = 0x0004,
    message_result_cannot_discard = 0x0008,
    message_is_cancelled          = 0x0010,
    message_needs_cleanup         = 0x0020,
    message_has_been_received     = 0x0040,

    message_is_placeholder        = 0x8000
  };





  using PFN_start_send_message  = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
  using PFN_finish_send_message = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
  using PFN_receive_message     = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
  using PFN_discard_message     = void(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t message);
  using PFN_cancel_message      = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t message);

  using PFN_mailbox_dtor        = agt_mailbox_cleanup_callback_t;

  enum object_flag_bits_e{
    object_has_single_consumer     = 0x0001,
    object_has_single_producer     = 0x0002,
    object_variable_message_sizes  = 0x0004,
    object_is_interprocess_capable = 0x0008,
    object_is_deputy               = 0x0010,
    object_max_flag                = 0x0020
  };
  enum deputy_flag_bits_e{
    deputy_type_thread,
    deputy_type_thread_pool,
    deputy_type_lazy,
    deputy_type_proxy,
    deputy_type_virtual,
    deputy_type_collective
  };
  enum handle_flag_bits_e{
    handle_has_receive_permission = 0x01,
    handle_has_send_permission    = 0x02,
    handle_has_ownership          = 0x04,
    handle_is_interprocess        = 0x08,
    handle_is_weak_ref            = 0x10,
    handle_max_flag               = 0x20
  };
  enum object_type_id_t {
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


  template <typename T>
  struct handle_traits;
}


extern "C" {
  typedef jem_flags32_t agt_object_flags_t;
  typedef jem_flags32_t agt_handle_flags_t;

  struct agt_object{
    agt_object_flags_t flags;
    atomic_u32_t       refCount;
  };

  struct agt_mailbox : agt_object {
    ipc_address_t messageSlots;
  };
  struct alignas(2 * sizeof(void*)) agt_message {
    agt_handle_t     parent;
    atomic_flags32_t flags;
    agt_status_t     status;
    jem_size_t       payloadSize;
    // jem_size_t       nextSlot;
  };

}

namespace {

  inline constexpr jem_size_t MessageBaseSize = sizeof(agt_message);
  static_assert(MessageBaseSize == 32);

  JEM_forceinline agt_object* get_handle_object(agt_handle_t handle) noexcept {
    constexpr static jem_size_t ObjectAddressMask = ~static_cast<jem_size_t>( agt::handle_max_flag - 1 );
    return reinterpret_cast<agt_object*>(handle & ObjectAddressMask);
  }

  JEM_forceinline agt_handle_flags_t agt_internal_get_handle_flags(agt_handle_t handle) noexcept {
    constexpr static auto HandleFlagMask = static_cast<jem_size_t>( agt::handle_max_flag - 1 );
    return handle & HandleFlagMask;
  }
  JEM_forceinline agt_object_flags_t agt_internal_get_object_flags(agt_handle_t handle) noexcept {
    constexpr static jem_size_t ObjectAddressMask = ~static_cast<jem_size_t>( agt::handle_max_flag - 1 );
    return reinterpret_cast<agt_object*>(handle & ObjectAddressMask)->flags;
  }

  JEM_forceinline bool handle_has_permissions(agt_handle_t handle, jem_flags32_t flags) noexcept {
    return (agt_internal_get_handle_flags(handle) & flags) == flags;
  }

  template <typename T>
  JEM_forceinline T cast(agt_handle_t handle) noexcept {
    constexpr static jem_size_t ObjectAddressMask = ~static_cast<jem_size_t>( agt::handle_max_flag - 1 );
    return reinterpret_cast<T>(handle & ObjectAddressMask);
  }
  template <typename T>
  JEM_forceinline T dyn_cast(agt_handle_t handle) noexcept {
    using target_type = std::remove_cv_t<std::remove_pointer_t<T>>;
    constexpr static jem_flags32_t Flags = agt::handle_traits<target_type>::flags;
    if ( agt_internal_get_handle_flags(handle) != Flags )
      return nullptr;
    return cast<T>(handle);
  }


  template <typename Mailbox>
  JEM_forceinline typename Mailbox::message_type* get_message(const Mailbox* mailbox, jem_size_t slot) noexcept {
    if constexpr ( std::is_pointer_v<std::remove_cvref_t<decltype(mailbox->messageSlots)>>) {
      return reinterpret_cast<typename Mailbox::message_type*>(mailbox->messageSlots + slot);
    }
    else {
      return reinterpret_cast<typename Mailbox::message_type*>(mailbox->messageSlots.get() + slot);
    }

  }
  template <typename Mailbox>
  JEM_forceinline jem_size_t    get_slot(const Mailbox* mailbox, agt_message_t message) noexcept {
    if constexpr ( std::is_pointer_v<std::remove_cvref_t<decltype(mailbox->messageSlots)>>) {
      return reinterpret_cast<address_t>(message) - mailbox->messageSlots;
    }
    else {
      return reinterpret_cast<address_t>(message) - mailbox->messageSlots.get();
    }
  }

  JEM_forceinline void*         message_to_payload(agt_message_t message) noexcept {
    return reinterpret_cast<address_t>(message) + MessageBaseSize;
  }
  JEM_forceinline agt_message_t payload_to_message(void* payload) noexcept {
    return reinterpret_cast<agt_message_t>(static_cast<address_t>(payload) - MessageBaseSize);
  }

}

namespace agt{

  struct ipc_message   : agt_message {
    jem_size_t     nextSlot;
  };
  struct local_message : agt_message {
    local_message* nextSlot;
  };

  using ipc_message_t          = ipc_message*;
  using local_message_t        = local_message*;
  using atomic_ipc_message_t   = std::atomic<ipc_message_t>;
  using atomic_local_message_t = std::atomic<local_message_t>;

  struct local_object         : agt_object{
    inline constexpr static jem_flags32_t ipc_flag = 0;
    inline constexpr static jem_flags32_t dynamic_flag = 0;
    using message_type = local_message;
  };
  struct ipc_object           : agt_object{
    inline constexpr static jem_flags32_t ipc_flag = object_is_interprocess_capable;
    inline constexpr static jem_flags32_t dynamic_flag = 0;
    using message_type = ipc_message;
  };
  struct dynamic_local_object : agt_object {
    inline constexpr static jem_flags32_t ipc_flag = 0;
    inline constexpr static jem_flags32_t dynamic_flag = object_variable_message_sizes;
    using message_type = local_message;
  };
  struct dynamic_ipc_object   : agt_object {
    inline constexpr static jem_flags32_t ipc_flag = object_is_interprocess_capable;
    inline constexpr static jem_flags32_t dynamic_flag = object_variable_message_sizes;
    using message_type = ipc_message;
  };

  struct mpsc_mailbox {
    inline constexpr static object_type_id_t object_id = object_type_mpsc_mailbox;
  };
  struct spsc_mailbox {
    inline constexpr static object_type_id_t object_id = object_type_spsc_mailbox;
  };
  struct mpmc_mailbox {
    inline constexpr static object_type_id_t object_id = object_type_mpmc_mailbox;
  };
  struct spmc_mailbox {
    inline constexpr static object_type_id_t object_id = object_type_spmc_mailbox;
  };


  struct thread_deputy {
    inline constexpr static object_type_id_t object_id = object_type_thread_deputy;
  };
  struct thread_pool_deputy {
    inline constexpr static object_type_id_t object_id = object_type_thread_pool_deputy;
  };
  struct lazy_deputy {
    inline constexpr static object_type_id_t object_id = object_type_lazy_deputy;
  };
  struct proxy_deputy {
    inline constexpr static object_type_id_t object_id = object_type_proxy_deputy;
  };
  struct virtual_deputy {
    inline constexpr static object_type_id_t object_id = object_type_virtual_deputy;
  };
  struct collective_deputy {
    inline constexpr static object_type_id_t object_id = object_type_collective_deputy;
  };


  struct local_mpmc_mailbox               : local_object, mpmc_mailbox {
    address_t        messageSlots;
    memory_desc      msgMemory;
    jem_size_t       slotCount;
    jem_size_t       slotSize;
    semaphore_t      slotSemaphore;
  JEM_cache_aligned
    atomic_size_t    nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t    lastQueuedSlot;
  JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t     queuedSinceLastCheck; // 196
    jem_u32_t        minQueuedMessages;    // 200
    PFN_mailbox_dtor pfnDtor;              // 208
    void*            dtorUserData;         // 216
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256
  };
  struct local_mpsc_mailbox               : local_object, mpsc_mailbox {
    address_t          messageSlots;
    memory_desc        msgMemory;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_local_message_t nextFreeSlot;
    jem_size_t             payloadOffset;
  JEM_cache_aligned
    atomic_local_message_t lastQueuedSlot;
    agt_message_t          previousReceivedMessage;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxProducers;         // 220
    binary_semaphore_t consumerSemaphore;    // 221
                                             // 224 - alignment
    semaphore_t        producerSemaphore;    // 232
    // 256 - alignment
  };
  struct local_spmc_mailbox               : local_object, spmc_mailbox {
    address_t          messageSlots;
    jem_size_t         messageSize;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                             // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
  };
  struct local_spsc_mailbox               : local_object, spsc_mailbox {
    address_t                  messageSlots;
    jem_size_t                 messageSize;          // [0]: 8;  (16, 24)
    jem_size_t                 slotCount;            // [0]: 8;  (24, 32)
    jem_size_t                 slotSize;             // [0]: 8;  (32, 40)
    jem_size_t                 payloadOffset;        // [0]: 8;  (40, 48)
  JEM_cache_aligned
    semaphore_t                availableSlotSema;    // [1]: 8;  (0,   8)
    binary_semaphore_t         producerSemaphore;    // [1]: 1;  (8,   9)
                                                     // [1]: 7;  (9,  16) - alignment
    agt_message_t              producerPreviousMsg;  // [1]: 8;  (16, 24)
  JEM_cache_aligned
    semaphore_t                queuedMessageSema;    // [2]: 8;  (0,   8)
    binary_semaphore_t         consumerSemaphore;    // [2]: 1;  (8,   9)
                                                     // [2]: 7;  (9,  16) - alignment
    agt_message_t              consumerPreviousMsg;  // [2]: 8;  (16, 24)
    agt_message_t              consumerLastFreeSlot; // [2]: 8;  (24, 32)
  JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;         // [3]: 8;  (0,   8)
    PFN_mailbox_dtor           pfnDtor;              // [3]: 8;  (8,  16)
    void*                      dtorUserData;         // [3]: 8;  (16, 24)
  };
  struct dynamic_local_mpmc_mailbox       : dynamic_local_object, mpmc_mailbox { };
  struct dynamic_local_mpsc_mailbox       : dynamic_local_object, mpsc_mailbox { };
  struct dynamic_local_spmc_mailbox       : dynamic_local_object, spmc_mailbox { };
  struct dynamic_local_spsc_mailbox       : dynamic_local_object, spsc_mailbox { };
  struct ipc_mpmc_mailbox                 : ipc_object, mpmc_mailbox {
    ipc_address_t    messageSlots;
    memory_desc      msgMemory;
    jem_size_t       slotCount;
    jem_size_t       slotSize;
    semaphore_t      slotSemaphore;
  JEM_cache_aligned
    atomic_size_t    nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t    lastQueuedSlot;
  JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t     queuedSinceLastCheck; // 196
    jem_u32_t        minQueuedMessages;    // 200
    PFN_mailbox_dtor pfnDtor;              // 208
    void*            dtorUserData;         // 216
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256
  };
  struct ipc_mpsc_mailbox                 : ipc_object, mpsc_mailbox {
    ipc_address_t      messageSlots;
    memory_desc        msgMemory;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    jem_size_t         payloadOffset;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    agt_message_t      previousMessage;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    address_t          consumerSlotsAddress; // 208
    PFN_mailbox_dtor   pfnDtor;              // 216
    void*              dtorUserData;         // 224
    jem_u32_t          maxProducers;         // 228
    binary_semaphore_t consumerSemaphore;    // 229
    // 232 - alignment
    semaphore_t        producerSemaphore;    // 248
    // 256 - alignment
  };
  struct ipc_spmc_mailbox                 : ipc_object, spmc_mailbox {
    ipc_address_t      messageSlots;
    jem_size_t         messageSize;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
    // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
  };
  struct ipc_spsc_mailbox                 : ipc_object, spsc_mailbox {
    ipc_address_t              messageSlots;
    jem_size_t                 messageSize;          // [0]: 8;  (16, 24)
    jem_size_t                 slotCount;            // [0]: 8;  (24, 32)
    jem_size_t                 slotSize;             // [0]: 8;  (32, 40)
    jem_size_t                 payloadOffset;        // [0]: 8;  (40, 48)
  JEM_cache_aligned
    semaphore_t                availableSlotSema;    // [1]: 8;  (0,   8)
    binary_semaphore_t         producerSemaphore;    // [1]: 1;  (8,   9)
                                                     // [1]: 3;  (9,  12) - alignment
    jem_u32_t                  producerProcessId;    // [1]: 4;  (12, 16)
    agt_message_t              producerPreviousMsg;  // [1]: 8;  (16, 24)
    address_t                  producerSlotsAddress; // [1]: 8;  (24, 32)
  JEM_cache_aligned
    semaphore_t                queuedMessageSema;    // [2]: 8;  (0,   8)
    binary_semaphore_t         consumerSemaphore;    // [2]: 1;  (8,   9)
                                                     // [2]: 3;  (9,  12) - alignment
    jem_u32_t                  consumerProcessId;    // [2]: 4;  (12, 16)
    agt_message_t              consumerPreviousMsg;  // [2]: 8;  (16, 24)
    address_t                  consumerSlotsAddress; // [2]: 8;  (24, 32)
    agt_message_t              consumerLastFreeSlot; // [2]: 8;  (32, 40)
  JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;         // [3]: 8;  (0,   8)
    PFN_mailbox_dtor           pfnDtor;              // [3]: 8;  (8,  16)
    void*                      dtorUserData;         // [3]: 8;  (16, 24)
  };
  struct dynamic_ipc_mpmc_mailbox         : dynamic_ipc_object, mpmc_mailbox { };
  struct dynamic_ipc_mpsc_mailbox         : dynamic_ipc_object, mpsc_mailbox { };
  struct dynamic_ipc_spmc_mailbox         : dynamic_ipc_object, spmc_mailbox { };
  struct dynamic_ipc_spsc_mailbox         : dynamic_ipc_object, spsc_mailbox { };

  struct local_thread_deputy              : local_object, thread_deputy { };
  struct dynamic_local_thread_deputy      : dynamic_local_object, thread_deputy { };
  struct ipc_thread_deputy                : ipc_object, thread_deputy { };
  struct dynamic_ipc_thread_deputy        : dynamic_ipc_object, thread_deputy { };

  struct local_thread_pool_deputy         : local_object, thread_pool_deputy { };
  struct ipc_thread_pool_deputy           : dynamic_local_object, thread_pool_deputy  { };
  struct dynamic_ipc_thread_pool_deputy   : ipc_object, thread_pool_deputy { };
  struct dynamic_local_thread_pool_deputy : dynamic_ipc_object, thread_pool_deputy { };

  struct local_lazy_deputy                : local_object, lazy_deputy { };
  struct dynamic_local_lazy_deputy        : dynamic_local_object, lazy_deputy  { };
  struct ipc_lazy_deputy                  : ipc_object, lazy_deputy { };
  struct dynamic_ipc_lazy_deputy          : dynamic_ipc_object, lazy_deputy { };

  struct local_proxy_deputy               : local_object, proxy_deputy { };
  struct dynamic_local_proxy_deputy       : dynamic_local_object, proxy_deputy { };
  struct ipc_proxy_deputy                 : ipc_object, proxy_deputy { };
  struct dynamic_ipc_proxy_deputy         : dynamic_ipc_object, proxy_deputy { };

  struct local_virtual_deputy             : local_object, virtual_deputy { };
  struct dynamic_local_virtual_deputy     : dynamic_local_object, virtual_deputy { };
  struct ipc_virtual_deputy               : ipc_object, virtual_deputy { };
  struct dynamic_ipc_virtual_deputy       : dynamic_ipc_object, virtual_deputy { };

  struct local_collective_deputy          : local_object, collective_deputy { };
  struct dynamic_local_collective_deputy  : dynamic_local_object, collective_deputy { };
  struct ipc_collective_deputy            : ipc_object, collective_deputy { };
  struct dynamic_ipc_collective_deputy    : dynamic_ipc_object, collective_deputy { };

  struct private_mailbox                  : local_object {

    inline constexpr static object_type_id_t object_id = object_type_private_mailbox;

    address_t                  messageSlots;
    jem_size_t                 messageSize;
    jem_size_t                 slotCount;
    jem_size_t                 slotSize;
    PFN_mailbox_dtor           pfnDtor;
    void*                      dtorUserData;
  JEM_cache_aligned
    jem_size_t                 availableSlotCount;
    jem_size_t                 queuedMessageCount;
    agt_message_t              nextFreeSlot;
    agt_message_t              prevAcquiredSlot;
    agt_message_t              prevQueuedMessage;
    agt_message_t              prevReleasedSlot;
  };
  struct dynamic_private_mailbox          : dynamic_local_object{

    inline constexpr static object_type_id_t object_id = object_type_private_mailbox;

    address_t        messageSlots;
    jem_size_t       mailboxSize;
    jem_size_t       maxSlotSize;
    jem_size_t       slotAlignment;
    PFN_mailbox_dtor pfnDtor;
    void*            dtorUserData;
  JEM_cache_aligned
    jem_size_t       availableSpace;
    jem_size_t       queuedMessageCount;
    jem_size_t       nextWriteOffset;
    jem_size_t       nextReadOffset;
  };


  /*struct mpmc_mailbox : agt_mailbox {
    memory_desc      msgMemory;
    jem_size_t       slotCount;
    jem_size_t       slotSize;
    semaphore_t      slotSemaphore;
  JEM_cache_aligned
    atomic_size_t    nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t    lastQueuedSlot;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t     queuedSinceLastCheck; // 196
    jem_u32_t        minQueuedMessages;    // 200
    PFN_mailbox_dtor pfnDtor;              // 208
    void*            dtorUserData;         // 216
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256
  };
  struct mpsc_mailbox : agt_mailbox {
    memory_desc        msgMemory;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    jem_size_t         payloadOffset;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    agt_message_t      previousMessage;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    address_t          consumerSlotsAddress; // 208
    PFN_mailbox_dtor   pfnDtor;              // 216
    void*              dtorUserData;         // 224
    jem_u32_t          maxProducers;         // 228
    binary_semaphore_t consumerSemaphore;    // 229
                                             // 232 - alignment
    semaphore_t        producerSemaphore;    // 248
                                             // 256 - alignment
  };
  struct spmc_mailbox : agt_mailbox {
    jem_size_t         messageSize;
    jem_size_t         slotCount;
    jem_size_t         slotSize;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
  JEM_cache_aligned                          // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                             // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
  };
  struct spsc_mailbox : agt_mailbox {
    jem_size_t                 messageSize;          // [0]: 8;  (16, 24)
    jem_size_t                 slotCount;            // [0]: 8;  (24, 32)
    jem_size_t                 slotSize;             // [0]: 8;  (32, 40)
    jem_size_t                 payloadOffset;        // [0]: 8;  (40, 48)
  JEM_cache_aligned
    semaphore_t                availableSlotSema;    // [1]: 8;  (0,   8)
    binary_semaphore_t         producerSemaphore;    // [1]: 1;  (8,   9)
                                                     // [1]: 3;  (9,  12) - alignment
    jem_u32_t                  producerProcessId;    // [1]: 4;  (12, 16)
    agt_message_t              producerPreviousMsg;  // [1]: 8;  (16, 24)
    address_t                  producerSlotsAddress; // [1]: 8;  (24, 32)
  JEM_cache_aligned
    semaphore_t                queuedMessageSema;    // [2]: 8;  (0,   8)
    binary_semaphore_t         consumerSemaphore;    // [2]: 1;  (8,   9)
                                                     // [2]: 3;  (9,  12) - alignment
    jem_u32_t                  consumerProcessId;    // [2]: 4;  (12, 16)
    agt_message_t              consumerPreviousMsg;  // [2]: 8;  (16, 24)
    address_t                  consumerSlotsAddress; // [2]: 8;  (24, 32)
    agt_message_t              consumerLastFreeSlot; // [2]: 8;  (32, 40)
  JEM_cache_aligned
    std::atomic<agt_message_t> nextFreeSlot;         // [3]: 8;  (0,   8)
    PFN_mailbox_dtor           pfnDtor;              // [3]: 8;  (8,  16)
    void*                      dtorUserData;         // [3]: 8;  (16, 24)
  };


  struct mpmc_dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
  JEM_cache_aligned                                 // 192 - alignment
    PFN_mailbox_dtor          pfnDtor;              // 208
    void*                     dtorUserData;         // 216
  };
  struct mpsc_dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
  JEM_cache_aligned                                 // 192 - alignment
    PFN_mailbox_dtor          pfnDtor;              // 208
    void*                     dtorUserData;         // 216
  };
  struct spmc_dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
  JEM_cache_aligned                                 // 192 - alignment
    PFN_mailbox_dtor          pfnDtor;              // 208
    void*                     dtorUserData;         // 216
  };
  struct spsc_dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
    jem_size_t nextReadOffset;
    jem_size_t nextWriteOffset;
  JEM_cache_aligned                                 // 192 - alignment
    PFN_mailbox_dtor          pfnDtor;              // 208
    void*                     dtorUserData;         // 216
  };*/
}

using agt_local_message_t = agt::local_message*;
using agt_ipc_message_t   = agt::ipc_message*;

namespace agt{
  template <typename T>
  struct handle_traits{
    using message_type                          = typename T::message_type*;
    inline constexpr static jem_flags32_t flags = ( ( T::object_type_id << 2 ) | T::ipc_flag | T::dynamic_flag );
  };
}


#endif  //JEMSYS_AGATE_INTERNAL_HPP
