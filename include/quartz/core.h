//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_CORE_H
#define JEMSYS_QUARTZ_CORE_H

#include "jemsys.h"

JEM_begin_c_namespace

typedef jem_u32_t qtz_local_id_t;
typedef jem_u64_t qtz_global_id_t;

typedef struct qtz_mailbox*        qtz_mailbox_t;
typedef struct qtz_request*        qtz_request_t;
typedef struct qtz_deputy*         qtz_deputy_t;
typedef struct qtz_process*        qtz_process_t;
typedef struct qtz_shared_mailbox* qtz_shared_mailbox_t;
typedef struct qtz_module*         qtz_module_t;




extern const jem_u32_t qtz_page_size;


JEM_api jem_status_t JEM_stdcall qtz_request_status(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_request_wait(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_request_discard(qtz_request_t message);




JEM_api qtz_request_t JEM_stdcall qtz_open_ipc_link();
JEM_api void          JEM_stdcall qtz_close_ipc_link();
JEM_api qtz_request_t JEM_stdcall qtz_send_ipc_message(const void* buffer, jem_u32_t messageSize);



JEM_api qtz_request_t JEM_stdcall qtz_register_agent(jem_u64_t* pResultId, void* state);
JEM_api void          JEM_stdcall qtz_unregister_agent();




JEM_end_c_namespace


#endif//JEMSYS_QUARTZ_CORE_H
