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
#include <cstring>
#include <memory>





using atomic_u32_t  = std::atomic_uint32_t;
using atomic_u64_t  = std::atomic_uint64_t;
using atomic_size_t = std::atomic_size_t;
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
#if defined(_WIN32)
    return static_cast<T*>( _aligned_malloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#else
    return static_cast<T*>( std::aligned_alloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#endif

  }
  template <typename T>
  inline T* realloc_array(T* array, jem_size_t newArraySize, jem_size_t oldArraySize, jem_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>(_aligned_realloc(array, newArraySize * sizeof(T), std::max(alignof(T), alignment)));
#else
#endif
  }
  template <typename T>
  inline void free_array(T* array, jem_size_t arraySize, jem_size_t alignment) noexcept {
#if defined(_WIN32)
    _aligned_free(array);
#else
    free(array);
#endif
  }




  class fixed_size_pool{

    using index_t = size_t;
    using block_t = void**;

    struct slab {
      index_t availableBlocks;
      block_t nextFreeBlock;
      slab**  stackPosition;
    };

    inline constexpr static size_t MinimumBlockSize = sizeof(slab);
    inline constexpr static size_t InitialStackSize = 4;
    inline constexpr static size_t StackGrowthRate  = 2;
    inline constexpr static size_t StackAlignment   = JEM_CACHE_LINE;




    inline slab* alloc_slab() const noexcept {
      return static_cast<slab*>(std::aligned_alloc(slabAlignment, slabAlignment));
    }
    inline void free_slab(slab* slab) const noexcept {
      std::free(slab);
    }

    inline block_t lookupBlock(slab* s, size_t blockIndex) const noexcept {
      return reinterpret_cast<block_t>(reinterpret_cast<char*>(s) + (blockIndex * blockSize));
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
        size_t oldStackSize = slabStackTop - oldStackBase;
        size_t newStackSize = oldStackSize * StackGrowthRate;
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


      uint32_t i = 1;
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


    size_t blockSize;
    size_t blocksPerSlab;
    slab** slabStackBase;
    slab** slabStackHead;
    slab** slabStackTop;
    slab** fullStackHead;
    slab*  allocSlab;
    slab*  freeSlab;
    size_t slabAlignment;
    size_t slabAlignmentMask;

  public:
    fixed_size_pool(size_t blockSize, size_t blocksPerSlab) noexcept
        : blockSize(std::max(blockSize, MinimumBlockSize)),
          blocksPerSlab(blocksPerSlab - 1),
          slabStackBase(alloc_array<slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          allocSlab(),
          freeSlab(),
          slabAlignment(std::bit_ceil(this->blockSize * blocksPerSlab)),
          slabAlignmentMask(~(slabAlignment - 1)) {
      makeNewSlab();
    }

    ~fixed_size_pool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      size_t stackSize = slabStackTop - slabStackBase;
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
using qtz_handle_t    = int;
#endif


enum {
  QTZ_MESSAGE_IS_READY      = 0x1,
  QTZ_MESSAGE_IS_DISCARDED  = 0x2,
  QTZ_MESSAGE_NOT_PRESERVED = 0x4,
  QTZ_MESSAGE_IS_FINALIZED  = 0x8,
  QTZ_MESSAGE_ALL_FLAGS     = 0xF,
  QTZ_MESSAGE_PRESERVE       = (QTZ_MESSAGE_ALL_FLAGS ^ QTZ_MESSAGE_NOT_PRESERVED)
};

typedef enum {
  GLOBAL_MESSAGE_KIND_NOOP,
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

namespace qtz {
  struct alloc_pages_request{

  };
  struct free_pages_request{

  };
  struct alloc_mailbox_request{

  };
  struct free_mailbox_request{

  };

}



struct qtz_request {

  inline constexpr static jem_u32_t DiscardedDeathFlags = QTZ_MESSAGE_NOT_PRESERVED | QTZ_MESSAGE_IS_DISCARDED;
  inline constexpr static jem_u32_t FinalizedDeathFlags = QTZ_MESSAGE_NOT_PRESERVED | QTZ_MESSAGE_IS_FINALIZED;
  inline constexpr static jem_u32_t FinalizedAndDiscardedFlags = DiscardedDeathFlags ^ FinalizedDeathFlags;

  JEM_cache_aligned
  jem_size_t           nextSlot;
  jem_size_t           thisSlot;

  qtz_message_kind_t   messageKind;
  jem_u32_t            queuePriority;

  /*atomic_flag_t       isReady;
  atomic_flag_t       isDiscarded;
*/
  atomic_u32_t         flags;
  qtz_message_action_t action;
  bool                 readyToDie;

  // 32 free bytes


  JEM_cache_aligned
  char                      payload[QTZ_REQUEST_PAYLOAD_SIZE];



  void init(jem_size_t slot) noexcept {
    nextSlot = slot + 1;
    thisSlot = slot;
    flags.store(QTZ_MESSAGE_NOT_PRESERVED);
  }


  template <typename P>
  inline P* payload_as() const noexcept {
    return static_cast<P*>(payload);
  }



  bool is_discarded() const noexcept {
    return flags.load() & QTZ_MESSAGE_IS_DISCARDED;
  }
  bool is_ready_to_die() const noexcept {
    return readyToDie;
  }

  void finalize() noexcept {
    jem_u32_t oldFlags = flags.fetch_or(QTZ_MESSAGE_IS_FINALIZED);
    readyToDie = (oldFlags & DiscardedDeathFlags) == DiscardedDeathFlags;
  }
  void discard() noexcept {
    readyToDie = (flags.fetch_or(QTZ_MESSAGE_IS_DISCARDED) & FinalizedDeathFlags) == FinalizedDeathFlags;
  }
  void finalize_and_return() noexcept {
    jem_u32_t oldFlags = flags.fetch_or(QTZ_MESSAGE_IS_READY | QTZ_MESSAGE_IS_FINALIZED);
    readyToDie = (oldFlags & DiscardedDeathFlags) == DiscardedDeathFlags;
    if ( !(oldFlags & QTZ_MESSAGE_IS_DISCARDED) )
      flags.notify_all();
  }
  void lock() noexcept {
    flags.fetch_and(QTZ_MESSAGE_PRESERVE);
  }
  void unlock() noexcept {
    jem_u32_t oldFlags = flags.fetch_or(QTZ_MESSAGE_NOT_PRESERVED);
    readyToDie = (oldFlags & FinalizedAndDiscardedFlags) == FinalizedAndDiscardedFlags;
  }

};

class request_priority_queue{

  jem_size_t     indexMask;
  jem_size_t     queueHeadIndex;
  jem_size_t     queueSize;
  qtz_request_t* entryArray;

  JEM_forceinline static jem_size_t parent(jem_size_t entry) noexcept {
    return entry >> 1;
  }
  JEM_forceinline static jem_size_t left(jem_size_t entry) noexcept {
    return entry << 1;
  }
  JEM_forceinline static jem_size_t right(jem_size_t entry) noexcept {
    return (entry << 1) + 1;
  }

  JEM_forceinline qtz_request_t& lookup(jem_size_t i) const noexcept {
    return const_cast<qtz_request_t&>(entryArray[indexMask & (queueHeadIndex + i)]);
  }

  JEM_forceinline bool is_less_than(jem_size_t a, jem_size_t b) const noexcept {
    return lookup(a)->queuePriority < lookup(b)->queuePriority;
  }
  JEM_forceinline void exchange(jem_size_t a, jem_size_t b) noexcept {
    std::swap(lookup(a), lookup(b));
  }


  inline void heapify(jem_size_t i) noexcept {
    jem_size_t l = left(i);
    jem_size_t r = right(i);
    jem_size_t smallest;

    if ( l > queueSize )
      return;

    qtz_request_t& i_ = lookup(i);
    qtz_request_t& l_ = lookup(l);

    if ( r > queueSize ) {
      if ( l_->queuePriority < i_->queuePriority )
        std::swap(l_, i_);
      return;
    }

    qtz_request_t& r_ = lookup(r);

    qtz_request_t& s_ = l_->queuePriority < r_->queuePriority ? l_ : r_;


    if ( s_->queuePriority < i_->queuePriority ) {
      jem_size_t s = l_->queuePriority < r_->queuePriority ? l : r;
      std::swap(s_, i_);
      heapify(s);
    }


    /*smallest = (l <= queueSize && is_less_than(l, i)) ? l : i;
    if ( r <= queueSize && is_less_than(r, smallest))
      smallest = r;

    if ( smallest != i ) {
      heapify(smallest);
    }*/
  }

  inline void build_heap() noexcept {
    for ( jem_size_t i = parent(queueSize); i > 0; --i )
      heapify(i);
  }

  inline void append_n(const qtz_request_t* requests, jem_size_t request_count) noexcept {

    /**
     *     ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
     *    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
     *    |   |   |   |   |   |   |   |   |   | 2 | 3 | 5 | 7 | 4 |   |   |
     *    |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
     *      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     *
     *  - queueHeadIndex = 8
     *  - queueSize      = 5
     *  - indexMask      = 0xF
     *
     *
     *     ___ ___ ___ ___ ___
     *    |   |   |   |   |   |
     *    | 7 | 1 | 2 | 6 | 1 |
     *    |___|___|___|___|___|
     *
     *  - request_count = 5
     *
     *  ::nextEndIndex       => 18
     *  ::actualNextEndIndex => 2
     *
     *
     *
     *     ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
     *    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
     *    | 2 | 6 | 1 |   |   |   |   |   |   | 2 | 3 | 5 | 7 | 4 | 7 | 1 |
     *    |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
     *      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     *
     *
     *  - queueHeadIndex = 8
     *  - queueSize      = 10
     *  - indexMask      = 0xF
     *
     * */

    size_t nextEndIndex = queueHeadIndex + queueSize + request_count;
    size_t actualNextEndIndex = nextEndIndex & indexMask;

    if ( actualNextEndIndex == nextEndIndex || actualNextEndIndex == 0 ) [[likely]] {
      std::memcpy(&lookup(queueSize + 1), requests, request_count * sizeof(void*));
    }
    else {
      size_t distanceUntilEnd = nextEndIndex - actualNextEndIndex;
      std::memcpy(&lookup(queueSize + 1), requests, distanceUntilEnd * request_count);
      std::memcpy(entryArray, requests + distanceUntilEnd, actualNextEndIndex * sizeof(void*));
    }
    queueSize += request_count;
  }

  inline void rebalance_after_pop() noexcept {


    /*
     * Example:
     *
     *                                     A
     *                                   ⟋   ⟍
     *                                 ⟋       ⟍
     *                               ⟋           ⟍
     *                             ⟋               ⟍
     *                           ⟋                   ⟍
     *                         ⟋                       ⟍
     *                       ⟋                           ⟍
     *                     B                                C
     *                   ⟋   ⟍                           ⟋   ⟍
     *                 ⟋       ⟍                       ⟋       ⟍
     *               ⟋           ⟍                   ⟋           ⟍
     *             D               E               F                G
     *            / \             / \             / \              / \
     *           /   \           /   \           /   \            /   \
     *          /     \         /     \         /     \          /     \
     *         H       I       J       K       L       M        N       O
     *        / \     / \     / \     / \     / \     /
     *       /   \   /   \   /   \   /   \   /   \   /
     *      P     Q R     S T     U V     W X     Y Z
     *
     *
     *      Order:
     *
     *      N, G
     *      P, H, D, B, A
     *      R, I
     *      T, J, E
     *      V, K
     *      X, L, F, C
     *      Z, M
     *
     * */



    jem_size_t i = parent(queueSize);
    i |= 0x1;
    ++i;

    assert( i % 2 == 0);
    assert( i * 2 > queueSize);

    for ( ; i <= queueSize; i += 2) {

      // for each even indexed node in the second half of the array,
      // ie. for every left leaf node,

      size_t j = i;
      do {

        // for every parent node until one is a right child (odd indexed)

        size_t node  = j;
        qtz_request_t* p = &lookup(parent(node));
        qtz_request_t* c;

        do {

          // bubble sort lol

          c = &lookup(node);

          if ( (*p)->queuePriority < (*c)->queuePriority )
            break;

          std::swap(*p, *c);

          p = c;
          node *= 2;

        } while (node <= i);

        j = parent(j);

      } while ( !(j & 0x1) );

    }
  }
  inline void rebalance_after_insert() noexcept {

    qtz_request_t* node = &lookup(queueSize);

    for ( jem_size_t i = parent(queueSize); i > 0; i = parent(i)) {
      qtz_request_t* papi = &lookup(i);
      if ( (*node)->queuePriority >= (*papi)->queuePriority )
        break;
      std::swap(*node, *papi);
      node = papi;
    }

  }

  bool is_valid_heap_subtree(jem_size_t i) const noexcept {

    if ( i * 2 > queueSize )
      return true;


    jem_size_t    l = left(i);
    jem_size_t    r = right(i);
    qtz_request_t node = lookup(i);
    qtz_request_t l_node = lookup(l);
    qtz_request_t r_node = lookup(r);

    bool result = node->queuePriority <= l_node->queuePriority;

    if ( l == queueSize )
      return result;

    return result &&
           node->queuePriority <= r_node->queuePriority &&
           is_valid_heap_subtree(l) &&
           is_valid_heap_subtree(r);
  }

public:

  explicit request_priority_queue(jem_size_t maxSize) noexcept {
    jem_size_t actualArraySize = std::bit_ceil(maxSize);
    indexMask = actualArraySize - 1;
    queueHeadIndex = 0;
    queueSize = 0;
    entryArray = new qtz_request_t[actualArraySize];
  }

  ~request_priority_queue() {
    delete[] entryArray;
  }



  qtz_request_t peek() const noexcept {
    assert( not empty() );
    return lookup(1);
  }
  qtz_request_t pop() noexcept {
    auto req = peek();
    queueHeadIndex = (queueHeadIndex + 1) & indexMask;
    queueSize -= 1;
    rebalance_after_pop();
    return req;
  }

  void insert(qtz_request_t request) noexcept {
    lookup(++queueSize) = request;
    rebalance_after_insert();
  }
  void insert_n(const qtz_request_t* requests, jem_size_t request_count) noexcept {
    switch (request_count) {
      case 0:
        return;
      case 1:
        insert(*requests);
        return;
      default:
        append_n(requests, request_count);
        build_heap();
    }
  }

  jem_size_t size() const noexcept {
    return queueSize;
  }
  bool empty() const noexcept {
    return !queueSize;
  }

  bool is_valid_heap() const noexcept {
    return is_valid_heap_subtree(1);
  }
};

struct alignas(QTZ_REQUEST_SIZE) qtz_mailbox {

  using time_point = std::chrono::high_resolution_clock::time_point;
  using duration   = std::chrono::high_resolution_clock::duration;

  // Cacheline 0 - Semaphore for claiming slots [writer]
  JEM_cache_aligned

  jem_semaphore_t slotSemaphore;

  // 48 free bytes


  // Cacheline 1 - Index of a free slot [writer]
  JEM_cache_aligned

  atomic_size_t nextFreeSlot;

  // 56 free bytes


  // Cacheline 2 - End of the queue [writer]
  JEM_cache_aligned

  atomic_size_t lastQueuedSlot;

  // 56 free bytes


  // Cacheline 3 - Queued message counter [writer,reader]
  JEM_cache_aligned

  atomic_u32_t queuedSinceLastCheck;
  jem_u32_t    totalSlotCount;

  // 56 free bytes


  // Cacheline 4 - Mailbox state [reader]
  JEM_cache_aligned

  qtz_request_t   previousRequest;
  jem_u32_t       minQueuedMessages;
  jem_u32_t       maxCurrentDeferred;
  jem_u32_t       releasedSinceLastCheck;
  atomic_flag_t   shouldCloseMailbox;
  qtz_exit_code_t exitCode;

  // 20 free bytes


  // Cacheline 5 - Memory Management [reader]
  JEM_cache_aligned

  qtz::fixed_size_pool   mailboxPool;
  request_priority_queue messagePriorityQueue;


  // 48 free bytes


  JEM_cache_aligned

  char fileMappingName[JEM_CACHE_LINE];

  // Cacheline 8:10 - Epoch Tracking [reader]


  // Message Slots
  alignas(QTZ_REQUEST_SIZE) qtz_request messageSlots[];




  qtz_mailbox(jem_size_t slotCount, jem_size_t mailboxSize, const char fileMappingName[JEM_CACHE_LINE])
      : slotSemaphore(static_cast<jem_ptrdiff_t>(slotCount) - 1),
        nextFreeSlot(0),
        lastQueuedSlot(0),
        queuedSinceLastCheck(0),
        totalSlotCount(static_cast<jem_u32_t>(slotCount)),
        previousRequest(nullptr),
        minQueuedMessages(0),
        maxCurrentDeferred(0),
        releasedSinceLastCheck(0),
        shouldCloseMailbox(),
        exitCode(0),
        mailboxPool(mailboxSize, 255),
        messagePriorityQueue(slotCount),
        fileMappingName()
  {
    std::memcpy(this->fileMappingName, std::assume_aligned<JEM_CACHE_LINE>(fileMappingName), JEM_CACHE_LINE);

    for ( jem_size_t i = 0; i < slotCount; ++i )
      messageSlots[i].init(i);
  }


  inline qtz_request_t     get_request(jem_size_t slot) const noexcept {
    assert(slot < totalSlotCount);
    return const_cast<qtz_request_t>(messageSlots + slot);
  }
  inline static jem_size_t get_slot(qtz_request_t request) noexcept {
    return request->thisSlot;
  }

  inline qtz_request_t get_free_request_slot() noexcept {

    jem_size_t thisSlot = nextFreeSlot.load(std::memory_order_acquire);
    jem_size_t nextSlot;
    qtz_request_t message;
    do {
      message  = get_request(thisSlot);
      nextSlot = message->nextSlot;
      // assert(atomic_load(&message->isFree, relaxed));
    } while ( !nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot) );

    return message;
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
    const jem_size_t thisSlot = get_slot(message);
    jem_size_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = newNextSlot;
    } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, thisSlot) );
    slotSemaphore.release();
  }

  inline qtz_request_t acquire_first_queued_request() const noexcept {
    return get_request(0);
  }
  inline qtz_request_t acquire_next_queued_request() noexcept {
    queuedSinceLastCheck.wait(0, std::memory_order_acquire);
    qtz_request_t message = get_request(previousRequest->nextSlot);

    auto minQueued = queuedSinceLastCheck.exchange(0);
    if ( minQueued == 1 ) {
      replace_previous(message);
      if ( messagePriorityQueue.empty() )
        return message;
      messagePriorityQueue.insert(message);
    }
    else {
      for (auto n = minQueued - 1; n > 0; --n) {
        message = get_request(message->nextSlot);
        messagePriorityQueue.insert(message);
      }
      replace_previous(message);
    }
    return messagePriorityQueue.pop();
  }


  inline void          enqueue_request(qtz_request_t message) noexcept {
    const jem_size_t slot = get_slot(message);
    jem_size_t prevLastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = lastQueuedSlot;
    } while ( !lastQueuedSlot.compare_exchange_weak(prevLastQueuedSlot, slot) );
    queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
    queuedSinceLastCheck.notify_one();
  }
  inline void          discard_request(qtz_request_t message) noexcept {
    message->discard();
    if ( message->is_ready_to_die() )
      release_request_slot(message);
  }

  inline bool          should_close() const noexcept {
    return shouldCloseMailbox.test();
  }

  inline void          replace_previous(qtz_request_t message) noexcept {
    message->lock();
    previousRequest->unlock();
    if ( previousRequest->is_ready_to_die() )
      release_request_slot(previousRequest);
    previousRequest = message;
  }


  void process_next_message() noexcept {

    using PFN_request_proc = qtz_message_action_t(qtz_mailbox::*)(qtz_request_t) noexcept;

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
    };

    auto message = acquire_next_queued_request();

    if ( message->is_discarded() )
      return;

    switch ( (this->*(global_dispatch_table[message->messageKind]))(message) ) {
      case QTZ_ACTION_DISCARD:
        message->discard();
        break;
      case QTZ_ACTION_NOTIFY_LISTENER:
        message->finalize_and_return();
        break;
      case QTZ_ACTION_DEFERRED:
        message->finalize();
        break;
        JEM_no_default;
    }
  }


  qtz_message_action_t proc_noop(qtz_request_t request) noexcept;
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


#if defined(_WIN32)
#define QTZ_NULL_HANDLE nullptr
#else
#define QTZ_NULL_HANDLE 0
#endif

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




extern "C" qtz_exit_code_t JEM_stdcall qtz_mailbox_main_thread_proc(void *params);



#endif//JEMSYS_QUARTZ_INTERNAL_H
