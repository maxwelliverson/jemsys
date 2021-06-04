//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_INTERNAL_H
#define JEMSYS_QUARTZ_INTERNAL_H

#define JEM_SHARED_LIB_EXPORT

#include <quartz/core.h>


#include <atomic>
#include <semaphore>
#include <span>




namespace qtz {
  namespace {

    struct small_alloc { };
    struct medium_alloc{ };
    struct large_alloc { };
    struct huge_alloc  { };

    struct page_allocation_handle{
      jem_u16_t pageCount;
      jem_u16_t smallPageAddress;
      jem_u16_t hugePageOffset;
      jem_u8_t  purpose;
      jem_u8_t  lifetime;
    };

    struct page_4gb_desc  { }; // 16
    struct page_256mb_desc{ }; // 16
    struct page_16mb_desc { }; // 16
    struct page_1mb_desc  { }; // 16
    struct page_64kb_desc { }; // 16
    struct page_4kb_desc  { }; // 16

    template <typename Word, size_t Bits = std::dynamic_extent>
    struct bitmap{

      inline constexpr static size_t Extent = Bits == std::dynamic_extent ? Bits : (Bits / (sizeof(Word) * 8));

      std::span<Word, Extent> fields;
    };


    struct page_map{
      jem_u32_t  hugePageCount;
      struct {}* hugePages;
    };

    struct memory_manager{};

    struct message_type{};
    struct message_module{};





    struct epoch_tracker{
      jem_u64_t currentEpoch;


      void         advance() noexcept;

      jem_status_t ensure_lifetime() noexcept;
      jem_status_t extend_lifetime() noexcept;


    };


    struct deputy_base{
      virtual ~deputy_base() = default;
    };
    struct single_thread_deputy : deputy_base{};
    struct thread_pool_deputy   : deputy_base{};
    struct lazy_deputy          : deputy_base{};
    struct proxy_deputy         : deputy_base{};
  }
}

using atomic_u32_t  = std::atomic_uint32_t;
using atomic_u64_t  = std::atomic_uint64_t;
using atomic_flag_t = std::atomic_flag;

using jem_semaphore_t        = std::counting_semaphore<>;
using jem_binary_semaphore_t = std::binary_semaphore;

typedef jem_u64_t jem_bitmap_field_t;


#define JEM_BITS_PER_BITMAP_FIELD (sizeof(jem_bitmap_field_t) * 8)
#define JEM_VIRTUAL_REGION_SIZE (JEM_VIRTUAL_PAGE_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_VIRTUAL_SEGMENT_SIZE (JEM_VIRTUAL_REGION_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_GLOBAL_VIRTUAL_MEMORY_MAX (JEM_VIRTUAL_SEGMENT_SIZE * JEM_BITS_PER_BITMAP_FIELD) // 16GB




/*typedef struct jem_global_message{
  jem_u32_t nextIndex;

} jem_global_message, * jem_global_message_t;*/

#if JEM_system_windows
typedef unsigned long qtz_exit_code_t;
#else
typedef int           qtz_exit_code_t;
#endif


typedef enum {
  GLOBAL_MESSAGE_KIND_ALLOC_PAGES,
  GLOBAL_MESSAGE_KIND_FREE_PAGES,
  GLOBAL_MESSAGE_KIND_ALLOCATE_STATIC_MAILBOX,
  GLOBAL_MESSAGE_KIND_DEALLOCATE_STATIC_MAILBOX,
  GLOBAL_MESSAGE_KIND_ALLOCATE_DYNAMIC_MAILBOX,
  GLOBAL_MESSAGE_KIND_DEALLOCATE_DYNAMIC_MAILBOX,
  GLOBAL_MESSAGE_KIND_ALLOCATE_STATIC_MAILBOX_SHARED,
  GLOBAL_MESSAGE_KIND_DEALLOCATE_STATIC_MAILBOX_SHARED,
  GLOBAL_MESSAGE_KIND_ALLOCATE_DYNAMIC_MAILBOX_SHARED,
  GLOBAL_MESSAGE_KIND_DEALLOCATE_DYNAMIC_MAILBOX_SHARED,
  GLOBAL_MESSAGE_KIND_OPEN_IPC_LINK,
  GLOBAL_MESSAGE_KIND_CLOSE_IPC_LINK,
  GLOBAL_MESSAGE_KIND_SEND_IPC_MESSAGE,
  GLOBAL_MESSAGE_KIND_OPEN_THREAD,
  GLOBAL_MESSAGE_KIND_CLOSE_THREAD,
  GLOBAL_MESSAGE_KIND_ATTACH_THREAD,
  GLOBAL_MESSAGE_KIND_DETACH_THREAD,
  GLOBAL_MESSAGE_KIND_REGISTER_AGENT,
  GLOBAL_MESSAGE_KIND_UNREGISTER_AGENT
} qtz_message_kind_t;
typedef enum {
  QTZ_DISCARD,
  QTZ_DEFERRED,
  QTZ_NOTIFY_LISTENER
} qtz_message_action_t;
typedef struct {
  const char*            name;
  qtz_shared_mailbox_t*  pHandleAddr;
} qtz_shared_mailbox_params_t;

struct JEM_page_aligned qtz_deputy{
  // Cacheline 0 - Semaphore for claiming slots [writer]
  JEM_cache_aligned

  jem_semaphore_t slotSemaphore;


  // Cacheline 1 - Index of a free slot [writer]
  JEM_cache_aligned

  atomic_u32_t nextFreeSlot;


  // Cacheline 2 - End of the queue [writer]
  JEM_cache_aligned

  atomic_u32_t lastQueuedSlot;


  // Cacheline 3 - Queued message counter [writer,reader]
  JEM_cache_aligned

  atomic_u32_t queuedSinceLastCheck;


  // Cacheline 4 - Mailbox state [reader]
  JEM_cache_aligned

  jem_u32_t       minQueuedMessages;
  jem_u32_t       maxCurrentDeferred;
  jem_u32_t       releasedSinceLastCheck;
  atomic_flag_t   shouldCloseMailbox;
  qtz_exit_code_t exitCode;


  // Cacheline 5:7 - Memory Management [reader]
  JEM_cache_aligned

  jem_size_t    sharedMemorySize;
  void*         sharedMemoryInitialAddress;
};

struct JEM_cache_aligned qtz_request {
  jem_u32_t                 nextSlot;
  atomic_flag_t             isReady;
  atomic_flag_t             isDiscarded;
  bool                      isForeign;
  jem_u8_t                  kind;
  char                      payload[];

  template <typename P>
  inline P* payload_as() const noexcept {
    return static_cast<P*>(payload);
  }

  void discard() noexcept {
    isDiscarded.test_and_set();
  }
  void notify_listener() noexcept {
    if ( isReady.test_and_set(std::memory_order_acq_rel) )
      isReady.notify_all();
  }
};
struct qtz_mailbox {

  using time_point = std::chrono::high_resolution_clock::time_point;
  using duration   = std::chrono::high_resolution_clock::duration;

  // Cacheline 0 - Semaphore for claiming slots [writer]
  JEM_cache_aligned

  jem_semaphore_t slotSemaphore;


  // Cacheline 1 - Index of a free slot [writer]
  JEM_cache_aligned

  atomic_u32_t nextFreeSlot;


  // Cacheline 2 - End of the queue [writer]
  JEM_cache_aligned

  atomic_u32_t lastQueuedSlot;


  // Cacheline 3 - Queued message counter [writer,reader]
  JEM_cache_aligned

  atomic_u32_t queuedSinceLastCheck;


  // Cacheline 4 - Mailbox state [reader]
  JEM_cache_aligned

  jem_u32_t       minQueuedMessages;
  jem_u32_t       maxCurrentDeferred;
  jem_u32_t       releasedSinceLastCheck;
  atomic_flag_t   shouldCloseMailbox;
  qtz_exit_code_t exitCode;


  // Cacheline 5:7 - Memory Management [reader]
  JEM_cache_aligned

  jem_size_t    sharedMemorySize;
  void*         sharedMemoryInitialAddress;


  // Cacheline 8:10 - Epoch Tracking [reader]


  // Message Slots
  struct qtz_request messageSlots[];


  inline qtz_request_t get_free_request_slot() noexcept {

    jem_u32_t thisSlot = nextFreeSlot.load(std::memory_order_acquire);
    jem_u32_t nextSlot;
    qtz_request_t message;
    do {
      message = messageSlots + thisSlot;
      nextSlot = message->nextSlot;
      // assert(atomic_load(&message->isFree, relaxed));
    } while ( !nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot, std::memory_order_acq_rel) );

    return messageSlots + thisSlot;
  }
  inline qtz_request_t acquire_free_request_slot() noexcept {
    slotSemaphore.acquire();
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_request_slot() noexcept {
    if ( !slotSemaphore.try_acquire() )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_request_slot_for(duration timeout) noexcept {
    if ( !slotSemaphore.try_acquire_for(timeout) )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_request_slot_until(time_point deadline) noexcept {
    if ( !slotSemaphore.try_acquire_until(deadline) )
      return nullptr;
    return get_free_request_slot();
  }
  inline void          release_request_slot(qtz_request_t message) noexcept {
    const jem_u32_t thisSlot = message - messageSlots;
    jem_u32_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = newNextSlot;
    } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, thisSlot) );
    slotSemaphore.release();
  }
  inline void          enqueue_request(qtz_request_t message) noexcept {
    const jem_u32_t slot = message - messageSlots;
    jem_u32_t u32LastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = lastQueuedSlot;
    } while ( !lastQueuedSlot.compare_exchange_weak(u32LastQueuedSlot, slot) );
    queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
    queuedSinceLastCheck.notify_one();
  }
  inline qtz_request_t wait_on_queued_request(qtz_request_t prev) noexcept {
    queuedSinceLastCheck.wait(0, std::memory_order_acquire);
    minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
    return messageSlots + prev->nextSlot;
  }
  inline qtz_request_t acquire_first_queued_request() noexcept {
    queuedSinceLastCheck.wait(0, std::memory_order_acquire);
    minQueuedMessages = queuedSinceLastCheck.exchange(0, std::memory_order_release);
    --minQueuedMessages;
    return messageSlots;
  }
  inline qtz_request_t acquire_next_queued_request(qtz_request_t prev) noexcept {
    qtz_request_t message;
    if ( minQueuedMessages == 0 )
      message = wait_on_queued_request(prev);
    else
      message = messageSlots + prev->nextSlot;
    --minQueuedMessages;
    return message;
  }
  inline void          discard_request(qtz_request_t message) noexcept {
    if ( atomic_flag_test_and_set(&message->isDiscarded) ) {
      release_request_slot(message);
    }
  }

  inline bool          should_close() const noexcept {
    return shouldCloseMailbox.test();
  }



  qtz_message_action_t proc_alloc_pages(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_pages(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_static_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_static_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_dynamic_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_dynamic_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_static_mailbox_shared(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_static_mailbox_shared(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_dynamic_mailbox_shared(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_dynamic_mailbox_shared(qtz_request_t request) noexcept;
  qtz_message_action_t proc_open_ipc_link(qtz_request_t request) noexcept;
  qtz_message_action_t proc_close_ipc_link(qtz_request_t request) noexcept;
  qtz_message_action_t proc_send_ipc_message(qtz_request_t request) noexcept;
  qtz_message_action_t proc_open_deputy(qtz_request_t request) noexcept;
  qtz_message_action_t proc_close_deputy(qtz_request_t request) noexcept;
  qtz_message_action_t proc_attach_thread(qtz_request_t request) noexcept;
  qtz_message_action_t proc_detach_thread(qtz_request_t request) noexcept;
  qtz_message_action_t proc_register_agent(qtz_request_t request) noexcept;
  qtz_message_action_t proc_unregister_agent(qtz_request_t request) noexcept;
};


static atomic_flag_t isInitialized{};



static void*         globalMailboxThreadHandle;
static int           globalMailboxThreadId;

static qtz_mailbox_t globalMailbox;

#define JEM_GLOBAL_MESSAGE_SIZE  64
#define JEM_GLOBAL_MESSAGE_SLOTS ((JEM_VIRTUAL_PAGE_SIZE - sizeof(struct qtz_mailbox)) / JEM_GLOBAL_MESSAGE_SIZE)
#define JEM_GLOBAL_MAILBOX_SIZE  (JEM_GLOBAL_MESSAGE_SIZE * JEM_GLOBAL_MESSAGE_SLOTS)
#define JEM_GLOBAL_MAILBOX_ALIGN (JEM_GLOBAL_MAILBOX_SIZE * 2)

extern "C" {

JEM_stdcall qtz_exit_code_t qtz_mailbox_main_thread_proc(void *params);


JEM_stdcall qtz_request_t   qtz_alloc_static_mailbox(void **pResultAddr, jem_u32_t messageSlots, jem_u32_t messageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_static_mailbox(void *mailboxAddr);
JEM_stdcall qtz_request_t   qtz_alloc_dynamic_mailbox(void **pResultAddr, jem_u32_t minMailboxSize, jem_u32_t maxMessageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_dynamic_mailbox(void *mailboxAddr);

}


#endif//JEMSYS_QUARTZ_INTERNAL_H
