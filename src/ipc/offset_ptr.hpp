//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_IPC_OFFSET_PTR_INTERNAL_HPP
#define JEMSYS_QUARTZ_IPC_OFFSET_PTR_INTERNAL_HPP

//#define JEM_SHARED_LIB_EXPORT

#include <jemsys.h>

#include <type_traits>
#include <ranges>
#include <concepts>
#include <utility>
#include <iterator>

namespace ipc{
  template <typename T>
  class offset_ptr;

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
}

#endif//JEMSYS_QUARTZ_IPC_OFFSET_PTR_INTERNAL_HPP
