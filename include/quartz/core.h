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


typedef struct {} qtz_init_params_t;

typedef int(JEM_stdcall*qtz_deputy_proc_t)(void*);


extern const jem_u32_t qtz_page_size;




JEM_api jem_status_t JEM_stdcall qtz_init(const qtz_init_params_t* params);

JEM_api jem_status_t JEM_stdcall qtz_task_status(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_task_wait(qtz_request_t message);
JEM_api void         JEM_stdcall qtz_task_discard(qtz_request_t message);




JEM_api qtz_request_t JEM_stdcall qtz_alloc_pages(jem_u32_t requestCount, void** ppPages, const jem_u32_t* pPageCounts);
JEM_api void          JEM_stdcall qtz_free_pages(jem_u32_t requestCount, void* const * ppPages, const jem_u32_t* pPageCounts);



JEM_api qtz_request_t JEM_stdcall qtz_open_ipc_link();
JEM_api void          JEM_stdcall qtz_close_ipc_link();
JEM_api qtz_request_t JEM_stdcall qtz_send_ipc_message(const void* buffer, jem_u32_t messageSize);

JEM_api qtz_request_t JEM_stdcall qtz_open_deputy(qtz_deputy_t* pDeputy, qtz_deputy_proc_t proc, void* userData);
JEM_api void          JEM_stdcall qtz_close_deputy(qtz_deputy_t deputy, bool sendKillSignal);

JEM_api qtz_request_t JEM_stdcall qtz_attach_thread();
JEM_api void          JEM_stdcall qtz_detach_thread();
JEM_api qtz_request_t JEM_stdcall qtz_register_agent();
JEM_api void          JEM_stdcall qtz_unregister_agent();




JEM_end_c_namespace


#endif//JEMSYS_QUARTZ_CORE_H
