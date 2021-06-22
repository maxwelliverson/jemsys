//
// Created by maxwe on 2021-05-22.
//

#ifndef JEMSYS_AGATE_INTERNAL_HPP
#define JEMSYS_AGATE_INTERNAL_HPP

#include <agate/mailbox.h>

#include <atomicutils.hpp>
#include <ipc/offset_ptr.hpp>
#include <memutils.hpp>


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

  enum agt_handle_flag_bits_e{
    handle_single_consumer        = 0x0001,
    handle_single_producer        = 0x0002,
    handle_variable_message_sizes = 0x0004,
    handle_interprocess_capable   = 0x0008,
    handle_interprocess_active    = 0x0010,
    handle_is_mailbox             = 0x0020,
    handle_is_deputy              = 0x0040,
    handle_has_read_permission    = 0x0080,
    handle_has_write_permission   = 0x0100
  };

  typedef jem_flags32_t agt_handle_flags_t;
}


extern "C" {
  struct agt_mailbox {
    agt_mailbox_flags_t flags;        // 4
    jem_u32_t           kind;         // 8
    ipc_address_t       messageSlots; // 16
  };
  struct alignas(16) agt_message {
    jem_size_t       nextSlot;
    agt_status_t     status;
    atomic_flags32_t flags;
  };
}

namespace {
  JEM_forceinline agt_message_t get_message(const agt_mailbox* mailbox, jem_size_t slot) noexcept {
    return reinterpret_cast<agt_message_t>(mailbox->messageSlots.get() + slot);
  }
  JEM_forceinline jem_size_t    get_slot(const agt_mailbox* mailbox, agt_message_t message) noexcept {
    return reinterpret_cast<address_t>(message) - mailbox->messageSlots.get();
  }
}

namespace agt{

  namespace impl{
    struct JEM_cache_aligned mailbox_first_cacheline{
      agt_mailbox_flags_t flags;          // 4
      jem_u32_t           kind;           // 8
      jem_u64_t           reserved[7];
    };
    struct JEM_cache_aligned mailbox_second_cacheline{
      jem_u64_t           reserved[8];
    };
    struct JEM_cache_aligned mailbox_third_cacheline{
      jem_u64_t           reserved[8];
    };
    struct JEM_cache_aligned mailbox_fourth_cacheline{
      jem_size_t       padding0;
      PFN_mailbox_dtor pfnDtor;       // 8
      void*            dtorUserData; // 16
      union{
        struct {
          jem_u32_t   maxProducers;
          jem_u32_t   maxConsumers;
          shared_semaphore_t producerSemaphore;
          shared_semaphore_t consumerSemaphore;
        } mpmc;
        struct {
          jem_u32_t          maxProducers;
          binary_semaphore_t consumerSemaphore;
          shared_semaphore_t producerSemaphore;
        } mpsc;
        struct {
          jem_u32_t          maxConsumers;
          binary_semaphore_t producerSemaphore;
          shared_semaphore_t consumerSemaphore;
        } spmc;
        struct {
          binary_semaphore_t producerSemaphore;
          binary_semaphore_t consumerSemaphore;
        } spsc;
      };
    };

    inline agt_message firstMessage{
      .nextSlot = 0,
      .status   = AGT_ERROR_INVALID_MESSAGE,
      .flags    = message_is_placeholder
    };

  }

  struct pseudo_mailbox{
    impl::mailbox_first_cacheline  firstCacheline;
    impl::mailbox_second_cacheline secondCacheline;
    impl::mailbox_third_cacheline  thirdCacheline;
    impl::mailbox_fourth_cacheline fourthCacheline;
  };

  /*struct JEM_cache_aligned static_mailbox  : agt_mailbox {
    memory_desc msgMemory;
    jem_size_t  slotCount;
    jem_size_t  slotSize;
  };
  struct JEM_cache_aligned dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
  };*/

  struct mpmc_mailbox : agt_mailbox {
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
    agt_message                sentinelSlot;         // [0]: 16; (48, 64)
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
  };
}

static_assert(sizeof(agt::pseudo_mailbox)  == AGT_MAILBOX_SIZE);
static_assert(alignof(agt::pseudo_mailbox) == AGT_MAILBOX_ALIGN);

static_assert(sizeof(agt::mpmc_mailbox)         == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::mpsc_mailbox)         == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::spmc_mailbox)         == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::spsc_mailbox)         == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::mpmc_dynamic_mailbox) == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::mpsc_dynamic_mailbox) == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::spmc_dynamic_mailbox) == AGT_MAILBOX_SIZE);
static_assert(sizeof(agt::spsc_dynamic_mailbox) == AGT_MAILBOX_SIZE);

static_assert(alignof(agt::mpmc_mailbox)         == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::mpsc_mailbox)         == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::spmc_mailbox)         == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::spsc_mailbox)         == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::mpmc_dynamic_mailbox) == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::mpsc_dynamic_mailbox) == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::spmc_dynamic_mailbox) == AGT_MAILBOX_ALIGN);
static_assert(alignof(agt::spsc_dynamic_mailbox) == AGT_MAILBOX_ALIGN);


#endif  //JEMSYS_AGATE_INTERNAL_HPP
