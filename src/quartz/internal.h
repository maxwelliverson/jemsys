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

    template <typename T>
    struct add_ref{
      using type = T&;
    };
    template <typename V> requires(std::same_as<void, std::remove_cv_t<V>>)
    struct add_ref<V>{
      using type = void;
    };

    template <typename T>
    struct type_size;
    template <typename T>
    struct type_align;

    template <typename T> requires(requires{ alignof(std::remove_cv_t<T>); })
    struct type_align<T>{
      inline constexpr static size_t value = alignof(std::remove_cv_t<T>);
    };
    template <typename V> requires(std::same_as<void, std::remove_cv_t<V>>)
    struct type_align<V>{
      inline constexpr static size_t value = 1;
    };
    template <typename T> requires(requires{ sizeof(std::remove_cv_t<T>); })
    struct type_size<T>{
      inline constexpr static size_t value = sizeof(std::remove_cv_t<T>);
    };
    template <typename V> requires(std::same_as<void, std::remove_cv_t<V>>)
    struct type_size<V>{
      inline constexpr static size_t value = 1;
    };

    template <typename T>
    concept incomplete_type = !requires{
      sizeof(type_size<T>);
      sizeof(type_align<T>);
    };
    template <typename T>
    concept complete_type = !incomplete_type<T>;

    template <typename T>
    inline constexpr size_t default_alignment = type_align<T>::value;
    template <incomplete_type T>
    inline constexpr size_t default_alignment<T> = 1;





    
    template <typename To, typename From>
    struct static_cast_maintains_address_tester{
      
      inline constexpr static size_t ToSize   = sizeof(To);
      inline constexpr static size_t FromSize = sizeof(From);
      inline constexpr static size_t BufferSize = 2 * std::max(ToSize, FromSize);
      
      union{
        char raw_buffer[BufferSize]{};
        From from[2];
      };
      
      constexpr bool operator()() noexcept {
        void* fromAddress = from + 1;
        void* toAddress   = static_cast<To*>(from + 1);
        return fromAddress == toAddress;
      }
    };
    template <typename A, typename B>
    struct static_cast_maintains_address{
      using To = std::conditional_t<std::derived_from<A, B>, B, A>;
      using From = std::conditional_t<std::derived_from<A, B>, A, B>;
      inline constexpr static bool value = static_cast_maintains_address_tester<To, From>()();
    };
    
    template <typename From, typename To>
    concept pointer_castable_to = requires(From* from){
      static_cast<To*>(from);
    };
    template <typename From, typename To>
    concept pointer_convertible_to = std::convertible_to<From*, To*> && pointer_castable_to<From, To>;
    template <typename From, typename To>
    concept noop_pointer_castable_to = pointer_castable_to<From, To> && static_cast_maintains_address<To, From>::value;
    template <typename From, typename To>
    concept noop_pointer_convertible_to = pointer_convertible_to<From, To> && noop_pointer_castable_to<From, To>;

//#define TEST_MAINTAINS_ADDRESS
#if defined(TEST_MAINTAINS_ADDRESS)
    struct A{ int a; };
    struct B{ int b; };
    struct C : A, B{};

    static_assert(pointer_castable_to<C, A>);
    static_assert(pointer_castable_to<C, B>);
    static_assert(pointer_castable_to<A, C>);
    static_assert(pointer_castable_to<B, C>);

    static_assert(pointer_convertible_to<C, A>);
    static_assert(pointer_convertible_to<C, B>);
    static_assert(!pointer_convertible_to<A, C>);
    static_assert(!pointer_convertible_to<B, C>);

    static_assert(noop_pointer_castable_to<C, A>);
    static_assert(!noop_pointer_castable_to<C, B>);
    static_assert(noop_pointer_castable_to<A, C>);
    static_assert(!noop_pointer_castable_to<B, C>);

    static_assert(noop_pointer_convertible_to<C, A>);
    static_assert(!noop_pointer_convertible_to<C, B>);
    static_assert(!noop_pointer_convertible_to<A, C>);
    static_assert(!noop_pointer_convertible_to<B, C>);
#endif

    struct offset_ptr_base{

    protected:
      using integer_type = jem_u64_t;
      using pointer_type = const volatile void*;

      offset_ptr_base() = default;
      explicit offset_ptr_base(pointer_type addr) noexcept
          : offset_(cast_to_offset(this, addr)){}

      JEM_nodiscard JEM_forceinline static integer_type cast_to_offset(pointer_type offset_ptr, pointer_type address) noexcept {
        const auto IsNull  = static_cast<integer_type>(address == nullptr);
        const auto Address = reinterpret_cast<integer_type>(address);
        const auto This    = reinterpret_cast<integer_type>(offset_ptr);
        return ((Address - This) & (IsNull - 1ULL)) | IsNull;
      }
      JEM_nodiscard JEM_forceinline static void*        cast_to_pointer(pointer_type offset_ptr, integer_type offset) noexcept {
        return reinterpret_cast<void*>((reinterpret_cast<integer_type>(offset_ptr) + offset) & (static_cast<integer_type>(offset == 1ULL) - 1ULL));
      }
      JEM_nodiscard JEM_forceinline static integer_type translate_offset(pointer_type to, pointer_type from, integer_type offset) noexcept {
        const auto OtherIsNull = static_cast<integer_type>(offset == 1ULL);
        return ((offset + reinterpret_cast<integer_type>(from) - reinterpret_cast<integer_type>(to)) & (OtherIsNull - 1ULL)) | OtherIsNull;
      }

      inline void ptr_to_offset(pointer_type address) noexcept {
        offset_ = cast_to_offset(this, address);
      }
      inline void copy_from(const offset_ptr_base& other) noexcept {
        offset_ = translate_offset(this, &other, other.offset_);
      }
      JEM_nodiscard inline void* integer_to_ptr() const noexcept {
        return cast_to_pointer(this, offset_);
      }
      inline void swap_with(offset_ptr_base& other) noexcept {
        integer_type tmp = translate_offset(&other, this, offset_);
        offset_ = translate_offset(this, &other, other.offset_);
        other.offset_ = tmp;
      }

      JEM_nodiscard JEM_forceinline bool is_null() const noexcept {
        return offset_ == 1;
      }

      integer_type offset_ = 1;
    };

    template <typename T, size_t Alignment = default_alignment<T>>
    class offset_ptr : public offset_ptr_base{
      template <typename, size_t>
      friend class offset_ptr;
    public:

      using element_type      = T;
      using pointer           = T*;
      using reference         = typename add_ref<T>::type;
      using value_type        = std::remove_cv_t<T>;
      using difference_type   = jem_i64_t;
      using iterator_concept  = std::contiguous_iterator_tag;
      using iterator_category = std::random_access_iterator_tag;
      using offset_type       = jem_u64_t;
      template <typename U>
      using rebind            = offset_ptr<U>;


      JEM_forceinline offset_ptr() = default;
      JEM_forceinline offset_ptr(const offset_ptr& other) noexcept{
        copy_from(other);
      }
      JEM_forceinline offset_ptr(offset_ptr&& other) noexcept {
        copy_from(other);
      }
      JEM_forceinline offset_ptr(std::nullptr_t) noexcept { }

      JEM_forceinline offset_ptr(T* ptr) noexcept
          : offset_ptr_base(ptr){}

      template <pointer_castable_to<T> U, size_t Al>
      JEM_forceinline explicit(!pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U, Al>& other) noexcept
          : offset_ptr_base(static_cast<pointer>(other.get())){}
      template <noop_pointer_castable_to<T> U, size_t Al>
      JEM_forceinline explicit(!noop_pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U, Al>& other) noexcept{
        copy_from(other);
      }



      JEM_nodiscard JEM_forceinline pointer     get() const noexcept {
        return std::assume_aligned<Alignment>(static_cast<pointer>(integer_to_ptr()));
      }
      JEM_nodiscard JEM_forceinline offset_type get_offset() const noexcept {
        return offset_;
      }

      JEM_nodiscard JEM_forceinline pointer operator->() const noexcept {
        return get();
      }
      JEM_nodiscard JEM_forceinline reference operator*() const noexcept requires(!std::same_as<void, reference>) {
        return *get();
      }
      JEM_nodiscard JEM_forceinline reference operator[](difference_type index) const noexcept {
        return get()[index];
      }

      JEM_forceinline offset_ptr & operator+=(difference_type index) noexcept requires complete_type<T>{
        offset_ += (index * type_size<T>::value);
        return *this;
      }
      JEM_forceinline offset_ptr & operator-=(difference_type index) noexcept requires complete_type<T> {
        offset_ -= (index * type_size<T>::value);
        return *this;
      }
      JEM_forceinline offset_ptr & operator++() noexcept requires complete_type<T> {
        offset_ += type_size<T>::value;
        return *this;
      }
      JEM_forceinline offset_ptr operator++(int) noexcept requires complete_type<T> {
        pointer tmp = get();
        offset_ += type_size<T>::value;
        return {tmp};
      }
      JEM_forceinline offset_ptr & operator--() noexcept requires complete_type<T> {
        offset_ -= type_size<T>::value;
        return *this;
      }
      JEM_forceinline offset_ptr operator--(int) noexcept requires complete_type<T> {
        pointer tmp = get();
        offset_ -= type_size<T>::value;
        return {tmp};
      }



      // public static functions
      JEM_forceinline static offset_ptr pointer_to(reference ref) noexcept requires(!std::same_as<void, reference>) {
        return {std::addressof(ref)};
      }

      // friend functions

      JEM_forceinline friend offset_ptr      operator+(offset_ptr ptr, difference_type index) noexcept requires complete_type<T> {
        pointer p = ptr.get() + index;
        return {p};
      }
      JEM_forceinline friend offset_ptr      operator-(offset_ptr ptr, difference_type index) noexcept requires complete_type<T> {
        pointer p = ptr.get() - index;
        return {p};
      }

      template <size_t N>
      JEM_forceinline difference_type operator-(const offset_ptr<T, N>& other) const noexcept requires complete_type<T> {
        return get() - other.get();
      }


      JEM_forceinline explicit operator bool() const noexcept {
        return !is_null();
      }

      JEM_forceinline friend bool operator==(const offset_ptr& ptr, std::nullptr_t) noexcept {
        return ptr.is_null();
      }

      template <size_t N>
      JEM_forceinline bool operator==(const offset_ptr<T, N>& other) const noexcept{
        return get() == other.get();
      }
      template <size_t N>
      JEM_forceinline std::strong_ordering operator<=>(const offset_ptr<T, N>& other) const noexcept{
        return get() <=> other.get();
      }
    };

    template <typename Word, size_t Bits = std::dynamic_extent>
    struct bitmap{

      inline constexpr static size_t Extent = Bits == std::dynamic_extent ? Bits : (Bits / (sizeof(Word) * 8));

      std::span<Word, Extent> fields;
    };

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
