//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_DEPUTY_H
#define JEMSYS_QUARTZ_DEPUTY_H

#include "core.h"

JEM_begin_c_namespace

typedef enum {
  QTZ_DEPUTY_KIND_THREAD,
  QTZ_DEPUTY_KIND_THREAD_POOL,
  QTZ_DEPUTY_KIND_LAZY,
  QTZ_DEPUTY_KIND_PROXY,
  QTZ_DEPUTY_KIND_VIRTUAL
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

} qtz_lazy_deputy_params_t;
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
JEM_api void          JEM_stdcall qtz_close_deputy(qtz_deputy_t deputy, bool sendKillSignal);



JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_DEPUTY_H
