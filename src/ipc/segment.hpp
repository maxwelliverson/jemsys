//
// Created by Maxwell on 2021-06-14.
//

#ifndef JEMSYS_IPC_SEGMENT_INTERNAL_HPP
#define JEMSYS_IPC_SEGMENT_INTERNAL_HPP

#include "span.hpp"
#include "file.hpp"
#include "process.hpp"

#include <concepts>
#include <atomic>

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

  enum class segment_kind {
    solo,
    leaf,
    branch,
    trunk
  };



  namespace impl{

    inline constexpr static jem_size_t MaxChunkNameLength = 256;


    enum segment_flags_t{
      null_segment         = 0x001,
      solo_segment         = 0x002,
      branch_segment       = 0x004,
      trunk_segment        = 0x008,
      can_relocate         = 0x010,
      linear_allocation    = 0x020,
      first_fit_allocation = 0x040,
      best_fit_allocation  = 0x080,
      stack_allocation     = 0x100,
      pool_allocation      = 0x200
    };

    struct segment_header_t{
      jem_u32_t            magic;
      segment_flags_t      flags;
      std::atomic_uint32_t refCount;

    };
    struct solo_segment_header_t{
      jem_u32_t            magic;
      segment_flags_t      flags;
      std::atomic_uint32_t refCount;
    };

    struct leaf_descriptor{
      native_handle_t handle;
      jem_size_t      size;
      jem_size_t      offset;
    };
    struct branch_segment_header_t{
      jem_u32_t            magic;
      segment_flags_t      flags;
      std::atomic_uint32_t refCount;
      jem_size_t           maxInPlaceCapacity;
      jem_size_t           currentCapacity;
      jem_size_t           cursorPos;
      jem_size_t           maxLeafCount;
      jem_size_t           leafCount;
      leaf_descriptor      leaves[];
    };
    struct process_segment_mapping{
      process::id       id;
      memory_access_t   access;
      segment_header_t* address;
      jem_size_t        addressSpaceConsumed;
      bool              canBeRelocated;
    };
    struct branch_descriptor{
      char                          name[MaxChunkNameLength];
      file_mapping                  mapping;
      span<process_segment_mapping> processMappings;
    };
    struct trunk_segment_header_t{
      jem_u32_t            magic;
      segment_flags_t      flags;
      std::atomic_uint32_t refCount;
      file_mapping         mapping;
      jem_u32_t            branchCount;
      branch_descriptor    branchDescriptors[];
    };

    struct local_segment_descriptor{
      file_mapping      mapping;
      memory_access_t   access;
      segment_header_t* address;
      jem_size_t        spaceConsumed;
      bool              canBeRelocated;
    };
    union local_segment_descriptor_slot{
      jem_u32_t                nextAvailableSlot;
      local_segment_descriptor descriptor;
    };

    struct local_segment_manager{
      jem_u32_t                      totalSlots;
      jem_u32_t                      availableSlots;
      jem_u32_t                      nextAvailableSlot;
      local_segment_descriptor_slot* slots;
    };


    struct process_descriptor{
      jem_u32_t              idHash;
      process                proc;
      local_segment_manager* localManager;
    };

    struct central_process_manager{
      jem_u32_t            tombstoneCount;
      jem_u32_t            bucketCount;
      jem_u32_t            entryCount;
      process_descriptor** descriptorHashTable;
    };

  }



  class segment{
  public:

    segment() = default;
    segment(const segment& other) noexcept;
    segment(segment&& other) noexcept;


    static segment open(std::string_view name) noexcept;






    explicit operator bool() const noexcept {
      return header;
    }



    JEM_nodiscard bool can_read() const noexcept {

    }
    JEM_nodiscard bool can_write() const noexcept {

    }

    JEM_nodiscard jem_size_t size() const noexcept {

    }


    JEM_nodiscard void* alloc(jem_size_t size) noexcept { }
    JEM_nodiscard void* alloc(jem_size_t size, jem_size_t align) noexcept { }
    JEM_nodiscard void* realloc(void* address, jem_size_t size) noexcept { }
    JEM_nodiscard void* realloc(void* address, jem_size_t size, jem_size_t align) noexcept { }
    void                free(void* address, jem_size_t size) noexcept { }


  private:
    native_handle_t         processHandle;
    impl::segment_header_t* header;
  };



}

#endif//JEMSYS_IPC_SEGMENT_INTERNAL_HPP
