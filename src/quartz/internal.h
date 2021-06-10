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


namespace qtz {
  namespace ipc{

    template <typename T>
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
        inline constexpr static size_t BufferSize = 4 * std::max(ToSize, FromSize);

        union{
          char raw_buffer[BufferSize]{ };
          From from[2];
        } mem = {};

        constexpr static_cast_maintains_address_tester() = default;

        constexpr bool operator()() noexcept {
          const void* fromAddress = mem.raw_buffer + sizeof(From);
          const void* toAddress   = static_cast<To*>(mem.from + 1);
          return fromAddress == toAddress;
        }
      };
      template <typename A, typename B>
      struct static_cast_maintains_address{
        using To = std::conditional_t<std::derived_from<A, B>, B, A>;
        using From = std::conditional_t<std::derived_from<A, B>, A, B>;
        inline constexpr static bool value = static_cast_maintains_address_tester<To, From>()();
      };
      template <typename A, typename B> requires(incomplete_type<A> || incomplete_type<B>)
      struct static_cast_maintains_address<A, B>{
        inline constexpr static bool value = false;
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


    template <typename T>
    class offset_ptr : public impl::offset_ptr_base{
      template <typename>
      friend class offset_ptr;
    public:

      using element_type      = T;
      using pointer           = T*;
      using reference         = typename impl::add_ref<T>::type;
      using value_type        = std::remove_cv_t<T>;
      using difference_type   = jem_ptrdiff_t;
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

      template <impl::pointer_castable_to<T> U>
      JEM_forceinline explicit(!impl::pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U>& other) noexcept
          : impl::offset_ptr_base(static_cast<pointer>(other.get())){}
      template <impl::noop_pointer_castable_to<T> U>
      JEM_forceinline explicit(!impl::noop_pointer_convertible_to<U, T>) offset_ptr(const offset_ptr<U>& other) noexcept{
        copy_from(other);
      }


      JEM_forceinline offset_ptr& operator=(std::nullptr_t) noexcept {
        offset_ = 1;
        return *this;
      }
      JEM_forceinline offset_ptr& operator=(pointer ptr) noexcept {
        offset_ = cast_to_offset(this, ptr);
        return *this;
      }
      JEM_forceinline offset_ptr& operator=(const offset_ptr& other) noexcept {
        copy_from(other);
        return *this;
      }
      JEM_forceinline offset_ptr& operator=(offset_ptr&& other) noexcept {
        copy_from(other);
        return *this;
      }



      JEM_nodiscard JEM_forceinline pointer     get() const noexcept {
        return static_cast<pointer>(integer_to_ptr());
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
      JEM_forceinline friend difference_type operator-(const offset_ptr& a, const offset_ptr& b) noexcept requires impl::complete_type<T> {
        return a.get() - b.get();
      }


      JEM_forceinline explicit operator bool() const noexcept {
        return !is_null();
      }

      JEM_forceinline friend bool operator==(const offset_ptr& ptr, std::nullptr_t) noexcept {
        return ptr.is_null();
      }

      template <size_t N>
      JEM_forceinline friend bool operator==(const offset_ptr& a, const offset_ptr& b) noexcept{
        return a.get() == b.get();
      }
      template <size_t N>
      JEM_forceinline friend std::strong_ordering operator<=>(const offset_ptr& a, const offset_ptr& b) noexcept{
        return a.get() <=> b.get();
      }
    };

    /*template <typename T, typename VoidPtr = offset_ptr<void>>
    class intrusive_ptr{
      using void_traits = std::pointer_traits<VoidPtr>;
    public:
      using pointer = typename void_traits::template rebind<T>;


    private:
      pointer ptr_;
    };*/


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

    template <typename Rng, typename T>
    concept span_of = std::ranges::contiguous_range<Rng> &&
                      std::ranges::sized_range<Rng> &&
                      (std::ranges::borrowed_range<Rng> || std::is_const_v<T>) &&
                      impl::is_not_span_or_array<Rng>::value &&
                      impl::qualified_convertible_to<std::ranges::range_value_t<Rng>, T>;

    template <typename T, size_t Extent>
    class span : public impl::span_base<T>, public impl::span_size_base<Extent>{

      consteval static size_t get_subspan_extent(size_t Offset, size_t Count) {
        if ( Count != std::dynamic_extent )
          return Count;
        if ( Extent != std::dynamic_extent )
          return Extent - Count;
        return std::dynamic_extent;
      }


    public:
      span() = default;
      span(const span&) noexcept = default;

      template <impl::qualified_convertible_to<T> U, size_t N>
      requires(Extent == std::dynamic_extent || N == std::dynamic_extent || Extent == N)
      explicit(Extent != std::dynamic_extent && N == std::dynamic_extent)
      span(const span<U, N>& other) noexcept
          : impl::span_base<T>(other.data()),
            impl::span_size_base<Extent>(other.size()){}


      span(std::span<T, Extent> std_span) noexcept
          : impl::span_base<T>(std_span.data()),
            impl::span_size_base<Extent>(std_span.size())
      {}


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

      template <span_of<T> Rng>
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


    class string{
    public:

      using element_type     = const char;
      using value_type       = char;
      using pointer          = const char*;
      using size_type        = jem_size_t;
      using difference_type  = jem_ptrdiff_t;

      using iterator         = pointer;
      using reverse_iterator = std::reverse_iterator<pointer>;

      string() noexcept = default;
      string(const string& other) noexcept{
        mem.kind = other.mem.kind;
        if ( mem.kind == InternedString ) {
          mem.ref.address = other.mem.ref.address;
          mem.ref.length  = other.mem.ref.length;
        }
        else {
          memcpy(&mem, &other.mem, sizeof(string));
        }
      }
      string(const char* str, jem_size_t length) noexcept{
        if ( length == 0 ) {
          mem.kind = EmptyString;
        }
        else if ( length <= MaxInlineLength ) {
          memset(mem.inl.buffer, 0, InlineBufferLength);
          mem.inl.buffer[0] = InlineString;
          memcpy(mem.inl.buffer + 1, str, length);
          mem.inl.buffer[MaxInlineLength + 1] = char(MaxInlineLength - length);
        }
        else {
          mem.ref.kind = InternedString;
          mem.ref.address = str;
          mem.ref.length  = length;
        }
      }
      string(std::string_view sv) noexcept : string(sv.data(), sv.length()){ }


      string& operator=(std::string_view other) noexcept {
        new(this) string(other);
        return *this;
      }
      string& operator=(const string& other) noexcept {
        new(this) string(other);
        return *this;
      }

      JEM_nodiscard pointer   data() const noexcept {
        switch( mem.kind ) {
          case EmptyString:
            return nullptr;
          case InlineString:
            return mem.inl.buffer + 1;
          case InternedString:
            return mem.ref.address.get();
          JEM_no_default;
        }
      }
      JEM_nodiscard size_type size() const noexcept {
        return length();
      }
      JEM_nodiscard size_type length() const noexcept {
        switch( mem.kind ) {
          case EmptyString:
            return 0;
          case InlineString:
            return MaxInlineLength - mem.inl.buffer[MaxInlineLength - 1];
          case InternedString:
            return mem.ref.length;
          JEM_no_default;
        }
      }

      JEM_nodiscard iterator begin() const noexcept {
        return data();
      }
      JEM_nodiscard iterator end()   const noexcept {
        return begin() + length();
      }

      JEM_nodiscard reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
      }
      JEM_nodiscard reverse_iterator rend()   const noexcept {
        return reverse_iterator(begin());
      }


      JEM_nodiscard std::string_view str() const noexcept {
        return { data(), length() };
      }



    private:

      inline static constexpr auto EmptyString    = 0x0;
      inline static constexpr auto InlineString   = 0x1;
      inline static constexpr auto InternedString = 0x2;

      inline static constexpr auto KindBits = CHAR_BIT;
      inline static constexpr auto LengthBits = (sizeof(void*) - 1) * KindBits;
      inline static constexpr auto InlineBufferLength = (2*sizeof(void*));
      inline static constexpr auto MaxInlineLength = InlineBufferLength - 2;

      union{
        jem_u8_t kind = EmptyString;
        struct {
          char buffer[InlineBufferLength];
        } inl;
        struct {
          jem_size_t             kind   : KindBits;
          jem_size_t             length : LengthBits;
          offset_ptr<const char> address;
        } ref;
      } mem = {};
    };




    /*class multi_segment_services {
    public:
      virtual std::pair<void *, jem_size_t> create_new_segment(jem_size_t mem) = 0;
      virtual bool                          update_segments   () = 0;
      virtual ~multi_segment_services() = default;
    };

    namespace impl{

      template <typename T>
      class set{

      };

      template <typename From, typename To>
      class flat_map{

      };

      consteval jem_size_t getPowSizeBits(jem_size_t MaxSegmentSizeBits) {
        if ( MaxSegmentSizeBits == 0)
          return 0;

        jem_size_t result = MaxSegmentSizeBits;
        do {
          result |= (result - 1);
          ++result;
        } while ( result & (result - 1) );
        jem_size_t count = 0;
        while ( result != 1 ) {
          result >>= 1;
          count += 1;
        }
        return count;
      }

      JEM_forceinline static jem_size_t align_size(jem_size_t Size, jem_size_t Align) JEM_noexcept {
        return ((Size-1)&(~(Align-1))) + Align;
      }
      JEM_forceinline static jem_size_t floor_log2(jem_size_t Value) JEM_noexcept {
        assert( Value != 0 );
        unsigned long result;
        if constexpr ( sizeof(void*) == 4 )
          _BitScanReverse(&result, Value);
        else
          _BitScanReverse64(&result, Value);
        return result;
      }

      struct portable_ptr_base{

        using self_t = portable_ptr_base;
        static_assert(sizeof(jem_size_t) == sizeof(void*));
        static_assert(sizeof(void*)*CHAR_BIT == 64);
        inline static constexpr jem_size_t PlatformBits = sizeof(void*)*CHAR_BIT;
        inline static constexpr jem_size_t CtrlBits     = 2;
        inline static constexpr jem_size_t ExtraBits    = 2;
        inline static constexpr jem_size_t AlignBits    = 12;
        inline static constexpr jem_size_t Alignment    = (static_cast<jem_size_t>(1) << AlignBits);
        inline static constexpr jem_size_t AlignMask    = ~(Alignment - static_cast<jem_size_t>(1));
        inline static constexpr jem_size_t MaxSegmentSizeBits = PlatformBits - CtrlBits;
        inline static constexpr jem_size_t MaxSegmentSize = (static_cast<jem_size_t>(1) << MaxSegmentSizeBits);

        inline static constexpr jem_size_t BeginBits = MaxSegmentSizeBits - AlignBits;
        inline static constexpr jem_size_t PowSizeBits = getPowSizeBits(MaxSegmentSizeBits);
        inline static constexpr jem_size_t FrcSizeBits = PlatformBits - CtrlBits - BeginBits - PowSizeBits;

        static_assert((PlatformBits - PowSizeBits - FrcSizeBits) >= CtrlBits);

        inline static constexpr jem_size_t RelativeOffsetBits = PlatformBits - ExtraBits;
        inline static constexpr jem_size_t RelativeSizeBits   = PlatformBits - MaxSegmentSizeBits - CtrlBits;

        enum : jem_u8_t {
          is_pointee_outside,
          is_in_stack,
          is_relative,
          is_segmented,
          is_max_mode
        };

        struct relative_addressing{
          jem_size_t ctrl      : CtrlBits;
          jem_size_t pow       : PowSizeBits;
          jem_size_t frc       : FrcSizeBits;
          jem_size_t beg       : BeginBits;
          jem_ptrdiff_t offset : MaxSegmentSizeBits;
          jem_ptrdiff_t bits   : ExtraBits;
        };
        struct segmented_addressing{
          jem_size_t ctrl    : CtrlBits;
          jem_size_t segment : MaxSegmentSizeBits;
          jem_size_t offset  : MaxSegmentSizeBits;
          jem_size_t bits    : ExtraBits;
        };
        struct direct_addressing{
          jem_size_t ctrl    : CtrlBits;
          jem_size_t dummy   : MaxSegmentSizeBits;
          void*      address;
        };

        union members_t{
          relative_addressing  relative;
          segmented_addressing segmented;
          direct_addressing    direct;
        } members;

        static_assert(sizeof(members_t) == (2 * sizeof(void*)));

        JEM_nodiscard inline void* relative_calculate_begin_address() const JEM_noexcept {
          const jem_size_t Begin      = this->members.relative.beg;
          const jem_size_t MaskedThis = reinterpret_cast<jem_size_t>(this) & AlignMask;
          return reinterpret_cast<void*>(MaskedThis - (Begin << AlignBits));
        }
        inline void relative_set_begin_from_base(void* address) {
          assert( address < static_cast<void*>(this));
          jem_size_t off = reinterpret_cast<char*>(this) - reinterpret_cast<char*>(address);
          members.relative.beg = off >> AlignBits;
        }
        
        //!Obtains the address pointed by the
        //!object
        JEM_nodiscard jem_size_t relative_size() const JEM_noexcept {
          const jem_size_t pow  = members.relative.pow;
          jem_size_t size = (jem_size_t(1u) << pow);
          assert(pow >= FrcSizeBits);
          size |= jem_size_t(members.relative.frc) << (pow - FrcSizeBits);
          return size;
        }

        JEM_nodiscard static jem_size_t calculate_size(jem_size_t orig_size, jem_size_t &pow, jem_size_t &frc) JEM_noexcept {
          if(orig_size < Alignment)
            orig_size = Alignment;
          orig_size = align_size(orig_size, Alignment);
          pow = floor_log2(orig_size);
          const jem_size_t low_size = (jem_size_t(1) << pow);
          const jem_size_t diff = orig_size - low_size;
          const jem_size_t frc_bits = pow - FrcSizeBits;
          assert(pow >= FrcSizeBits);
          jem_size_t rounded = align_size(diff, (jem_size_t)(jem_size_t(1) << frc_bits));
          if(rounded == low_size){
            ++pow;
            frc = 0;
            rounded = 0;
          }
          else{
            frc = rounded >> frc_bits;
          }
          assert(((frc << frc_bits) & (Alignment - 1))==0);
          return low_size + rounded;
        }

        JEM_nodiscard jem_size_t get_mode() const JEM_noexcept {
          return members.direct.ctrl;
        }

        void set_mode(jem_size_t mode) JEM_noexcept {
          assert(mode < is_max_mode);
          members.direct.ctrl = mode;
        }

        //!Returns true if object represents
        //!null pointer
        JEM_nodiscard bool is_null() const JEM_noexcept {
          return (this->get_mode() < is_relative) &&
                 !members.direct.dummy &&
                 !members.direct.address;
        }

        //!Sets the object to represent
        //!the null pointer
        void set_null() JEM_noexcept {
          if(this->get_mode() >= is_relative) {
            this->set_mode(is_pointee_outside);
          }
          members.direct.dummy   = 0;
          members.direct.address = nullptr;
        }

        JEM_nodiscard static jem_size_t round_size(jem_size_t orig_size) JEM_noexcept {
          jem_size_t pow, frc;
          return calculate_size(orig_size, pow, frc);
        }
      };
      
      template <typename Mutex>
      struct portable_ptr_flat_map : public portable_ptr_base {
        using self_t = portable_ptr_flat_map;

        void set_from_pointer(const volatile void *ptr) {
          this->set_from_pointer(const_cast<const void *>(ptr));
        }

        JEM_nodiscard void* to_raw_pointer() const JEM_noexcept {
          if(is_null()){
            return nullptr;
          }
          switch(this->get_mode()){
            case is_relative:
              return const_cast<char*>(reinterpret_cast<const char*>(this)) + members.relative.offset;
            case is_segmented: {
                segment_info_t segment_info;
                jem_size_t offset;
                void *this_base;
                get_segment_info_and_offset(this, segment_info, offset, this_base);
                char *base  = static_cast<char*>(segment_info.group->address_of(members.segmented.segment));
                return base + members.segmented.offset;
              }
            case is_in_stack:
            case is_pointee_outside:
              return members.direct.address;
            default:
              return nullptr;
          }
        }

        JEM_nodiscard jem_ptrdiff_t diff(const self_t &other) const JEM_noexcept {
          return static_cast<char*>(this->to_raw_pointer()) -
                 static_cast<char*>(other.to_raw_pointer());
        }

        JEM_nodiscard bool equal(const self_t &y) const JEM_noexcept {
          return this->to_raw_pointer() == y.to_raw_pointer();
        }

        JEM_nodiscard bool less(const self_t &y) const JEM_noexcept {
          return this->to_raw_pointer() < y.to_raw_pointer();
        }

        void swap(self_t &other) JEM_noexcept {
          void *ptr_this  = this->to_raw_pointer();
          void *ptr_other = other.to_raw_pointer();
          other.set_from_pointer(ptr_this);
          this->set_from_pointer(ptr_other);
        }

        void set_from_pointer(const void *ptr) JEM_noexcept {
          if(!ptr){
            this->set_null();
            return;
          }

          jem_size_t mode = this->get_mode();
          if(mode == is_in_stack){
            members.direct.address = const_cast<void*>(ptr);
            return;
          }
          if(mode == is_relative){
            char *beg_addr = static_cast<char*>(this->relative_calculate_begin_addr());
            jem_size_t seg_size = this->relative_size();
            if(ptr >= beg_addr && ptr < (beg_addr + seg_size)){
              members.relative.offset = static_cast<const char*>(ptr) - reinterpret_cast<const char*>(this);
              return;
            }
          }
          jem_size_t ptr_offset;
          jem_size_t this_offset;
          segment_info_t ptr_info;
          segment_info_t this_info;
          void *ptr_base;
          void *this_base;
          get_segment_info_and_offset(this, this_info, this_offset, this_base);

          if(!this_info.group){
            this->set_mode(is_in_stack);
            this->members.direct.addr = const_cast<void*>(ptr);
          }
          else{
            get_segment_info_and_offset(ptr, ptr_info, ptr_offset, ptr_base);

            if(ptr_info.group != this_info.group){
              this->set_mode(is_pointee_outside);
              this->members.direct.addr =  const_cast<void*>(ptr);
            }
            else if(ptr_info.id == this_info.id){
              this->set_mode(is_relative);
              members.relative.offset = (static_cast<const char*>(ptr) - reinterpret_cast<const char*>(this));
              this->relative_set_begin_from_base(this_base);
              jem_size_t pow, frc;
              jem_size_t s = calculate_size(this_info.size, pow, frc);
              (void)s;
              assert(this_info.size == s);
              this->members.relative.pow = pow;
              this->members.relative.frc = frc;
            }
            else{
              this->set_mode(is_segmented);
              this->members.segmented.segment = ptr_info.id;
              this->members.segmented.off     = ptr_offset;
            }
          }
        }

        void set_from_other(const self_t &other) JEM_noexcept
        {
          this->set_from_pointer(other.to_raw_pointer());
        }

        void inc_offset(jem_ptrdiff_t bytes) JEM_noexcept
        {
          this->set_from_pointer(static_cast<char*>(this->to_raw_pointer()) + bytes);
        }
        void dec_offset(jem_ptrdiff_t bytes) JEM_noexcept {
          this->set_from_pointer(static_cast<char*>(this->to_raw_pointer()) - bytes);
        }

      private:

        class segment_group_t {
          struct segment_data{
            void *     addr;
            jem_size_t size;
          };
          std::vector<segment_data> m_segments;
          multi_segment_services&   m_ms_services;

        public:
          segment_group_t(multi_segment_services &ms_services)
              : m_ms_services(ms_services){ }

          void push_back(void *addr, jem_size_t size) noexcept {
            segment_data d = { addr, size };
            m_segments.push_back(d);
          }
          void pop_back() noexcept {
            assert(!m_segments.empty());
            m_segments.erase(--m_segments.end());
          }

          void *address_of(jem_size_t segment_id) noexcept {
            assert(segment_id < (jem_size_t)m_segments.size());
            return m_segments[segment_id].addr;
          }

          void clear_segments() noexcept {
            m_segments.clear();
          }

          JEM_nodiscard jem_size_t get_size() const noexcept {
            return m_segments.size();
          }

          JEM_nodiscard multi_segment_services& get_multi_segment_services() const noexcept {
            return m_ms_services;
          }

          friend bool operator< (const segment_group_t&l, const segment_group_t &r) noexcept {
            return &l.m_ms_services < &r.m_ms_services;
          }
        };

        struct segment_info_t {
          jem_size_t size;
          jem_size_t id;
          segment_group_t *group;
          segment_info_t() : size(0), id(0), group(0) {}
        };

        using segment_groups_t      = set<segment_group_t>;
        using ptr_to_segment_info_t = flat_map<const void*, segment_info_t>;


        struct mappings_t : Mutex {
          //!Mutex to preserve integrity in multi-threaded
          //!environments
          using mutex_type = Mutex;


          ptr_to_segment_info_t m_ptr_to_segment_info;

          ~mappings_t() {
            assert(m_ptr_to_segment_info.empty());
          }
        };

        //Static members
        static mappings_t       s_map;
        static segment_groups_t s_groups;
      public:

        using segment_group_id = segment_group_t*;

        //!Returns the segment and offset
        //!of an address
        static void get_segment_info_and_offset(const void *ptr, segment_info_t &segment, jem_size_t &offset, void *&base)
        {
          //------------------------------------------------------------------
          boost::interprocess::scoped_lock<typename mappings_t::mutex_type> lock(s_map);
          //------------------------------------------------------------------
          base = 0;
          if(s_map.m_ptr_to_segment_info.empty()){
            segment = segment_info_t();
            offset  = reinterpret_cast<const char*>(ptr) - static_cast<const char*>(0);
            return;
          }
          //Find the first base address greater than ptr
          typename ptr_to_segment_info_t::iterator it
          = s_map.m_ptr_to_segment_info.upper_bound(ptr);
          if(it == s_map.m_ptr_to_segment_info.begin()){
            segment = segment_info_t();
            offset  = reinterpret_cast<const char*>(ptr) - static_cast<const char *>(0);
          }
          //Go to the previous one
          --it;
          char *      segment_base = const_cast<char*>(reinterpret_cast<const char*>(it->first));
          jem_size_t segment_size = it->second.size;

          if(segment_base <= reinterpret_cast<const char*>(ptr) &&
          (segment_base + segment_size) >= reinterpret_cast<const char*>(ptr)){
            segment = it->second;
            offset  = reinterpret_cast<const char*>(ptr) - segment_base;
            base = segment_base;
          }
          else{
            segment = segment_info_t();
            offset  = reinterpret_cast<const char*>(ptr) - static_cast<const char*>(0);
          }
        }

        //!Associates a segment defined by group/id with a base address and size.
        //!Returns false if the group is not found or there is an error
        static void insert_mapping(segment_group_id group_id, void *ptr, jem_size_t size) {
          std::lock_guard lock(s_map);

          typedef typename ptr_to_segment_info_t::value_type value_type;
          typedef typename ptr_to_segment_info_t::iterator   iterator;
          typedef std::pair<iterator, bool>                  it_b_t;

          segment_info_t info;
          info.group = group_id;
          info.size  = size;
          info.id    = group_id->get_size();

          it_b_t ret = s_map.m_ptr_to_segment_info.insert(value_type(ptr, info));
          assert(ret.second);

          value_eraser<ptr_to_segment_info_t> v_eraser(s_map.m_ptr_to_segment_info, ret.first);
          group_id->push_back(ptr, size);
          v_eraser.release();
        }

        static bool erase_last_mapping(segment_group_id group_id) {
          std::lock_guard lock(s_map);

          if(!group_id->get_size()){
            return false;
          }
          else{
            void *addr = group_id->address_of(group_id->get_size()-1);
            group_id->pop_back();
            jem_size_t erased = s_map.m_ptr_to_segment_info.erase(addr);
            (void)erased;
            assert(erased);
            return true;
          }
        }

        static segment_group_id new_segment_group(multi_segment_services *services) {
          std::lock_guard lock(s_map);

          typedef typename segment_groups_t::iterator iterator;
          std::pair<iterator, bool> ret = s_groups.insert(segment_group_t(*services));
          assert(ret.second);
          return & *ret.first;
        }

        static bool delete_group(segment_group_id id) {

          std::lock_guard lock(s_map);

          bool success = 1u == s_groups.erase(segment_group_t(*id));
          if(success){
            typedef typename ptr_to_segment_info_t::iterator ptr_to_segment_info_it;
            ptr_to_segment_info_it it(s_map.m_ptr_to_segment_info.begin());
            while(it != s_map.m_ptr_to_segment_info.end()){
              if(it->second.group == id){
                it = s_map.m_ptr_to_segment_info.erase(it);
              }
              else{
                ++it;
              }
            }
          }
          return success;
        }
      };
    }

    class interprocess_mutex{};



    template <typename T>
    class portable_ptr : public impl::portable_ptr_flat_map<interprocess_mutex>{

    };*/



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


  namespace types{

    struct descriptor;

    enum flags_t : jem_u16_t {
      default_flags             = 0x0000,
      const_qualified_flag      = 0x0001,
      volatile_qualified_flag   = 0x0002,
      mutable_qualified_flag    = 0x0004,
      public_visibility_flag    = 0x0008,
      private_visibility_flag   = 0x0010,
      protected_visibility_flag = 0x0020,
      is_virtual_flag           = 0x0040,
      is_abstract_flag          = 0x0080,
      is_override_flag          = 0x0100,
      is_final_flag             = 0x0200,
      is_bitfield_flag          = 0x0400,
      is_noexcept_flag          = 0x0800,
      is_variadic_flag          = 0x1000
    };
    enum kind_t  : jem_u8_t {
      void_type,
      boolean_type,
      integral_type,
      floating_point_type,
      pointer_type,
      array_type,
      function_type,
      member_pointer_type,
      member_function_pointer_type,
      aggregate_type,
      enumerator_type
    };

    using  type_t = ipc::offset_ptr<const descriptor>;
    using  type_id_t = const descriptor*;
    struct type_id_ref_t{
      type_id_t type;
      flags_t flags;
    };
    struct aggregate_member_id_t{
      type_id_t        type;
      flags_t          flags;
      jem_u8_t         bitfield_bits;
      jem_u32_t        alignment;
      std::string_view name;
    };
    struct enumeration_id_t{
      std::string_view name;
      jem_u64_t        value;
    };


    static std::string_view allocate_string(std::string_view name) noexcept;
    static void* allocate_array(jem_size_t objectSize, jem_size_t objectCount) noexcept;
    template <typename T>
    inline static T* allocate_array(jem_size_t count) noexcept {
      return static_cast<T*>(allocate_array(sizeof(T), count));
    }
    template <typename T>
    static std::span<ipc::offset_ptr<T>> allocate_pointer_span(std::span<T*> pointers) noexcept;

    static void align_offset(jem_size_t& offset, jem_size_t alignment) noexcept {
      const jem_size_t lower_mask = alignment - 1;
      --offset;
      offset |= lower_mask;
      ++offset;
    }


    struct void_params_t{};
    struct boolean_params_t{};
    struct integral_params_t{
      std::string_view name;
      jem_u32_t        size;
      jem_u32_t        alignment;
      bool             is_signed;
    };
    struct floating_point_params_t{
      std::string_view name;
      jem_u32_t        size;
      jem_u32_t        alignment;
      bool             is_signed;
      jem_u16_t        exponent_bits;
      jem_u32_t        mantissa_bits;
    };
    struct pointer_params_t{
      type_id_ref_t pointee_type;
    };
    struct reference_params_t{
      type_id_ref_t pointee_type;
    };
    struct array_params_t{
      type_id_ref_t elements;
      jem_u32_t     count;
    };
    struct function_params_t{
      type_id_ref_t                  ret;
      std::span<const type_id_ref_t> args;
      flags_t                        flags;
    };
    struct member_pointer_params_t{
      type_id_ref_t pointee;
      type_id_t     aggregate_type;
    };
    struct member_function_pointer_params_t{
      type_id_ref_t function;
      type_id_t     aggregate_type;
    };
    struct aggregate_params_t{
      flags_t                                flags;
      jem_u32_t                              alignment;
      std::string_view                       name;
      std::span<const type_id_ref_t>         bases;
      std::span<const aggregate_member_id_t> members;
    };
    struct enumerator_params_t{
      std::string_view                  name;
      type_id_t                         underlying_type;
      std::span<const enumeration_id_t> entries;
      bool                              is_bitflag;
    };


    struct descriptor{

      inline static constexpr jem_u16_t QualifierMask  = jem_u16_t(const_qualified_flag | volatile_qualified_flag);
      inline static constexpr jem_u16_t VisibilityMask = jem_u16_t(public_visibility_flag | protected_visibility_flag | private_visibility_flag);

      using  type_t = ipc::offset_ptr<const descriptor>;
      struct type_ref_t{
        type_t  type  = nullptr;
        flags_t flags = default_flags;
      };
      struct aggregate_member_t{
        type_t           type;
        flags_t          flags;
        jem_u8_t         bitfield_bits;
        jem_u8_t         bitfield_offset;
        jem_u32_t        offset;
        ipc::string      name;
      };
      struct aggregate_base_t {
        type_t    type;
        flags_t   flags;
        jem_u32_t offset;
      };
      struct enumeration_t{
        ipc::string name;
        jem_u64_t   value;
      };

      kind_t     kind;
      jem_size_t size;
      jem_size_t alignment;

      union{
        char default_init = '\0';
        struct {
          ipc::string name;
          bool        is_signed;
        } integral;
        struct {
          ipc::string name;
          bool        is_signed;
          jem_u16_t   exponent_bits;
          jem_u32_t   mantissa_bits;
        } floating_point;
        struct {
          type_ref_t pointee;
        } pointer;
        struct {
          struct {
            type_t     type;
            flags_t    flags;
            bool       is_bounded;
            jem_u32_t  count;
          } element;
        } array;
        struct {
          type_ref_t                  ret;
          ipc::span<const type_ref_t> args;
          bool                        is_variadic;
          bool                        is_noexcept;
        } function;
        struct {
          type_ref_t pointee;
          type_t aggregate_type;
        } member_pointer;
        struct {
          type_ref_t function;
          type_t     aggregate_type;
        } member_function_pointer;
        struct {
          ipc::string                         name;
          ipc::span<const aggregate_base_t>   bases;
          ipc::span<const aggregate_member_t> members;
          bool                                is_polymorphic;
          bool                                is_abstract;
          bool                                is_final;
        } aggregate;
        struct {
          ipc::string                    name;
          type_t                         underlying_type;
          ipc::span<const enumeration_t> entries;
          bool                           is_bitflag;
        } enumerator;
      } mem = {};


      descriptor() = default;
      explicit descriptor(void_params_t) noexcept
          : kind(void_type), size(0), alignment(0){}
      explicit descriptor(boolean_params_t) noexcept
          : kind(boolean_type), size(1), alignment(1){}
      explicit descriptor(const integral_params_t& params) noexcept
          : kind(integral_type), size(params.size), alignment(params.alignment),
            mem{
              .integral = {
                .name = allocate_string(params.name),
                .is_signed = params.is_signed
              }
            }
      {}
      explicit descriptor(const floating_point_params_t& params) noexcept
          : kind(floating_point_type), size(params.size), alignment(params.alignment),
            mem{
              .floating_point = {
                  .name = allocate_string(params.name),
                  .is_signed = params.is_signed,
                  .exponent_bits = params.exponent_bits,
                  .mantissa_bits = params.mantissa_bits
              }
            }
      {}
      explicit descriptor(const pointer_params_t& params) noexcept
          : kind(pointer_type), size(sizeof(void*)), alignment(alignof(void*)),
            mem{
              .pointer = {
                .pointee = {
                  .type  = params.pointee_type.type,
                  .flags = params.pointee_type.flags
                }
              }
            }
      {}
      explicit descriptor(const array_params_t& params) noexcept
          : kind(array_type), size(params.count == 0 ? 0 : (params.elements.type->size * params.count)), alignment(params.elements.type->alignment),
            mem {
              .array = {
                .element = {
                  .type       = params.elements.type,
                  .flags      = params.elements.flags,
                  .is_bounded = params.count != 0,
                  .count      = params.count
                }
              }
            }
      {}
      explicit descriptor(const function_params_t& params) noexcept
          : kind(function_type), size(0), alignment(1),
            mem {
              .function = {
                .ret = {
                  .type  = params.ret.type,
                  .flags = params.ret.flags
                },
                .is_variadic = bool(params.flags & is_variadic_flag),
                .is_noexcept = bool(params.flags & is_noexcept_flag)
              }
            }
      {
        const jem_size_t argCount = params.args.size();
        auto* argsPtr = allocate_array<type_ref_t>(argCount);
        for ( jem_size_t i = 0; i < argCount; ++i ) {
          argsPtr[i].type  = params.args[i].type;
          argsPtr[i].flags = params.args[i].flags;
        }
        mem.function.args = { argsPtr, argCount };
      }
      explicit descriptor(const member_pointer_params_t& params) noexcept
          : kind(member_pointer_type), size(sizeof(type_t type_ref_t::*)), alignment(alignof(type_t type_ref_t::*)),
            mem {
              .member_pointer = {
                .pointee = {
                  .type  = params.pointee.type,
                  .flags = params.pointee.flags
                },
                .aggregate_type = params.aggregate_type
              }
            }
      {}
      explicit descriptor(const member_function_pointer_params_t& params) noexcept
          : kind(member_function_pointer_type), size(/*TODO: figure out how to get member function pointer size*/), alignment(std::min((jem_size_t)size, sizeof(void*))),
            mem {
              .member_function_pointer = {
                .function = {
                  .type  = params.function.type,
                  .flags = params.function.flags
                },
                .aggregate_type = params.aggregate_type
              }
            }
      {}
      explicit descriptor(const aggregate_params_t& params) noexcept
          : kind(aggregate_type), size(0), alignment(params.alignment),
            mem{
              .aggregate = {
                .name = params.name,
                .is_polymorphic = bool(params.flags & is_virtual_flag),
                .is_abstract    = bool(params.flags & is_abstract_flag),
                .is_final       = bool(params.flags & is_final_flag)
              }
            }
      {
        jem_size_t align  = 1;
        jem_size_t offset = 0;
        jem_size_t bitfield_offset = 0;


        const jem_size_t baseCount   = params.bases.size();
        const jem_size_t memberCount = params.members.size();
        auto* const      basePtr     = allocate_array<aggregate_base_t>(baseCount);
        auto* const      memberPtr   = allocate_array<aggregate_member_t>(memberCount);

        for ( jem_size_t i = 0; i < baseCount;   ++i ) {
          basePtr[i].type   = params.bases[i].type;
          basePtr[i].flags  = params.bases[i].flags;

          align_offset(offset, params.bases[i].type->alignment);

          basePtr[i].offset = offset;
          offset += params.bases[i].type->size;
          align = std::max(align, params.bases[i].type->alignment);
        }

        if ( mem.aggregate.is_polymorphic && offset == 0 ) {
          offset += sizeof(void*); // vptr
        }

        for ( jem_size_t i = 0; i < memberCount; ++i ) {
          const jem_size_t member_alignment = params.members[i].alignment ?: params.members[i].type->alignment;
          const jem_size_t type_bits        = params.members[i].type->size * CHAR_BIT;

          memberPtr[i].name            = allocate_string(params.members[i].name);
          memberPtr[i].type            = params.members[i].type;
          memberPtr[i].flags           = params.members[i].flags;

          if ( params.members[i].flags & is_bitfield_flag ) {
            memberPtr[i].bitfield_bits   = params.members[i].bitfield_bits;
            if ( bitfield_offset + memberPtr[i].bitfield_bits > type_bits ) {
              bitfield_offset = 0;
              align_offset(offset, member_alignment);
              memberPtr[i].offset = offset;
              offset += memberPtr[i].bitfield_bits / 8;
            }

            memberPtr[i].bitfield_offset = bitfield_offset;
            bitfield_offset += memberPtr[i].bitfield_bits;
          }
          else {
            align_offset(offset, member_alignment);
            memberPtr[i].offset = offset;
            offset += memberPtr[i].type->size;
            memberPtr[i].bitfield_bits = 0;
            memberPtr[i].bitfield_offset = 0;
            bitfield_offset = 0;
          }

        }

        mem.aggregate.bases   = { basePtr, baseCount };
        mem.aggregate.members = { memberPtr, memberCount };

      }
      explicit descriptor(const enumerator_params_t& params) noexcept
          : kind(enumerator_type), size(params.underlying_type->size), alignment(params.underlying_type->alignment),
            mem{
              .enumerator = {
                .name            = params.name,
                .underlying_type = params.underlying_type,
                .entries         = {},
                .is_bitflag      = params.is_bitflag
              }
            }
      {
        const jem_size_t entryCount = params.entries.size();
        auto* entryPtr = allocate_array<enumeration_t>(entryCount);

        for ( jem_size_t i = 0; i < entryCount; ++i ) {
          entryPtr[i].value = params.entries[i].value;
          entryPtr[i].name  = allocate_string(params.entries[i].name);
        }

        mem.enumerator.entries = { entryPtr, entryCount };
      }
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





    using message_type_id = jem_u16_t;

    struct message_type{

    };
    struct message_type_registry{

    };


    struct message_module{
      enum : jem_u8_t {
        little_endian,
        big_endian
      } endianness;
    };







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
