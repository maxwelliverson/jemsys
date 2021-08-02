//
// Created by maxwe on 2021-06-01.
//

#include "internal.h"

#include <quartz/init.h>


namespace {
  inline atomic_flag_t isInitialized{};

  void* globalMailboxThreadHandle;
  int   globalMailboxThreadId;
}

extern "C" {
  JEM_api jem_status_t  JEM_stdcall qtz_init(const qtz_init_params_t* params) {

  }
}