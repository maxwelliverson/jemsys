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


std::optional<qtz::process> qtz::process::create(const char* applicationName, std::string_view commandLineOpts, void* procSecurityDesc, void* threadSecurityDesc, bool inherit, const environment<char>& env) noexcept {
  char commandLine[1 << 14] = {};
  PROCESS_INFORMATION processInfo;
  SECURITY_DESCRIPTOR processSecDesc, threadSecDesc;
  InitializeSecurityDescriptor(&processSecDesc, SECURITY_DESCRIPTOR_REVISION);
  SECURITY_ATTRIBUTES processAttributes{
    .nLength = sizeof(SECURITY_ATTRIBUTES),
    .lpSecurityDescriptor = procSecurityDesc,
    .bInheritHandle = true
  };
  SECURITY_ATTRIBUTES threadAttributes{
    .nLength = sizeof(SECURITY_ATTRIBUTES),
    .lpSecurityDescriptor = threadSecurityDesc,
    .bInheritHandle = true
  };

  memcpy(commandLine, commandLineOpts.data(), commandLineOpts.length());
  auto environmentBlock{env.get_normalized_buffer()};

  if (!CreateProcessA(applicationName, commandLine, &processAttributes, &threadAttributes, inherit, DETACHED_PROCESS, environmentBlock.get(), ))
    return std::nullopt;
}
void* qtz::process::reserve_memory(jem_size_t size, jem_size_t alignment) const noexcept {
  if ( alignment == JEM_DONT_CARE ) {
    return VirtualAlloc2(handle_, nullptr, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
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
    return VirtualAlloc2(handle_, nullptr, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, &alignParam, 1);
  }
}
bool qtz::process::release_memory(void* placeholder) const noexcept {
  return VirtualFreeEx(handle_, placeholder, 0, MEM_RELEASE);
}
void* qtz::process::local_alloc(void* address, jem_size_t size, qtz::memory_access_t access, qtz::numa_node_t numaNode) const noexcept {
  if ( numaNode == JEM_CURRENT_NUMA_NODE ) {
    return VirtualAlloc2(handle_, address, size, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, access, nullptr, 0);
  }
  else {
    MEM_EXTENDED_PARAMETER numaNodeParam{
      .Type = MemExtendedParameterNumaNode,
      .ULong = numaNode
    };
    return VirtualAlloc2(handle_, address, size, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, access, &numaNodeParam, 1);
  }
}
bool qtz::process::local_free(void* allocation, jem_size_t size) const noexcept {
  return VirtualFreeEx(handle_, allocation, 0, MEM_RELEASE);
}
void* qtz::process::map(void* address, const qtz::file_mapping& fileMapping, qtz::memory_access_t access, qtz::numa_node_t numaNode) const noexcept {
  if ( numaNode == JEM_CURRENT_NUMA_NODE ) {
    return MapViewOfFile3(fileMapping.handle, handle_, address, 0, fileMapping.size, MEM_REPLACE_PLACEHOLDER, access, nullptr, 0);
  }
  else {
    MEM_EXTENDED_PARAMETER numaNodeParam{
      .Type = MemExtendedParameterNumaNode,
      .ULong = numaNode
    };
    return MapViewOfFile3(fileMapping.handle, handle_, address, 0, fileMapping.size, MEM_REPLACE_PLACEHOLDER, access, &numaNodeParam, 1);
  }
}
bool qtz::process::unmap(void* address) const noexcept {
  return UnmapViewOfFile2(handle_, address, MEM_PRESERVE_PLACEHOLDER);
}
bool qtz::process::split(void* placeholder, jem_size_t firstSize) const noexcept {
  return VirtualFreeEx(handle_, placeholder, firstSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
}
bool qtz::process::coalesce(void* placeholder, jem_size_t totalSize) const noexcept {
  return VirtualFreeEx(handle_, placeholder, totalSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
}


std::optional<qtz::process> qtz::process::open(qtz::process::id processId, qtz::process_permissions_t permissions) noexcept {
  HANDLE handle = OpenProcess(permissions, true, processId.id_);
  if ( !handle )
    return std::nullopt;
  process proc{};
  proc.handle_ = handle;
  proc.id_ = processId;
  proc.permissions_ = permissions;
  return proc;
}


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
  return VirtualFree(placeholder, firstSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
}
bool qtz::virtual_placeholder::coalesce(void* placeholder, jem_size_t totalSize) noexcept {
  return VirtualFree(placeholder, totalSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
}




JEM_stdcall qtz_request_t   qtz_alloc_static_mailbox(void **pResultAddr, jem_u32_t messageSlots, jem_u32_t messageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_static_mailbox(void *mailboxAddr);
JEM_stdcall qtz_request_t   qtz_alloc_dynamic_mailbox(void **pResultAddr, jem_u32_t minMailboxSize, jem_u32_t maxMessageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_dynamic_mailbox(void *mailboxAddr);

