//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_CORE_H
#define JEMSYS_QUARTZ_CORE_H

#include "jemsys.h"

JEM_begin_c_namespace

typedef jem_u32_t                  qtz_local_id_t;
typedef jem_u64_t                  qtz_global_id_t;

typedef struct qtz_request*        qtz_request_t;
typedef struct qtz_process*        qtz_process_t;

typedef struct qtz_module*         qtz_module_t;


typedef enum {
  QTZ_SUCCESS,
  QTZ_ERROR_INTERNAL,
  QTZ_ERROR_UNKNOWN,
  QTZ_ERROR_BAD_SIZE,
  QTZ_ERROR_INVALID_ARGUMENT,
  QTZ_ERROR_BAD_ENCODING_IN_NAME,
  QTZ_ERROR_NAME_TOO_LONG,
  QTZ_ERROR_INSUFFICIENT_BUFFER_SIZE,
  QTZ_ERROR_BAD_ALLOC,
  QTZ_ERROR_TOO_MANY_PRODUCERS,
  QTZ_ERROR_TOO_MANY_CONSUMERS
} qtz_status_t;
typedef enum {
  QTZ_KERNEL_INIT_OPEN_ALWAYS,
  QTZ_KERNEL_INIT_CREATE_NEW,
  QTZ_KERNEL_INIT_OPEN_EXISTING
} qtz_kernel_init_mode_t;


typedef struct {
  jem_u32_t              kernel_version;
  qtz_kernel_init_mode_t kernel_mode;
  const char*            kernel_access_code;
  jem_size_t             message_slot_count;
  const char*            process_name;
  jem_size_t             module_count;
  const char* const *    modules;
} qtz_init_params_t;


extern const jem_u32_t qtz_page_size;



JEM_api qtz_status_t JEM_stdcall qtz_init(const qtz_init_params_t* params);


JEM_api qtz_status_t JEM_stdcall qtz_request_status(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_request_wait(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_request_discard(qtz_request_t message);



JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_CORE_H
