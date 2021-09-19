//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_CORE_H
#define JEMSYS_QUARTZ_CORE_H

#include "jemsys.h"

JEM_begin_c_namespace


#define QTZ_KERNEL_VERSION_MOST_RECENT ((jem_u32_t)-1)
#define QTZ_DEFAULT_MODULE ((qtz_module_t)nullptr)
#define QTZ_GLOBAL_MODULE_ID ((qtz_local_id_t)0)

typedef jem_u32_t                  qtz_local_id_t;
typedef jem_u64_t                  qtz_global_id_t;

typedef struct qtz_request*        qtz_request_t;
typedef struct qtz_process*        qtz_process_t;
typedef struct qtz_error*          qtz_error_t;
typedef struct qtz_module*         qtz_module_t;


typedef enum {
  QTZ_SUCCESS,
  QTZ_NOT_READY,
  QTZ_DISCARDED,
  QTZ_ERROR_TIMED_OUT,
  QTZ_ERROR_INTERNAL,
  QTZ_ERROR_UNKNOWN,
  QTZ_ERROR_BAD_SIZE,
  QTZ_ERROR_INVALID_ARGUMENT,
  QTZ_ERROR_BAD_ENCODING_IN_NAME,
  QTZ_ERROR_NAME_TOO_LONG,
  QTZ_ERROR_NAME_ALREADY_IN_USE,
  QTZ_ERROR_INSUFFICIENT_BUFFER_SIZE,
  QTZ_ERROR_BAD_ALLOC,
  QTZ_ERROR_TOO_MANY_PRODUCERS,
  QTZ_ERROR_TOO_MANY_CONSUMERS,
  QTZ_ERROR_NOT_IMPLEMENTED,
  QTZ_ERROR_UNSUPPORTED_OPERATION,
  QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE
} qtz_status_t;
typedef enum {
  QTZ_KERNEL_INIT_OPEN_ALWAYS,
  QTZ_KERNEL_INIT_CREATE_NEW,
  QTZ_KERNEL_INIT_OPEN_EXISTING
} qtz_kernel_init_mode_t;
typedef enum {
  QTZ_SEND_MUST_CHECK_RESULT = 0x100,
  QTZ_SEND_CANCEL_ON_DISCARD = 0x200,
  QTZ_SEND_IGNORE_ERRORS     = 0x400
} qtz_send_flag_bits_t;

typedef jem_flags32_t qtz_send_flags_t;


typedef struct {
  jem_u32_t              kernel_version;
  qtz_kernel_init_mode_t kernel_mode;
  const char*            kernel_access_code;
  jem_size_t             message_slot_count;
  const char*            process_name;
  jem_size_t             module_count;
  const void*            modules;
} qtz_init_params_t;






JEM_api qtz_status_t  JEM_stdcall qtz_init(const qtz_init_params_t* params);

JEM_api qtz_status_t  JEM_stdcall qtz_request_status(qtz_request_t message);
JEM_api qtz_status_t  JEM_stdcall qtz_request_wait(qtz_request_t message, jem_u64_t timeout_us);
JEM_api void          JEM_stdcall qtz_request_discard(qtz_request_t message);

JEM_api qtz_request_t JEM_stdcall qtz_send(qtz_local_id_t messageId, const void* messageBuffer) JEM_noexcept;
JEM_api qtz_request_t JEM_stdcall qtz_send_ex(qtz_process_t process, qtz_global_id_t messageId, const void* messageBuffer, qtz_send_flags_t flags) JEM_noexcept;


JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_CORE_H
