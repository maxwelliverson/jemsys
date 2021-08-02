//
// Created by maxwe on 2021-07-03.
//

#ifndef JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP
#define JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP

#include <quartz/core.h>
#include <agate/core.h>

#include "atomicutils.hpp"

namespace qtz{

  using pid_t = int;
  using tid_t = int;


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
    pid_t   srcProcess;
  };

  struct attach_process_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    uintptr_t         inboxAddress;
    std::atomic_flag* isReady;
    uintptr_t*        kernelInboxAddress;
    jem_size_t        nameLength;
    char              name[];
  };
  struct attach_thread_args {
    knl_cmd    cmd;
    pid_t      srcProcessId;
    tid_t      threadId;
    jem_size_t nameLength;
    char       name[];
  };
  struct attach_agt_handle_args {
    knl_cmd      cmd;
    pid_t        srcProcessId;
    agt_handle_t handle;
    jem_size_t   nameLength;
    char         name[];
  };
  struct detach_process_args {
    knl_cmd    cmd;
    pid_t      srcProcessId;
  };
  struct detach_thread_args {
    knl_cmd    cmd;
    pid_t      srcProcessId;
    tid_t      threadId;
  };
  struct detach_agt_handle_args {
    knl_cmd      cmd;
    pid_t        srcProcessId;
    agt_handle_t handle;
  };
  struct open_address_space_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    std::atomic_flag* isReady;
    uintptr_t*        resultAddress;
    jem_size_t        addressSpaceSize;
    jem_size_t        nameLength;
    char              name[];
  };
  struct close_address_space_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    uintptr_t         addressSpace;
  };
  struct alloc_within_address_space_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    uintptr_t         addressSpace;
    std::atomic_flag* isReady;
    uintptr_t*        resultAddress;
    jem_size_t        size;
    jem_size_t        alignment;
    jem_ptrdiff_t     alignOffset;
  };
  struct alloc_thread_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct alloc_agt_handle_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct alloc_process_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    std::atomic_flag* isReady;
    jem_u32_t*        resultKey;
    jem_size_t        size;
  };
  struct free_within_address_space_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    uintptr_t*        address;
    jem_size_t        size;
  };
  struct free_thread_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };
  struct free_agt_handle_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };
  struct free_process_local_args {
    knl_cmd           cmd;
    pid_t             srcProcessId;
    jem_u32_t         key;
    jem_size_t        size;
  };

  struct enumerate_processes_args {};
  struct enumerate_handles_args {};


  jem_size_t encode_params(const attach_process_args& args, char* argBuffer, jem_size_t argBufferLength) noexcept;

}

#endif//JEMSYS_QUARTZ_KERNEL_INTERNAL_HPP
