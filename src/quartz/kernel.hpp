//
// Created by maxwe on 2021-07-03.
//

#ifndef JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP
#define JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP

#include <agate/core.h>
#include <quartz.h>

#include "atomicutils.hpp"
#include "internal.hpp"

namespace qtz{




  enum class knl_cmd{
    attach_process,
    attach_thread,
    attach_agt_handle,
    detach_process,
    detach_thread,
    detach_agt_handle,
    open_address_space,
    close_address_space,
    alloc_within_address_space,
    alloc_thread_local,
    alloc_agt_handle_local,
    alloc_process_local,
    free_within_address_space,
    free_thread_local,
    free_agt_handle_local,
    free_process_local,

    enumerate_processes,
    enumerate_handles
  };

  struct args{
    knl_cmd cmd;
    qtz_pid_t   srcProcess;
  };

  struct init_kernel_args {
    const char* processName;
    void*       processInboxFileMapping;
    jem_size_t  processInboxBytes;
    qtz_pid_t       srcProcessId;
    jem_u32_t   kernelVersion;
    const char* kernelIdentifier;
    const char* kernelBlockName;
    jem_size_t  kernelBlockSize;
  };

  struct attach_process_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    uintptr_t         inboxAddress;
    std::atomic_flag* isReady;
    uintptr_t*        kernelInboxAddress;
    jem_size_t        nameLength;
    char              name[];
  };
  struct attach_thread_args {
    knl_cmd    cmd;
    qtz_pid_t      srcProcessId;
    qtz_tid_t      threadId;
    jem_size_t nameLength;
    char       name[];
  };
  struct attach_agt_handle_args {
    knl_cmd      cmd;
    qtz_pid_t        srcProcessId;
    agt_handle_t handle;
    jem_size_t   nameLength;
    char         name[];
  };
  struct detach_process_args {
    knl_cmd    cmd;
    qtz_pid_t      srcProcessId;
  };
  struct detach_thread_args {
    knl_cmd   cmd;
    qtz_pid_t srcProcessId;
    qtz_tid_t threadId;
  };
  struct detach_agt_handle_args {
    knl_cmd      cmd;
    qtz_pid_t    srcProcessId;
    agt_handle_t handle;
  };
  struct open_address_space_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    std::atomic_flag* isReady;
    uintptr_t*        resultAddress;
    jem_size_t        addressSpaceSize;
    jem_size_t        nameLength;
    char              name[];
  };
  struct close_address_space_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    uintptr_t         addressSpace;
  };
  struct alloc_within_address_space_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    uintptr_t         addressSpace;
    std::atomic_flag* isReady;
    uintptr_t*        resultAddress;
    jem_size_t        size;
    jem_size_t        alignment;
    jem_ptrdiff_t     alignOffset;
  };
  struct alloc_thread_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct alloc_agt_handle_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct alloc_process_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct free_within_address_space_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    uintptr_t*        address;
    jem_size_t        size;
  };
  struct free_thread_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };
  struct free_agt_handle_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };
  struct free_process_local_args {
    knl_cmd           cmd;
    qtz_pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };

  struct enumerate_processes_args {};
  struct enumerate_handles_args {};

  enum {
    kernel_is_uninitialized,
    kernel_is_initialized,
    kernel_process_launch_failed,
    kernel_handshake_failed,
    kernel_process_exited
  };

  struct kernel_control_block{
    atomic_u32_t state;
    jem_size_t   controlBlockSize;
    jem_u32_t    version;
    atomic_u32_t attachedProcessCount;
    bool         exitWhenProcessCountIsZero;

    JEM_cache_aligned
    char         identifier[32];
    char         mailboxWriteMutexName[32];
  };

  extern kernel_control_block* g_kernelBlock;



  char*     encode_params(const init_kernel_args& args) noexcept;
  jem_u32_t decode_params(init_kernel_args& args, int argc, char** argv) noexcept;
  void      free_params(char* params) noexcept;

  jem_size_t encode_params(const attach_process_args& args, char* argBuffer, jem_size_t argBufferLength) noexcept;

  jem_size_t encode_params(const init_kernel_args& args, char* argBuffer, jem_size_t argBufferLength) noexcept;


}

#endif//JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP
