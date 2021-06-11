//
// Created by maxwe on 2021-06-01.
//

#include "internal.h"

#include "quartz/mm.h"
#include "mm.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#pragma comment(lib, "mincore")





void* qtz::virtual_placeholder::reserve(jem_size_t size, jem_size_t alignment) noexcept {
  if ( alignment == JEM_DONT_CARE ) {
    return VirtualAlloc2(nullptr, nullptr, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
  }
  else {
    MEM_ADDRESS_REQUIREMENTS addressRequirements{
      .LowestStartingAddress = nullptr,
      .HighestEndingAddress  = nullptr,
      .Alignment = alignment
    };
    MEM_EXTENDED_PARAMETER alignParam{
      .Type    = MemExtendedParameterAddressRequirements,
      .Pointer = &addressRequirements
    };
    return VirtualAlloc2(nullptr, nullptr, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, &alignParam, 1);
  }
}
bool  qtz::virtual_placeholder::release(void* placeholder) noexcept {
  return VirtualFree(placeholder, 0, MEM_RELEASE);
}

void* qtz::virtual_placeholder::alloc(void* placeholder, jem_size_t size, memory_access_t access, numa_node_t numaNode) noexcept {
  if ( numaNode == JEM_CURRENT_NUMA_NODE ) {
    return VirtualAlloc2(nullptr, placeholder, size, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, access, nullptr, 0);
  }
  else {
    MEM_EXTENDED_PARAMETER numaNodeParam{
      .Type = MemExtendedParameterNumaNode,
      .ULong = numaNode
    };
    return VirtualAlloc2(nullptr, placeholder, size, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, access, &numaNodeParam, 1);
  }
}
bool  qtz::virtual_placeholder::free(void* allocation, jem_size_t size) noexcept {
  return VirtualFree(allocation, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
}


void* qtz::virtual_placeholder::map(void* placeholder, const file_mapping& fileMapping, memory_access_t access, numa_node_t numaNode) noexcept {
  if ( numaNode == JEM_CURRENT_NUMA_NODE ) {
    return MapViewOfFile3(fileMapping.handle, nullptr, placeholder, 0, fileMapping.size, MEM_REPLACE_PLACEHOLDER, access, nullptr, 0);
  }
  else {
    MEM_EXTENDED_PARAMETER numaNodeParam{
      .Type = MemExtendedParameterNumaNode,
      .ULong = numaNode
    };
    return MapViewOfFile3(fileMapping.handle, nullptr, placeholder, 0, fileMapping.size, MEM_REPLACE_PLACEHOLDER, access, &numaNodeParam, 1);
  }
}
bool  qtz::virtual_placeholder::unmap(void* placeholder) noexcept {
  return UnmapViewOfFile2(nullptr, placeholder, MEM_PRESERVE_PLACEHOLDER);
}

bool qtz::virtual_placeholder::split(void* placeholder, jem_size_t firstSize) noexcept {
  return VirtualFree();
}
bool qtz::virtual_placeholder::coalesce(void* placeholder, jem_size_t totalSize) noexcept {}




JEM_stdcall qtz_request_t   qtz_alloc_static_mailbox(void **pResultAddr, jem_u32_t messageSlots, jem_u32_t messageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_static_mailbox(void *mailboxAddr);
JEM_stdcall qtz_request_t   qtz_alloc_dynamic_mailbox(void **pResultAddr, jem_u32_t minMailboxSize, jem_u32_t maxMessageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_dynamic_mailbox(void *mailboxAddr);

