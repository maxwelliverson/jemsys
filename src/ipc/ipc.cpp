//
// Created by Maxwell on 2021-06-14.
//

#include "ipc/file.hpp"
#include "ipc/segment.hpp"
#include "ipc/process.hpp"
#include "ipc/memory.hpp"
#include "ipc/mpsc_mailbox.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#pragma comment(lib, "mincore")


/*std::optional<ipc::process> ipc::process::create(const char* applicationName, std::string_view commandLineOpts, void* procSecurityDesc, void* threadSecurityDesc, bool inherit, const environment<char>& env) noexcept {
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
}*/


namespace {
class this_process_class_ : public ipc::process {
public:
  this_process_class_() noexcept : ipc::process(){
    this->handle_ = GetCurrentProcess();
    this->id_     = GetCurrentProcessId();
    this->permissions_ = ipc::all_process_permissions;
  }
};
inline this_process_class_ this_process_{};
}

ipc::process& ipc::this_process = this_process_;



void* ipc::process::reserve_memory(jem_size_t size, jem_size_t alignment) const noexcept {
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
bool  ipc::process::release_memory(void* placeholder) const noexcept {
  return VirtualFreeEx(handle_, placeholder, 0, MEM_RELEASE);
}
void* ipc::process::local_alloc(void* address, jem_size_t size, ipc::memory_access_t access, ipc::numa_node_t numaNode) const noexcept {
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
bool  ipc::process::local_free(void* allocation) const noexcept {
  return VirtualFreeEx(handle_, allocation, 0, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
}
void* ipc::process::map(void* address, const ipc::file_mapping& fileMapping, ipc::memory_access_t access, ipc::numa_node_t numaNode) const noexcept {
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
bool  ipc::process::unmap(void* address) const noexcept {
  return UnmapViewOfFile2(handle_, address, MEM_PRESERVE_PLACEHOLDER);
}
bool  ipc::process::split(void* placeholder, jem_size_t firstSize) const noexcept {
  return VirtualFreeEx(handle_, placeholder, firstSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
}
bool  ipc::process::coalesce(void* placeholder, jem_size_t totalSize) const noexcept {
  return VirtualFreeEx(handle_, placeholder, totalSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
}
bool  ipc::process::prefetch(void* address, jem_size_t size) const noexcept {
  WIN32_MEMORY_RANGE_ENTRY range{
    .VirtualAddress = address,
    .NumberOfBytes  = size
  };
  return PrefetchVirtualMemory(handle_, 1, &range, 0);
}
bool  ipc::process::prefetch(std::span<const address_range> addressRanges) const noexcept {
  return PrefetchVirtualMemory(handle_, addressRanges.size(), (PWIN32_MEMORY_RANGE_ENTRY)addressRanges.data(), 0);
}



jem_size_t ipc::process::read(const void* srcAddr, void* dstAddr, jem_size_t bytesToRead) const noexcept {
  jem_size_t bytesWritten;
  if ( !ReadProcessMemory(handle_, srcAddr, dstAddr, bytesToRead, &bytesWritten) )
    return 0;
  return bytesWritten;
}
jem_size_t ipc::process::write(void* dstAddr, const void* srcAddr, jem_size_t bytesToWrite) const noexcept {
  jem_size_t bytesWritten;
  if ( !WriteProcessMemory(handle_, dstAddr, srcAddr, bytesToWrite, &bytesWritten) )
    return 0;
  return bytesWritten;
}


std::optional<ipc::process> ipc::process::open(ipc::process::id processId, ipc::process_permissions_t permissions) noexcept {
  process proc{};
  HANDLE handle = OpenProcess(permissions, true, processId);
  if ( handle ) {
    proc.handle_      = handle;
    proc.id_          = processId;
    proc.permissions_ = permissions;
    return proc;
  }
  return std::nullopt;
}
std::optional<ipc::process> ipc::process::launch(const char* applicationName, char* commandLineOpts, std::span<process_extra_arg> extraArgs) {


  std::unique_ptr<char[]> envBuffer = nullptr;

  LPSECURITY_ATTRIBUTES procAttr   = nullptr;
  LPSECURITY_ATTRIBUTES threadAttr = nullptr;
  bool        inheritHandles       = true;
  void*       environment          = nullptr;
  const char* currentDirectory     = nullptr;
  void**      pThreadHandle        = nullptr;
  int*        pThreadId            = nullptr;

  PROCESS_INFORMATION procInfo;

  for ( auto arg : extraArgs ) {
    switch ( arg.kind ) {
      case process_arg_process_security:
        procAttr = reinterpret_cast<LPSECURITY_ATTRIBUTES>(arg.value);
        break;
      case process_arg_thread_security:
        threadAttr = reinterpret_cast<LPSECURITY_ATTRIBUTES>(arg.value);
        break;
      case process_arg_inherit_handle:
        inheritHandles = arg.value;
        break;
      case process_arg_environment:
        envBuffer = reinterpret_cast<ipc::environment<char>*>(arg.value)->get_normalized_buffer();
        environment = envBuffer.get();
        break;
      case process_arg_current_directory:
        currentDirectory = reinterpret_cast<const char*>(arg.value);
        break;
      case process_arg_thread_id:
        pThreadId = reinterpret_cast<int*>(arg.value);
        break;
      case process_arg_thread_handle:
        pThreadHandle = reinterpret_cast<void**>(arg.value);
        break;
    }
  }

  if (!CreateProcess(applicationName,
                commandLineOpts,
                procAttr,
                threadAttr,
                inheritHandles,
                DETACHED_PROCESS,
                environment,
                currentDirectory,
                nullptr,
                &procInfo)) {
    return std::nullopt;
  }

  process proc;

  proc.handle_ = procInfo.hProcess;
  proc.id_     = procInfo.dwProcessId;
  proc.permissions_ = static_cast<process_permissions_t>(PROCESS_ALL_ACCESS);

  if ( pThreadHandle != nullptr ) {
    *pThreadHandle = procInfo.hThread;
  }
  if ( pThreadId != nullptr ) {
    *pThreadId = procInfo.dwThreadId;
  }

  return proc;
}
std::optional<ipc::process> ipc::process::launch_child(const char* applicationName, char* commandLineOpts, std::span<process_extra_arg> extraArgs) {

  std::unique_ptr<char[]> envBuffer = nullptr;

  LPSECURITY_ATTRIBUTES procAttr   = nullptr;
  LPSECURITY_ATTRIBUTES threadAttr = nullptr;
  bool        inheritHandles       = true;
  void*       environment          = nullptr;
  const char* currentDirectory     = nullptr;
  void**      pThreadHandle        = nullptr;
  int*        pThreadId            = nullptr;

  PROCESS_INFORMATION procInfo;

  for ( auto arg : extraArgs ) {
    switch ( arg.kind ) {
      case process_arg_process_security:
        procAttr = reinterpret_cast<LPSECURITY_ATTRIBUTES>(arg.value);
        break;
        case process_arg_thread_security:
          threadAttr = reinterpret_cast<LPSECURITY_ATTRIBUTES>(arg.value);
          break;
          case process_arg_inherit_handle:
            inheritHandles = arg.value;
            break;
            case process_arg_environment:
              envBuffer = reinterpret_cast<ipc::environment<char>*>(arg.value)->get_normalized_buffer();
              environment = envBuffer.get();
              break;
              case process_arg_current_directory:
                currentDirectory = reinterpret_cast<const char*>(arg.value);
                break;
                case process_arg_thread_id:
                  pThreadId = reinterpret_cast<int*>(arg.value);
                  break;
                  case process_arg_thread_handle:
                    pThreadHandle = reinterpret_cast<void**>(arg.value);
                    break;
    }
  }

  if (!CreateProcess(applicationName,
                     commandLineOpts,
                     procAttr,
                     threadAttr,
                     inheritHandles,
                     0,
                     environment,
                     currentDirectory,
                     nullptr,
                     &procInfo)) {
    return std::nullopt;
  }

  process proc;

  proc.handle_ = procInfo.hProcess;
  proc.id_     = procInfo.dwProcessId;
  proc.permissions_ = static_cast<process_permissions_t>(PROCESS_ALL_ACCESS);

  if ( pThreadHandle != nullptr ) {
    *pThreadHandle = procInfo.hThread;
  }
  if ( pThreadId != nullptr ) {
    *pThreadId = procInfo.dwThreadId;
  }

  return proc;
}