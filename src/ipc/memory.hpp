//
// Created by Maxwell on 2021-06-14.
//

#ifndef JEMSYS_IPC_MEMORY_INTERNAL_HPP
#define JEMSYS_IPC_MEMORY_INTERNAL_HPP

#include "file.hpp"


#include <span>

namespace ipc{

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


  struct anonymous_file_mapping{
    native_handle_t handle;
    jem_size_t      size;
  };

  struct page_map{
    jem_u32_t  hugePageCount;
    struct {}* hugePages;
  };
  struct thread_local_segment{
    jem_size_t totalSize;
    void*      address;
    bool       locallyAllocated;

  };
  struct local_heap;
  struct thread_group{
    jem_size_t  groupSize;
    local_heap* heaps;
  };
  struct local_heap{
    jem_u32_t             threadId;
    thread_local_segment* segments;
    jem_size_t            totalAllocated;
    local_heap*           parent;
    jem_size_t            childCount;
    local_heap**          children;
    thread_group*         group;
  };
  struct shared_heap{
    jem_size_t   threadCount;
    local_heap** threadHeaps;
  };
  struct memory_manager{
    jem_size_t addressSpaceSize;

  };


  template <typename A>
  concept ipc_allocator_c = requires(A& a, void* p, const void* cp, jem_size_t size, jem_size_t align){
    { a.allocate(size) }                   noexcept -> std::convertible_to<void*>;
    { a.allocate(size, align) }            noexcept -> std::convertible_to<void*>;
    { a.reallocate(p, size, size) }        noexcept -> std::convertible_to<void*>;
    { a.reallocate(p, size, size, align) } noexcept -> std::convertible_to<void*>;
  };



  class extensible_segment {
    inline constexpr static jem_size_t MagicNumber        = 0xFA81'1B0C'CAC0'182C;
    friend class extensible_segment_ref;
    inline constexpr static jem_size_t MaxChunkNameLength = 256;
  public:

    extensible_segment(jem_address_t address);

  private:

    void* grow_by(jem_size_t size) noexcept {
      void* pos = reinterpret_cast<char*>(header) + header->cursorPos;
      const uintptr_t subheaderMask = ~(header->alignment - 1ULL);
      auto* subheader = reinterpret_cast<subheader_t*>(reinterpret_cast<uintptr_t>(pos) & subheaderMask);

      if ( pos == static_cast<void*>(subheader)) {

      }

      if ( header->cursorPos + size > header->currentCapacity ) {

      }
    }


    inline constexpr static jem_size_t MaxChunkCount         = 128;
    inline constexpr static jem_size_t GrowthRateNumerator   = 7;
    inline constexpr static jem_size_t GrowthRateDenominator = 4;


    struct header_t{
      jem_u64_t              magic;
      anonymous_file_mapping nextMapping;
      anonymous_file_mapping mapping;
      jem_size_t             chunkSize; // Multiple of alignment
      jem_size_t             alignment;
      jem_size_t             maxCapacity;
      jem_size_t             currentCapacity;
      jem_size_t             cursorPos;
      char                   name[MaxChunkNameLength];
      jem_u8_t               buffer[];
    };
    struct subheader_t{
      jem_u64_t       magic;
      jem_u32_t       waymarker;
      native_handle_t nextFileHandle;
      jem_size_t      nextFileSize;
      jem_u8_t        buffer[];
    };

    header_t*  header;    // Address is aligned to alignment
  };

  class extensible_segment_ref {
  public:
    extensible_segment_ref(const extensible_segment& segment) noexcept
    : initialSegmentFileMapping(segment.header->mapping),
    maximumSize(segment.header->maxCapacity)
    {}
  private:
    anonymous_file_mapping initialSegmentFileMapping;
    jem_size_t             maximumSize;
  };


}

#endif//JEMSYS_IPC_MEMORY_INTERNAL_HPP
