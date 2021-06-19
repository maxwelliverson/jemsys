//
// Created by maxwe on 2021-05-22.
//

#ifndef JEMSYS_AGATE_INTERNAL_HPP
#define JEMSYS_AGATE_INTERNAL_HPP

#include <agate/mailbox.h>

#include <memutils.hpp>
#include <atomicutils.hpp>



/*using atomic_u32_t = std::atomic_uint32_t;
using atomic_u64_t = std::atomic_uint64_t;
using atomic_flag_t = std::atomic_flag;*/


namespace agt{


  enum message_flags_t {
    message_in_use              = 0x01,
    message_result_is_ready     = 0x02,
    message_result_is_discarded = 0x04,
    message_is_cancelled        = 0x08,
    message_needs_cleanup       = 0x10
  };


  using PFN_start_send_message  = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
  using PFN_finish_send_message = agt_message_t(JEM_stdcall*)(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload);
  using PFN_receive_message     = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
  using PFN_discard_message     = void(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t message);
  using PFN_cancel_message      = agt_status_t(JEM_stdcall*)(agt_mailbox_t mailbox, agt_message_t message);

  using PFN_mailbox_dtor        = agt_mailbox_cleanup_callback_t;
}


extern "C" {
  struct agt_mailbox {
    agt_mailbox_flags_t flags; // 4
    jem_u32_t           kind;  // 8
  };
  struct alignas(16) agt_message {
    jem_u32_t        nextSlot;
    jem_u32_t        thisSlot;
    agt_status_t     status;
    atomic_flags32_t flags;
    std::byte        payload[];
  };
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
          semaphore_t producerSemaphore;
          semaphore_t consumerSemaphore;
        } mpmc;
        struct {
          jem_u32_t          maxProducers;
          binary_semaphore_t consumerSemaphore;
          semaphore_t        producerSemaphore;
        } mpsc;
        struct {
          jem_u32_t          maxConsumers;
          binary_semaphore_t producerSemaphore;
          semaphore_t        consumerSemaphore;
        } spmc;
        struct {
          binary_semaphore_t producerSemaphore;
          binary_semaphore_t consumerSemaphore;
        } spsc;
      };
    };
  }

  struct pseudo_mailbox{
    impl::mailbox_first_cacheline  firstCacheline;
    impl::mailbox_second_cacheline secondCacheline;
    impl::mailbox_third_cacheline  thirdCacheline;
    impl::mailbox_fourth_cacheline fourthCacheline;
  };

  struct JEM_cache_aligned static_mailbox  : agt_mailbox {
    memory_desc msgMemory;
    jem_size_t  slotCount;
    semaphore_t slotSemaphore;
  };
  struct JEM_cache_aligned dynamic_mailbox : agt_mailbox {
    jem_size_t mailboxSize;
    jem_size_t maxMessageSize;
  };

  struct mpmc_mailbox : static_mailbox {
    JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxProducers;         // 220
    jem_u32_t          maxConsumers;         // 224
    semaphore_t        producerSemaphore;    // 240
    semaphore_t        consumerSemaphore;    // 256
  };
  struct mpsc_mailbox : static_mailbox {
    JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxProducers;         // 220
    binary_semaphore_t consumerSemaphore;    // 221
                                             // 224 - alignment
    semaphore_t        producerSemaphore;    // 240
                                             // 256 - alignment
  };
  struct spmc_mailbox : static_mailbox {
    JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                             // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240
  };
  struct spsc_mailbox : static_mailbox {
    JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                        // 192 - alignment
    atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    PFN_mailbox_dtor   pfnDtor;              // 208
    void*              dtorUserData;         // 216
    binary_semaphore_t producerSemaphore;    // 217
    binary_semaphore_t consumerSemaphore;    // 218
  };


  struct mpmc_dynamic_mailbox : dynamic_mailbox {
    jem_size_t lmao[2];
  };
  struct mpsc_dynamic_mailbox : dynamic_mailbox {
    jem_size_t lmao[2];
  };
  struct spmc_dynamic_mailbox : dynamic_mailbox {
    jem_size_t lmao[2];
  };
  struct spsc_dynamic_mailbox : dynamic_mailbox {
    jem_size_t nextReadOffset;
    jem_size_t nextWriteOffset;
    jem_size_t lmao[2];
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
