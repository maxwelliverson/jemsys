//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_IPC_STRING_INTERNAL_HPP
#define JEMSYS_QUARTZ_IPC_STRING_INTERNAL_HPP

#include "offset_ptr.hpp"
#include "../mm.hpp"

namespace qtz::ipc{
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
}

#endif//JEMSYS_QUARTZ_IPC_STRING_INTERNAL_HPP
