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
#include <cstdlib>


#include "atomicutils.hpp"
#include "dictionary.hpp"
#include "handles.hpp"





/*using atomic_u32_t  = std::atomic_uint32_t;
using atomic_u64_t  = std::atomic_uint64_t;
using atomic_size_t = std::atomic_size_t;
using atomic_flag_t = std::atomic_flag;

using jem_semaphore_t        = std::counting_semaphore<>;
using jem_binary_semaphore_t = std::binary_semaphore;*/

typedef jem_u64_t jem_bitmap_field_t;


#define JEM_BITS_PER_BITMAP_FIELD (sizeof(jem_bitmap_field_t) * 8)
#define JEM_VIRTUAL_REGION_SIZE (JEM_VIRTUAL_PAGE_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_VIRTUAL_SEGMENT_SIZE (JEM_VIRTUAL_REGION_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_GLOBAL_VIRTUAL_MEMORY_MAX (JEM_VIRTUAL_SEGMENT_SIZE * JEM_BITS_PER_BITMAP_FIELD) // 16GB


#define QTZ_REQUEST_SIZE 512
#define QTZ_REQUEST_PAYLOAD_SIZE QTZ_REQUEST_SIZE - JEM_CACHE_LINE

#define QTZ_NAME_MAX_LENGTH (2*JEM_CACHE_LINE)

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



/*typedef struct jem_global_message{
  jem_u32_t nextIndex;

} jem_global_message, * jem_global_message_t;*/


namespace qtz{




  struct handle_descriptor;

  using handle_info_t = typename jem::dictionary<handle_descriptor>::entry_type*;

  // TODO: Expand utility of handle_descriptor
  // TODO: Create handle ownership model?
  // TODO: Create handle lifetime model

  /*
   * Every handle has an owner
   * There exists a root handle
   * All handles created without explicit owner are owned by the root handle
   * A handle X can "require" a handle Y
   * A handle X can "borrow" a handle Y
   *
   * A handle that enters an invalid state will notify every handle that has
   * borrowed or required it.
   *
   *
   * */
  struct handle_descriptor{
    void* address;

    bool  isShared;

  };


  /*class memory_placeholder{
    void*  m_address;
    size_t m_size;
  public:
    memory_placeholder();
    memory_placeholder(size_t size) noexcept;
    memory_placeholder(void* addr, size_t size) noexcept : m_address(addr), m_size(size) { }
    ~memory_placeholder();

    inline size_t size() const noexcept {
      return m_size;
    }

    inline bool is_valid() const noexcept {
      return m_address != nullptr;
    }
    inline explicit operator bool() const noexcept {
      return is_valid();
    }

    memory_placeholder split() noexcept;
    memory_placeholder split(size_t size) noexcept;

    bool               coalesce() noexcept;
    bool               coalesce(memory_placeholder& other) noexcept;

    void*              commit() noexcept;
    void*              map(qtz_handle_t handle) noexcept;

    static memory_placeholder release(void* addr, size_t size) noexcept;
    static memory_placeholder unmap(void* addr, size_t size) noexcept;
  };

  class virtual_allocator{

    void*      addressSpaceInitial;
    jem_size_t addressSpaceSize;

  protected:
    inline void*      addr() const noexcept {
      return addressSpaceInitial;
    }
    inline jem_size_t size() const noexcept {
      return addressSpaceSize;
    }

  public:

    virtual ~virtual_allocator();

    virtual void* reserve(jem_size_t size) noexcept = 0;
    virtual void* reserve(jem_size_t size, jem_size_t alignment, jem_size_t offset) noexcept = 0;



    virtual void  release(void* address, jem_size_t size) noexcept = 0;

  };*/
}




enum {
  QTZ_MESSAGE_IS_READY      = 0x1,
  QTZ_MESSAGE_IS_DISCARDED  = 0x2,
  QTZ_MESSAGE_NOT_PRESERVED = 0x4,
  QTZ_MESSAGE_IS_FINALIZED  = 0x8,
  QTZ_MESSAGE_ALL_FLAGS     = 0xF,
  QTZ_MESSAGE_PRESERVE      = (QTZ_MESSAGE_ALL_FLAGS ^ QTZ_MESSAGE_NOT_PRESERVED)
};

typedef enum {
  GLOBAL_MESSAGE_KIND_NOOP                         = 0,
  GLOBAL_MESSAGE_KIND_1                            = 1,
  GLOBAL_MESSAGE_KIND_2                            = 2,
  GLOBAL_MESSAGE_KIND_ALLOCATE_MAILBOX             = 3,
  GLOBAL_MESSAGE_KIND_FREE_MAILBOX                 = 4,
  GLOBAL_MESSAGE_KIND_3                            = 5,
  GLOBAL_MESSAGE_KIND_4                            = 6,
  GLOBAL_MESSAGE_KIND_5                            = 7,
  GLOBAL_MESSAGE_KIND_6                            = 8,
  GLOBAL_MESSAGE_KIND_7                            = 9,
  GLOBAL_MESSAGE_KIND_8                            = 10,
  GLOBAL_MESSAGE_KIND_9                            = 11,
  GLOBAL_MESSAGE_KIND_10                           = 12,
  GLOBAL_MESSAGE_KIND_11                           = 13,
  GLOBAL_MESSAGE_KIND_REGISTER_OBJECT              = 14,
  GLOBAL_MESSAGE_KIND_UNREGISTER_OBJECT            = 15,
  GLOBAL_MESSAGE_KIND_LINK_OBJECTS                 = 16,
  GLOBAL_MESSAGE_KIND_UNLINK_OBJECTS               = 17,
  GLOBAL_MESSAGE_KIND_OPEN_OBJECT_HANDLE           = 18,
  GLOBAL_MESSAGE_KIND_DESTROY_OBJECT               = 19,
  GLOBAL_MESSAGE_KIND_LOG_MESSAGE                  = 20,
  GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK             = 21,
  GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK_WITH_BUFFER = 22
} qtz_message_kind_t;
typedef enum {
  QTZ_ACTION_DISCARD,
  QTZ_ACTION_DEFERRED,
  QTZ_ACTION_NOTIFY_LISTENER
} qtz_message_action_t;

namespace qtz {
  struct pause_request {
    size_t structSize;
  };
  struct alloc_mailbox_request{
    size_t structSize;
    void** handle;
    char   name[QTZ_NAME_MAX_LENGTH];
    bool   isShared;
  };
  struct free_mailbox_request{
    size_t structSize;
    void* handle;
    char  name[QTZ_NAME_MAX_LENGTH];
  };
  struct log_message_request {
    size_t    structLength;
    // jem_u64_t timestamp;
    char      message[];
  };
  struct execute_callback_request{
    size_t structLength;
    void(* callback)(void*);
    void*  userData;
  };
  struct execute_callback_with_buffer_request{
    size_t structLength;
    void(*callback)(void*);
    char  userData[];
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

  atomic_flags32_t     flags = 0;
  qtz_message_action_t action;
  qtz_status_t         status;

  void*                senderObject;

  bool                 readyToDie;
  bool                 isRealMessage;
  bool                 fromForeignProcess;

  // 27 free bytes


  JEM_cache_aligned
  char                      payload[QTZ_REQUEST_PAYLOAD_SIZE];



  void init(jem_size_t slot) noexcept {
    nextSlot = slot + 1;
    thisSlot = slot;
    flags.set(QTZ_MESSAGE_NOT_PRESERVED);
    fromForeignProcess = false;
    readyToDie         = false;
    isRealMessage      = true;
  }


  template <typename P>
  inline P* payload_as() const noexcept {
    return (P*)payload;
  }


  bool is_real_message() const noexcept {
    return isRealMessage;
  }

  bool is_discarded() const noexcept {
    return flags.test(QTZ_MESSAGE_IS_DISCARDED);
  }
  bool is_ready_to_die() const noexcept {
    return readyToDie;
  }

  void finalize() noexcept {
    readyToDie = (flags.fetch_and_set(QTZ_MESSAGE_IS_FINALIZED) & DiscardedDeathFlags) == DiscardedDeathFlags;
  }
  void discard() noexcept {
    readyToDie = (flags.fetch_and_set(QTZ_MESSAGE_IS_DISCARDED) & FinalizedDeathFlags) == FinalizedDeathFlags;
  }
  void finalize_and_return() noexcept {
    jem_u32_t oldFlags = flags.fetch_and_set(QTZ_MESSAGE_IS_READY | QTZ_MESSAGE_IS_FINALIZED);
    readyToDie = (oldFlags & DiscardedDeathFlags) == DiscardedDeathFlags;
    if ( !(oldFlags & QTZ_MESSAGE_IS_DISCARDED) )
      flags.notify_all();
  }
  void lock() noexcept {
    flags.reset(QTZ_MESSAGE_NOT_PRESERVED);
  }
  void unlock() noexcept {
    jem_u32_t oldFlags = flags.fetch_and_set(QTZ_MESSAGE_NOT_PRESERVED);
    readyToDie = (oldFlags & FinalizedAndDiscardedFlags) == FinalizedAndDiscardedFlags;
  }

  struct qtz_mailbox* parent_mailbox() const noexcept;
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

  semaphore_t slotSemaphore;


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

  // atomic_u32_t queuedSinceLastCheck;
  mpsc_counter_t queuedMessages;
  jem_u8_t       uuid[16];
  jem_u32_t      totalSlotCount;

  std::chrono::high_resolution_clock::time_point startTime;

  // 40 free bytes


  // Cacheline 4 - Mailbox state [reader]
  JEM_cache_aligned

  qtz_request_t    previousRequest;
  jem_u32_t        maxCurrentDeferred;
  jem_u32_t        releasedSinceLastCheck;
  std::atomic_flag shouldCloseMailbox;
  qtz_exit_code_t  exitCode;

  // 20 free bytes


  // Cacheline 5 - Memory Management [reader]
  JEM_cache_aligned

  jem::fixed_size_pool   mailboxPool;
  request_priority_queue messagePriorityQueue;

  jem::dictionary<qtz::handle_descriptor> namedHandles;

  jem_u32_t defaultPriority;


  // 48 free bytes


  JEM_cache_aligned

  char fileMappingName[JEM_CACHE_LINE];


  // Message Slots
  alignas(QTZ_REQUEST_SIZE) qtz_request messageSlots[];




  qtz_mailbox(jem_size_t slotCount, jem_size_t mailboxSize, const char fileMappingName[JEM_CACHE_LINE])
      : slotSemaphore(static_cast<jem_ptrdiff_t>(slotCount)),
        nextFreeSlot(0),
        lastQueuedSlot(0),
        totalSlotCount(static_cast<jem_u32_t>(slotCount)),
        queuedMessages(),
        uuid(),
        previousRequest(nullptr),
        maxCurrentDeferred(0),
        releasedSinceLastCheck(0),
        shouldCloseMailbox(),
        exitCode(0),
        mailboxPool(mailboxSize, 255),
        messagePriorityQueue(slotCount),
        namedHandles(),
        defaultPriority(100),
        fileMappingName()
  {
    std::memcpy(this->fileMappingName, std::assume_aligned<JEM_CACHE_LINE>(fileMappingName), JEM_CACHE_LINE);

    for ( jem_size_t i = 0; i < slotCount - 1; ++i )
      messageSlots[i].init(i);
  }

private:
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
  inline qtz_request_t get_next_queued_request() const noexcept {
    if ( previousRequest ) [[likely]]
      return get_request(previousRequest->nextSlot);
    return get_request(0);
  }
  inline void          release_request_slot(qtz_request_t message) noexcept {
    const jem_size_t thisSlot = get_slot(message);
    jem_size_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = newNextSlot;
    } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, thisSlot) );
    slotSemaphore.release();
  }
  inline void          replace_previous(qtz_request_t message) noexcept {
    message->lock();
    if ( previousRequest ) [[likely]] {
      previousRequest->unlock();
      if ( previousRequest->is_ready_to_die() )
        release_request_slot(previousRequest);
    }
    previousRequest = message;
  }

public:




  inline qtz_request_t acquire_free_slot() noexcept {
    slotSemaphore.acquire();
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot() noexcept {
    if ( !slotSemaphore.try_acquire() )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot_for(jem_u64_t timeout_us) noexcept {
    if ( !slotSemaphore.try_acquire_for(timeout_us) )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot_until(deadline_t deadline) noexcept {
    if ( !slotSemaphore.try_acquire_until(deadline) )
      return nullptr;
    return get_free_request_slot();
  }


  inline qtz_request_t acquire_next_queued_request() noexcept {

    queuedMessages.decrease(1);
    auto nextRequest = get_next_queued_request();
    replace_previous(nextRequest);
    return nextRequest;


    /*if ( messagePriorityQueue.empty() ) {


      qtz_request_t message = get_next_queued_request();

      auto minQueued = queuedSinceLastCheck.exchange(0);
      assert(minQueued != 0);
      if ( minQueued == 1 ) {
        replace_previous(message);
        return message;
      }
      else {
        messagePriorityQueue.insert(message);
        for (auto n = minQueued - 1; n > 0; --n) {
          message = get_request(message->nextSlot);
          messagePriorityQueue.insert(message);
        }
        replace_previous(message);
      }
    }
    else {

      auto minQueued = queuedSinceLastCheck.exchange(0);

      if ( minQueued > 0 ) {
        qtz_request_t message = get_next_queued_request();
        messagePriorityQueue.insert(message);
        for (auto n = minQueued - 1; n > 0; --n) {
          message = get_request(message->nextSlot);
          messagePriorityQueue.insert(message);
        }
        replace_previous(message);
      }
    }


    return messagePriorityQueue.pop();*/
  }


  inline void          enqueue_request(qtz_request_t message) noexcept {
    const jem_size_t slot = get_slot(message);
    jem_size_t prevLastQueuedSlot = lastQueuedSlot.exchange(slot);
    /*jem_size_t prevLastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
    do {
      //message->nextSlot = lastQueuedSlot;
    } while ( !lastQueuedSlot.compare_exchange_weak(prevLastQueuedSlot, slot) );*/
    get_request(prevLastQueuedSlot)->nextSlot = slot;
    queuedMessages.increase(1);
    /*queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
    queuedSinceLastCheck.notify_one();*/
  }
  inline void          discard_request(qtz_request_t message) noexcept {
    message->discard();
    if ( message->is_ready_to_die() )
      release_request_slot(message);
  }

  inline bool          should_close() const noexcept {
    return shouldCloseMailbox.test();
  }




  void process_next_message() noexcept {

    using PFN_request_proc = qtz_message_action_t(qtz_mailbox::*)(qtz_request_t) noexcept;

    constexpr static PFN_request_proc global_dispatch_table[] = {
      &qtz_mailbox::proc_noop,
      &qtz_mailbox::proc_placeholder1,
      &qtz_mailbox::proc_placeholder2,
      &qtz_mailbox::proc_alloc_mailbox,
      &qtz_mailbox::proc_free_mailbox,
      &qtz_mailbox::proc_placeholder3,
      &qtz_mailbox::proc_placeholder4,
      &qtz_mailbox::proc_placeholder5,
      &qtz_mailbox::proc_placeholder6,
      &qtz_mailbox::proc_placeholder7,
      &qtz_mailbox::proc_placeholder8,
      &qtz_mailbox::proc_placeholder9,
      &qtz_mailbox::proc_placeholder10,
      &qtz_mailbox::proc_placeholder11,
      &qtz_mailbox::proc_register_object,
      &qtz_mailbox::proc_unregister_object,
      &qtz_mailbox::proc_link_objects,
      &qtz_mailbox::proc_unlink_objects,
      &qtz_mailbox::proc_open_object_handle,
      &qtz_mailbox::proc_destroy_object,
      &qtz_mailbox::proc_log_message,
      &qtz_mailbox::proc_execute_callback,
      &qtz_mailbox::proc_execute_callback_with_buffer
    };

    auto message = acquire_next_queued_request();

    if ( message->is_discarded() )
      return;

    switch ( (this->*(global_dispatch_table[message->messageKind]))(message) ) {
      case QTZ_ACTION_DISCARD:
        // FIXME: Messages are not being properly discarded :(
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


  /*
   * GLOBAL_MESSAGE_KIND_NOOP,
GLOBAL_MESSAGE_KIND_1,
GLOBAL_MESSAGE_KIND_2,
GLOBAL_MESSAGE_KIND_ALLOCATE_MAILBOX,
GLOBAL_MESSAGE_KIND_FREE_MAILBOX,
GLOBAL_MESSAGE_KIND_3,
GLOBAL_MESSAGE_KIND_4,
GLOBAL_MESSAGE_KIND_5,
GLOBAL_MESSAGE_KIND_6,
GLOBAL_MESSAGE_KIND_7,
GLOBAL_MESSAGE_KIND_8,
GLOBAL_MESSAGE_KIND_9,
GLOBAL_MESSAGE_KIND_10,
GLOBAL_MESSAGE_KIND_11,
GLOBAL_MESSAGE_KIND_REGISTER_OBJECT,
GLOBAL_MESSAGE_KIND_UNREGISTER_OBJECT,
GLOBAL_MESSAGE_KIND_LINK_OBJECTS,
GLOBAL_MESSAGE_KIND_UNLINK_OBJECTS,
GLOBAL_MESSAGE_KIND_OPEN_OBJECT_HANDLE,
GLOBAL_MESSAGE_KIND_DESTROY_OBJECT,
GLOBAL_MESSAGE_KIND_LOG_MESSAGE,
GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK,
GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK_WITH_BUFFER
   *
   * */


  qtz_message_action_t proc_noop(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder1(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder2(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder3(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder4(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder5(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder6(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder7(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder8(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder9(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder10(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder11(qtz_request_t request) noexcept;
  qtz_message_action_t proc_register_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_unregister_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_link_objects(qtz_request_t request) noexcept;
  qtz_message_action_t proc_unlink_objects(qtz_request_t request) noexcept;
  qtz_message_action_t proc_open_object_handle(qtz_request_t request) noexcept;
  qtz_message_action_t proc_destroy_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_log_message(qtz_request_t request) noexcept;
  qtz_message_action_t proc_execute_callback(qtz_request_t request) noexcept;
  qtz_message_action_t proc_execute_callback_with_buffer(qtz_request_t request) noexcept;

};

inline struct qtz_mailbox* qtz_request::parent_mailbox() const noexcept {
  return const_cast<qtz_mailbox*>(reinterpret_cast<const qtz_mailbox*>(this - (thisSlot + 1)));
}


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
