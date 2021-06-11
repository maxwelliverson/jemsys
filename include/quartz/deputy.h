//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_DEPUTY_H
#define JEMSYS_QUARTZ_DEPUTY_H




/**
 * [ Deputies ]
 *
 * A deputy is a low level abstract executor. Deputies can, in practice,
 * be thought of as an autonomous request queue. Arbitrary messages can
 * be sent to
 * A deputy is a flexible abstraction of an autonomous request queue.
 *
 * Thread Deputy:
 *
 * ThreadPool Deputy:
 *
 * Lazy Deputy:
 *
 * Proxy Deputy:
 *
 * Virtual Deputy:
 *
 * Null Deputy:
 *
 * */

#include "core.h"

JEM_begin_c_namespace


#define QTZ_NULL_DEPUTY ((qtz_deputy_t)0)


typedef enum {
  QTZ_DEPUTY_KIND_THREAD,      // Single mailbox backed by single thread
  QTZ_DEPUTY_KIND_THREAD_POOL, // Single mailbox backed by thread pool
  QTZ_DEPUTY_KIND_LAZY,        // Single mailbox backed by a single thread, only woken to satisfy request on a wait operation.
  QTZ_DEPUTY_KIND_PROXY,
  QTZ_DEPUTY_KIND_VIRTUAL,
  QTZ_DEPUTY_KIND_NULL
} qtz_deputy_kind_t;

typedef void(JEM_stdcall* qtz_thread_deputy_proc_t)(qtz_deputy_t deputy, void* userData);
typedef void(JEM_stdcall* qtz_deputy_message_proc_t)(qtz_deputy_t deputy, void* message, void* userData);

typedef struct {
  qtz_thread_deputy_proc_t pfnEntryPoint;
  void*                    pUserData;
} qtz_thread_deputy_params_t;
typedef struct {
  jem_u32_t threadCount;
} qtz_thread_pool_deputy_params_t;
typedef struct {
  const qtz_deputy_t* pDeputies;
  jem_size_t          deputyCount;
} qtz_proxy_deputy_params_t;
typedef struct {
  const void* const* ppVirtualDeputyParams;
  jem_size_t         virtualDeputyParamCount;
} qtz_virtual_deputy_params_t;


typedef struct {
  const char*       name;
  qtz_module_t      module;
  const jem_u32_t*  pMessageTypeIds;
  const qtz_deputy_message_proc_t* pMessageProcs;
  jem_size_t        messageTypeCount;
  qtz_deputy_kind_t kind;
  const void*       pKindParams;
} qtz_deputy_params_t;

JEM_api qtz_request_t JEM_stdcall qtz_open_deputy(qtz_deputy_t* pDeputy, const qtz_deputy_params_t* pParams);
JEM_api void          JEM_stdcall qtz_close_deputy(qtz_deputy_t deputy);
JEM_api qtz_request_t JEM_stdcall qtz_send_to_deputy(qtz_deputy_t deputy, const void* buffer, jem_size_t bytes);

JEM_api JEM_noreturn void JEM_stdcall qtz_convert_thread_to_deputy(const qtz_deputy_params_t* pParams);


JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_DEPUTY_H
