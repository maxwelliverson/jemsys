//
// Created by maxwe on 2021-06-07.
//

#ifndef JEMSYS_QUARTZ_INIT_H
#define JEMSYS_QUARTZ_INIT_H

#include "core.h"

JEM_begin_c_namespace

typedef struct {
  jem_u64_t messageSlotCount;
} qtz_init_params_t;

JEM_api jem_status_t  JEM_stdcall qtz_init(const qtz_init_params_t* params);
JEM_api qtz_request_t JEM_stdcall qtz_attach_thread(qtz_deputy_t* pDeputyHandle);
JEM_api void          JEM_stdcall qtz_detach_thread();

JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_INIT_H
