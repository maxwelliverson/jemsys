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
#include <array>
#include <ranges>


namespace qtz {
  namespace ipc{

    template <typename T, size_t Alignment>
    class offset_ptr;
    template <typename T, size_t Extent = std::dynamic_extent>
    class span;

    namespace impl{
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
    }


    template <typename T, size_t Alignment = impl::default_alignment<T>>
    class offset_ptr : public impl::offset_ptr_base{
      template <typename, size_t>
      friend class offset_ptr;
    public:

      using element_type      = T;
      using pointer           = T*;
      using reference         = typename impl::add_ref<T>::type;
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
          : impl::offset_ptr_base(ptr){}

      template <impl::pointer_castable_to<T> U, size_t Al>
      JEM_forceinline explicit(!impl::pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U, Al>& other) noexcept
          : impl::offset_ptr_base(static_cast<pointer>(other.get())){}
      template <impl::noop_pointer_castable_to<T> U, size_t Al>
      JEM_forceinline explicit(!impl::noop_pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U, Al>& other) noexcept{
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

      JEM_forceinline offset_ptr & operator+=(difference_type index) noexcept requires impl::complete_type<T>{
        offset_ += (index * impl::type_size<T>::value);
        return *this;
      }
      JEM_forceinline offset_ptr & operator-=(difference_type index) noexcept requires impl::complete_type<T> {
        offset_ -= (index * impl::type_size<T>::value);
        return *this;
      }
      JEM_forceinline offset_ptr & operator++() noexcept requires impl::complete_type<T> {
        offset_ += impl::type_size<T>::value;
        return *this;
      }
      JEM_forceinline offset_ptr operator++(int) noexcept requires impl::complete_type<T> {
        pointer tmp = get();
        offset_ += impl::type_size<T>::value;
        return {tmp};
      }
      JEM_forceinline offset_ptr & operator--() noexcept requires impl::complete_type<T> {
        offset_ -= impl::type_size<T>::value;
        return *this;
      }
      JEM_forceinline offset_ptr operator--(int) noexcept requires impl::complete_type<T> {
        pointer tmp = get();
        offset_ -= impl::type_size<T>::value;
        return {tmp};
      }



      // public static functions
      JEM_forceinline static offset_ptr pointer_to(reference ref) noexcept requires(!std::same_as<void, reference>) {
        return {std::addressof(ref)};
      }

      // friend functions

      JEM_forceinline friend offset_ptr      operator+(offset_ptr ptr, difference_type index) noexcept requires impl::complete_type<T> {
        pointer p = ptr.get() + index;
        return {p};
      }
      JEM_forceinline friend offset_ptr      operator-(offset_ptr ptr, difference_type index) noexcept requires impl::complete_type<T> {
        pointer p = ptr.get() - index;
        return {p};
      }

      template <size_t N>
      JEM_forceinline difference_type operator-(const offset_ptr<T, N>& other) const noexcept requires impl::complete_type<T> {
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

    template <typename T, typename VoidPtr = offset_ptr<void>>
    class intrusive_ptr{
      using void_traits = std::pointer_traits<VoidPtr>;
    public:
      using pointer = typename void_traits::template rebind<T>;


    private:
      pointer ptr_;
    };


    namespace impl{
      template <typename T>
      class span_base{
      protected:
        span_base() = default;
        span_base(T* ptr) noexcept                   : ptr(ptr){}
        span_base(const offset_ptr<T>& ptr) noexcept : ptr(ptr){}
      public:

        using element_type     = T;
        using value_type       = std::remove_cv_t<T>;
        using difference_type  = ptrdiff_t;
        using pointer          = offset_ptr<T>;
        using const_pointer    = offset_ptr<const T>;
        using reference        = T&;
        using const_reference  = const T&;
        using iterator         = offset_ptr<T>;
        using reverse_iterator = std::reverse_iterator<offset_ptr<T>>;




        pointer          data() const noexcept {
          return ptr;
        }

        reference        front() const noexcept {
          return *ptr;
        }

        reference        operator[](size_t index) const noexcept {
          return ptr[index];
        }

        iterator         begin() const noexcept {
          return ptr;
        }
        reverse_iterator rend() const noexcept {
          return reverse_iterator(begin());
        }



      private:
        offset_ptr<T> ptr = nullptr;
      };

      template <size_t Extent>
      struct span_size_base{
        using size_type = size_t;

        [[nodiscard]] size_type size() const noexcept {
          return Extent;
        }
        [[nodiscard]] bool      empty() const noexcept {
          return !Extent;
        }

      protected:
        span_size_base() = default;
        span_size_base(size_type size_){
          assert(size_ == Extent);
        }
      };
      template <>
      struct span_size_base<std::dynamic_extent>{
        using size_type = size_t;

        [[nodiscard]] size_type size() const noexcept {
          return size_;
        }
        [[nodiscard]] bool      empty() const noexcept {
          return !size_;
        }

      protected:
        span_size_base() = default;
        span_size_base(size_type size) noexcept : size_(size) {}

        size_type size_ = 0;
      };

      template <typename T>
      struct is_not_span_or_array : std::true_type{};
      template <typename T, size_t N>
      struct is_not_span_or_array<std::array<T, N>> : std::false_type{};
      template <typename T> requires(std::is_array_v<T>)
          struct is_not_span_or_array<T> : std::false_type{};
      template <typename T, size_t N>
      struct is_not_span_or_array<span<T, N>> : std::false_type{};
      template <typename T> requires(std::is_const_v<T> || std::is_volatile_v<T> || std::is_reference_v<T>)
          struct is_not_span_or_array<T> : is_not_span_or_array<std::remove_cvref_t<T>>{};

      template <typename T, typename U>
      struct is_qualified_convertible : std::false_type{};
      template <typename T>
      struct is_qualified_convertible<T, T> : std::true_type{};
      template <typename T>
      struct is_qualified_convertible<T, const T> : std::true_type{};
      template <typename T>
      struct is_qualified_convertible<T, volatile T> : std::true_type{};
      template <typename T>
      struct is_qualified_convertible<T, const volatile T> : std::true_type{};
      template <typename T>
      struct is_qualified_convertible<const T, const volatile T> : std::true_type{};
      template <typename T>
      struct is_qualified_convertible<volatile T, const volatile T> : std::true_type{};

      template <typename T, typename U>
      concept qualified_convertible_to = is_qualified_convertible<T, U>::value;
    }

    template <typename T, size_t Extent>
    class span : public impl::span_base<T>, public impl::span_size_base<Extent>{

      consteval static size_t get_subspan_extent(size_t Offset, size_t Count) {
        if ( Count != std::dynamic_extent )
          return Count;
        if ( Extent != std::dynamic_extent )
          return Extent - Count;
        return std::dynamic_extent;
      }

      template <typename Rng>
      inline constexpr static bool IsValidRange =
          std::ranges::contiguous_range<Rng> &&
          std::ranges::sized_range<Rng> &&
          (std::ranges::borrowed_range<Rng> || std::is_const_v<T>) &&
          impl::is_not_span_or_array<Rng>::value &&
          impl::qualified_convertible_to<std::ranges::range_value_t<Rng>, T>;

    public:
      span() = default;
      span(const span&) noexcept = default;

      template <impl::qualified_convertible_to<T> U, size_t N>
      requires(Extent == std::dynamic_extent || N == std::dynamic_extent || Extent == N)
      explicit(Extent != std::dynamic_extent && N == std::dynamic_extent)
      span(const span<U, N>& other) noexcept
          : impl::span_base<T>(other.data()),
            impl::span_size_base<Extent>(other.size()){}


      explicit(Extent != std::dynamic_extent) span(T* address, size_t length) noexcept
          : impl::span_base<T>(address),
            impl::span_size_base<Extent>(length) {}
      explicit(Extent != std::dynamic_extent) span(const offset_ptr<T>& address, size_t length) noexcept
          : impl::span_base<T>(address),
            impl::span_size_base<Extent>(length) {}

      explicit(Extent != std::dynamic_extent) span(T* first, T* last) noexcept
          : impl::span_base<T>(first),
            impl::span_size_base<Extent>(last - first) {}
      explicit(Extent != std::dynamic_extent) span(const offset_ptr<T>& first, const offset_ptr<T>& last) noexcept
          : impl::span_base<T>(first),
            impl::span_size_base<Extent>(last - first) {}

      template <size_t N> requires(Extent == std::dynamic_extent || N == Extent)
      span(T(& array)[N]) noexcept
          : impl::span_base<T>(array),
            impl::span_size_base<Extent>(N){}
      template <impl::qualified_convertible_to<T> U, size_t N> requires(Extent == std::dynamic_extent || N == Extent)
      span(std::array<U, N>& array) noexcept
          : impl::span_base<T>(array.data()),
            impl::span_size_base<Extent>(N){}

      template <impl::qualified_convertible_to<T> U, size_t N> requires(Extent == std::dynamic_extent || N == Extent)
      span(const std::array<U, N>& array) noexcept
          : impl::span_base<T>(array.data()),
            impl::span_size_base<Extent>(N){}

      template <typename Rng> requires(IsValidRange<Rng>)
      explicit(Extent != std::dynamic_extent) span(Rng&& range) noexcept
          : impl::span_base<T>(std::ranges::data(range)),
            impl::span_size_base<Extent>(std::ranges::size(range)){}


      using iterator         = typename impl::span_base<T>::iterator;
      using reference        = typename impl::span_base<T>::reference;
      using reverse_iterator = typename impl::span_base<T>::reverse_iterator;
      using size_type        = typename impl::span_size_base<Extent>::size_type;


      iterator end() const noexcept {
        return this->data() + this->size();
      }
      reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
      }

      reference back() const noexcept {
        return *rbegin();
      }


      [[nodiscard]] size_type size_bytes() const noexcept {
        return this->size() * sizeof(T);
      }


      template <size_t Count> requires(Count <= Extent)
      [[nodiscard]] span<T, Count> first() const noexcept {
        return span<T, Count>{ this->data(), Count };
      }
      [[nodiscard]] span<T>        first(size_type Count) const noexcept {
        return span<T>{ this->data(), Count };
      }

      template <size_t Count> requires(Count <= Extent)
      [[nodiscard]] span<T, Count> last() const noexcept {
        return span<T, Count>{ this->data() + (this->size() - Count), Count };
      }
      [[nodiscard]] span<T>        last(size_type Count) const noexcept {
        return span<T>{ this->data() + (this->size() - Count), Count };
      }

      template <size_t Offset, size_t Count = std::dynamic_extent> requires(Offset <= Extent && (Count == std::dynamic_extent || Count <= (Extent - Offset)))
      [[nodiscard]] span<T, get_subspan_extent(Offset, Count)> subspan() const noexcept {
        return span<T, get_subspan_extent(Offset, Count)>{ this->data() + Offset, Count == std::dynamic_extent ? this->size() - Offset : Count };
      }
      [[nodiscard]] span<T> subspan(size_t Offset, size_t Count = std::dynamic_extent) const noexcept {
        return span<T>{ this->data() + Offset, Count == std::dynamic_extent ? this->size() - Offset : Count };
      }
    };


    struct file_mapping{
      void*      handle;
      jem_size_t size;
      jem_char_t name[];
    };
    struct anonymous_file_mapping {
      void*      handle;
      jem_size_t size;
    };
    struct mapped_region{

    };

  }


  namespace {

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
    struct virtual_deputy       : deputy_base{};
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

extern "C" JEM_stdcall qtz_exit_code_t qtz_mailbox_main_thread_proc(void *params);



#endif//JEMSYS_QUARTZ_INTERNAL_H
