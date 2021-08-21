//
// Created by maxwell on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_INTERNAL_H
#define JEMSYS_QUARTZ_INTERNAL_H

#define JEM_SHARED_LIB_EXPORT

#include <quartz/core.h>


#include <atomic>
#include <semaphore>
#include <span>
#include <array>
#include <ranges>
#include <mutex>
#include <shared_mutex>

#include <intrin.h>



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


#define QTZ_REQUEST_SIZE 512
#define QTZ_REQUEST_PAYLOAD_SIZE QTZ_REQUEST_SIZE - JEM_CACHE_LINE



/*typedef struct jem_global_message{
  jem_u32_t nextIndex;

} jem_global_message, * jem_global_message_t;*/


namespace qtz{

  template <typename T>
  inline T* alloc_array(jem_size_t arraySize, jem_size_t alignment) noexcept {
    return static_cast<T*>( _aligned_malloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
  }
  template <typename T>
  inline T* realloc_array(T* array, jem_size_t newArraySize, jem_size_t oldArraySize, jem_size_t alignment) noexcept {
    return static_cast<T*>(_aligned_realloc(array, newArraySize * sizeof(T), std::max(alignof(T), alignment)));
  }
  template <typename T>
  inline void free_array(T* array, jem_size_t arraySize, jem_size_t alignment) noexcept {
    _aligned_free(array);
  }




  class fixed_size_pool{

    using index_t = jem_size_t;
    using block_t = void**;

    struct slab {
      index_t availableBlocks;
      block_t nextFreeBlock;
      slab**  stackPosition;
    };

    inline constexpr static jem_size_t MinimumBlockSize = sizeof(slab);
    inline constexpr static jem_size_t InitialStackSize = 4;
    inline constexpr static jem_size_t StackGrowthRate  = 2;
    inline constexpr static jem_size_t StackAlignment   = JEM_CACHE_LINE;




    inline slab* alloc_slab() const noexcept {
      return static_cast<slab*>(_aligned_malloc(slabAlignment, slabAlignment));
    }
    inline void free_slab(slab* slab) const noexcept {
      _aligned_free(slab);
    }

    inline block_t lookupBlock(slab* s, jem_size_t blockIndex) const noexcept {
      return reinterpret_cast<block_t>(reinterpret_cast<jem_u8_t*>(s) + (blockIndex * blockSize));
    }
    inline slab*  lookupSlab(void* block) const noexcept {
      return reinterpret_cast<slab*>(reinterpret_cast<uintptr_t>(block) & slabAlignmentMask);
    }



    inline bool isEmpty(const slab* s) const noexcept {
      return s->availableBlocks == blocksPerSlab;
    }
    inline static bool isFull(const slab* s) noexcept {
      return s->availableBlocks == 0;
    }




    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        slab** oldStackBase = slabStackBase;
        jem_size_t oldStackSize = slabStackTop - oldStackBase;
        jem_size_t newStackSize = oldStackSize * StackGrowthRate;
        slab** newStackBase = realloc_array(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackHead = newStackBase + oldStackSize;

          for ( slab** entry = newStackBase; entry != slabStackHead; ++entry )
            (*entry)->stackPosition = entry;
        }

      }

      const auto s = slabStackHead;

      allocSlab = *s;

      *s = alloc_slab();
      (*s)->availableBlocks = static_cast<jem_u32_t>(blocksPerSlab);
      (*s)->nextFreeBlock   = lookupBlock(*s, 1);
      (*s)->stackPosition = s;


      jem_u32_t i = 1;
      while (  i < blocksPerSlab ) {
        block_t block = lookupBlock(*s, i);
        *block = lookupBlock(*s, ++i);
      }

      ++slabStackHead;
    }

    inline void findAllocSlab() noexcept {
      if ( isFull(allocSlab) ) {
        if (fullStackHead == slabStackHead ) [[unlikely]] {
          makeNewSlab();
        }
        else {
          if ( allocSlab != *fullStackHead )
            swapSlabs(allocSlab, *fullStackHead);
          allocSlab = *++fullStackHead;
        }
      }
    }


    inline static void swapSlabs(slab* a, slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }



    inline void removeStackEntry(slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }


    jem_size_t blockSize;
    jem_size_t blocksPerSlab;
    jem_size_t slabAlignment;
    jem_size_t slabAlignmentMask;
    slab**     slabStackBase;
    slab**     slabStackHead;
    slab**     slabStackTop;
    slab**     fullStackHead;
    slab*      allocSlab;
    slab*      freeSlab;

  public:
    fixed_size_pool(jem_size_t blockSize, jem_size_t blocksPerSlab) noexcept
        : blockSize(std::max(blockSize, MinimumBlockSize)),
          blocksPerSlab(blocksPerSlab - 1),
          slabAlignment(std::bit_ceil(this->blockSize * blocksPerSlab)),
          slabAlignmentMask(~(slabAlignment - 1)),
          slabStackBase(alloc_array<slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          allocSlab(),
          freeSlab(){
      makeNewSlab();
    }

    ~fixed_size_pool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      jem_size_t stackSize = slabStackTop - slabStackBase;
      free_array(slabStackBase, stackSize, StackAlignment);
    }

    void* alloc_block() noexcept {

      findAllocSlab();

      block_t block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<block_t>(*block);
      --allocSlab->availableBlocks;

      return block;

    }
    void  free_block(void* block_) noexcept {
      auto* parentSlab = lookupSlab(block_);
      auto  block = static_cast<block_t>(block_);
      *block = parentSlab->nextFreeBlock;
      parentSlab->nextFreeBlock = block;
      ++parentSlab->availableBlocks;

      if ( isEmpty(parentSlab) ) {
        if ( isEmpty(freeSlab) ) [[unlikely]] {
          removeStackEntry(freeSlab);
        }
        freeSlab = parentSlab;
      }
      else if (parentSlab->stackPosition < fullStackHead ) [[unlikely]] {
        --fullStackHead;
        if ( parentSlab->stackPosition != fullStackHead )
          swapSlabs(parentSlab, *fullStackHead);
      }
      allocSlab = parentSlab;
    }
  };
}

#if JEM_system_windows
using qtz_exit_code_t = unsigned long;
using qtz_pid_t = unsigned long;
using qtz_tid_t = unsigned long;
using qtz_handle_t = void*;
#else
using qtz_exit_code_t = int;
using qtz_pid_t       = int;
using qtz_tid_t       = int;
using qtz_qtz_handle_tt    = int;
#endif




typedef enum {
  GLOBAL_MESSAGE_KIND_ALLOC_PAGES,
  GLOBAL_MESSAGE_KIND_FREE_PAGES,
  GLOBAL_MESSAGE_KIND_ALLOCATE_MAILBOX,
  GLOBAL_MESSAGE_KIND_FREE_MAILBOX,
  GLOBAL_MESSAGE_KIND_OPEN_IPC_LINK,
  GLOBAL_MESSAGE_KIND_CLOSE_IPC_LINK,
  GLOBAL_MESSAGE_KIND_SEND_IPC_MESSAGE,
  GLOBAL_MESSAGE_KIND_OPEN_DEPUTY,
  GLOBAL_MESSAGE_KIND_CLOSE_DEPUTY,
  GLOBAL_MESSAGE_KIND_ATTACH_THREAD,
  GLOBAL_MESSAGE_KIND_DETACH_THREAD,
  GLOBAL_MESSAGE_KIND_REGISTER_AGENT,
  GLOBAL_MESSAGE_KIND_UNREGISTER_AGENT
} qtz_message_kind_t;
typedef enum {
  QTZ_ACTION_DISCARD,
  QTZ_ACTION_DEFERRED,
  QTZ_ACTION_NOTIFY_LISTENER
} qtz_message_action_t;




struct qtz_request {
  JEM_cache_aligned
  jem_size_t                nextSlot;
  atomic_flag_t             isReady;
  atomic_flag_t             isDiscarded;
  qtz_message_kind_t        messageKind;
  JEM_cache_aligned
  char                      payload[QTZ_REQUEST_PAYLOAD_SIZE];

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
struct alignas(QTZ_REQUEST_SIZE) qtz_mailbox {

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
  jem_u32_t    totalSlotCount;


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
  alignas(QTZ_REQUEST_SIZE) qtz_request messageSlots[];


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
  qtz_message_action_t proc_alloc_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_mailbox(qtz_request_t request) noexcept;
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



extern qtz_mailbox* g_qtzGlobalMailbox;
extern void*        g_qtzProcessAddressSpace;
extern size_t       g_qtzProcessAddressSpaceSize;


extern qtz_handle_t g_qtzProcessInboxFileMapping;
extern size_t       g_qtzProcessInboxFileMappingBytes;

extern qtz_handle_t g_qtzMailboxThreadHandle;
extern qtz_tid_t    g_qtzMailboxThreadId;


extern qtz_handle_t g_qtzKernelCreationMutex;
extern qtz_handle_t g_qtzKernelBlockMappingHandle;

extern qtz_handle_t g_qtzKernelProcessHandle;
extern qtz_pid_t    g_qtzKernelProcessId;




extern "C" JEM_stdcall qtz_exit_code_t qtz_mailbox_main_thread_proc(void *params);



#endif//JEMSYS_QUARTZ_INTERNAL_H
