//
// Created by maxwe on 2021-06-11.
//

#ifndef JEMSYS_AGATE_DEPUTY_H
#define JEMSYS_AGATE_DEPUTY_H

/**
 * [ Deputies ]
 *
 * A deputy is a low level abstract executor. Deputies can, in practice,
 * be thought of as an autonomous request queue. Arbitrary messages can
 * be sent to
 * A deputy is a flexible abstraction of an autonomous request queue.
 *
 * Thread Deputy: Backed by a single thread which runs a user defined callback
 *                function which itself is responsible for reading and processing
 *                the messages sent to it.
 *
 * Thread Pool Deputy: Backed by a thread pool which delegates incoming messages
 *                    to individual threads, which then execute a user defined
 *                    callback function whenever a message needs to be processed.
 *
 * Lazy Deputy: Backed by a single thread that sits idle until any given message
 *              response is waited on. A couple notable consequence of this are
 *              that messages have no guaranteed order of execution, and that
 *              "fire and forget" messages are discarded.
 *
 * Proxy Deputy: Messages sent to a proxy deputy are filtered/dispatched by a
 *               user defined callback function. The callback must be thread
 *               safe if messages are to be sent concurrently.
 *
 * Virtual Deputy: Backed by nothing. Similar to the lazy deputy in that messages
 *                 are only processed with waited on, or after checking message
 *                 status at least N times, where N is a user specified number.
 *                 The messages are however then processed on the waiting thread.
 *
 * Collective Deputy: One of a group of deputies which are all backed by the same
 *                    thread.
 *
 *
 * Null Deputy: All messages sent to a null deputy are immediately discarded.
 *
 * */

#include "core.h"

JEM_begin_c_namespace

#define AGT_NULL_DEPUTY ((agt_deputy_t)0)
#define AGT_DISCARD_MESSAGE ((jem_u32_t)-1)



typedef enum {
  AGT_DEPUTY_KIND_THREAD,
  AGT_DEPUTY_KIND_THREAD_POOL,
  AGT_DEPUTY_KIND_LAZY,
  AGT_DEPUTY_KIND_PROXY,
  AGT_DEPUTY_KIND_VIRTUAL,
  AGT_DEPUTY_KIND_COLLECTIVE,
  AGT_DEPUTY_KIND_NULL
} agt_deputy_kind_t;


typedef void(JEM_stdcall*      agt_thread_deputy_proc_t)(agt_deputy_t deputy, void* userData);
typedef void(JEM_stdcall*      agt_message_proc_t)(agt_deputy_t deputy, void* message, void* userData);
typedef jem_u32_t(JEM_stdcall* agt_message_filter_t)(agt_deputy_t deputy, jem_u32_t messageId, void* userData);

typedef struct {
  agt_thread_deputy_proc_t pfnEntryPoint;
  void*                    pUserData;
} agt_thread_deputy_params_t;
typedef struct {
  jem_u32_t          threadCount;
} agt_thread_pool_deputy_params_t;
typedef struct {
  const agt_deputy_t*  pDeputies;
  jem_size_t           deputyCount;
  agt_message_filter_t pfnFilter;
  void*                pUserData;
} agt_proxy_deputy_params_t;
typedef struct {
  const void* const* ppVirtualDeputyParams;
  jem_size_t         virtualDeputyParamCount;
} agt_virtual_deputy_params_t;


typedef struct {
  const char*        name;
  agt_module_t       module;
  agt_deputy_kind_t  kind;
  const void*        pKindParams;
  agt_message_proc_t pfnMessageProc;
  void*              pUserData;
} agt_deputy_params_t;

JEM_api agt_request_t  JEM_stdcall agt_open_deputy(agt_deputy_t* pDeputy, const agt_deputy_params_t* pParams);
JEM_api void           JEM_stdcall agt_close_deputy(agt_deputy_t deputy);



JEM_api agt_promise_t JEM_stdcall agt_send_to_deputy(agt_deputy_t deputy, jem_u32_t msgId, const void* buffer);

JEM_api JEM_noreturn void JEM_stdcall agt_convert_thread_to_deputy(const agt_deputy_params_t* pParams);

JEM_api agt_deputy_t JEM_stdcall agt_current_deputy();

// JEM_api agt_deputy_t JEM_stdcall agt_current_deputy();




JEM_end_c_namespace

#endif//JEMSYS_AGATE_DEPUTY_H
