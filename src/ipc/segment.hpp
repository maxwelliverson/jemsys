//
// Created by Maxwell on 2021-06-14.
//

#ifndef JEMSYS_IPC_SEGMENT_INTERNAL_HPP
#define JEMSYS_IPC_SEGMENT_INTERNAL_HPP

#include "offset_ptr.hpp"

#include <concepts>

namespace ipc{

  /*template <typename A>
  concept segment_allocator_c = requires(A& a, void* p, const void* cp, jem_size_t size, jem_size_t align){
    { a.alloc(size) }                   noexcept -> std::same_as<void*>;
    { a.alloc(size, align) }            noexcept -> std::same_as<void*>;
    { a.realloc(p, size, size) }        noexcept -> std::same_as<void*>;
    { a.realloc(p, size, size, align) } noexcept -> std::same_as<void*>;
    { a.free(p, size) }                 noexcept;
    { a.free(p, size, align) }          noexcept;
  };*/

  enum class segment_kind{
    solo,
    leaf,
    branch,
    trunk
  };



  namespace impl{

    struct segment_header{
      jem_u32_t         magic;
      segment_kind      kind;
      anonymous_mapping nextMapping;
      anonymous_mapping mapping;
      jem_size_t        chunkSize; // Multiple of alignment
      jem_size_t        alignment;
      jem_size_t        maxCapacity;
      jem_size_t        currentCapacity;
      jem_size_t        cursorPos;
      char              name[MaxChunkNameLength];
      jem_u8_t          buffer[];
    };
    struct solo_segment_header{

    };
    struct leaf_segment_header{

    };
    struct branch_segment_header{

    };
    struct trunk_segment_header{

    };
  }



  class segment{
  public:
  private:
    impl::segment_header* header;
  };



}

#endif//JEMSYS_IPC_SEGMENT_INTERNAL_HPP
