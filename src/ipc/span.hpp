//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_IPC_SPAN_INTERNAL_HPP
#define JEMSYS_QUARTZ_IPC_SPAN_INTERNAL_HPP

#include "offset_ptr.hpp"

#include <span>

namespace ipc{

  template <typename T, size_t Extent = std::dynamic_extent>
  class span;

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
      using pointer          = T*;
      using const_pointer    = const T*;
      using reference        = T&;
      using const_reference  = const T&;
      using iterator         = pointer;
      using reverse_iterator = std::reverse_iterator<pointer>;




      pointer          data() const noexcept {
        return ptr.get();
      }

      reference        front() const noexcept {
        return *ptr;
      }

      reference        operator[](size_t index) const noexcept {
        return ptr[index];
      }

      iterator         begin() const noexcept {
        return ptr.get();
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

    template <typename Rng, typename T>
    concept span_of = std::ranges::contiguous_range<Rng> &&
      std::ranges::sized_range<Rng> &&
      (std::ranges::borrowed_range<Rng> || std::is_const_v<T>) &&
      is_not_span_or_array<Rng>::value &&
      qualified_convertible_to<std::ranges::range_value_t<Rng>, T>;
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

          template <impl::span_of<T> Rng>
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
}

#endif//JEMSYS_QUARTZ_IPC_SPAN_INTERNAL_HPP
