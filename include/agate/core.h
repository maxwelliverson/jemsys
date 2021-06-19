//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_AGATE_CORE_H
#define JEMSYS_AGATE_CORE_H

#include "jemsys.h"

JEM_begin_c_namespace

typedef struct agt_mailbox*  agt_mailbox_t;
typedef struct agt_deputy*   agt_deputy_t;
typedef struct agt_promise*  agt_promise_t;
typedef struct agt_message*  agt_message_t;
typedef struct agt_progress* agt_progress_t;
typedef struct agt_signal*   agt_signal_t;

typedef struct qtz_request*  agt_request_t;
typedef struct qtz_module*   agt_module_t;


typedef enum {
  AGT_SUCCESS,
  AGT_NOT_READY,
  AGT_CANCELLED,
  AGT_TIMED_OUT,
  AGT_ERROR_UNKNOWN,
  AGT_ERROR_INVALID_MESSAGE,
  AGT_ERROR_BAD_SIZE,
  AGT_ERROR_INVALID_ARGUMENT,
  AGT_ERROR_BAD_ENCODING_IN_NAME,
  AGT_ERROR_NAME_TOO_LONG,
  AGT_ERROR_BAD_ALLOC,
  AGT_ERROR_MAILBOX_IS_FULL,
  AGT_ERROR_TOO_MANY_PRODUCERS,
  AGT_ERROR_TOO_MANY_CONSUMERS
} agt_status_t;






JEM_end_c_namespace

#endif//JEMSYS_AGATE_CORE_H
