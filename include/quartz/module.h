//
// Created by maxwe on 2021-06-11.
//

#ifndef JEMSYS_QUARTZ_MODULE_H
#define JEMSYS_QUARTZ_MODULE_H

#include "core.h"

JEM_begin_c_namespace

typedef struct qtz_type_descriptor* qtz_type_descriptor_t;
typedef struct qtz_module_entry*    qtz_module_entry_t;
typedef struct qtz_module_builder*  qtz_module_builder_t;


/*typedef struct {
  bool
} qtz_module_builder_params_t;*/



JEM_api qtz_request_t JEM_stdcall qtz_open_module_builder(qtz_module_builder_t* pModuleBuilder);
JEM_api void          JEM_stdcall qtz_add_module_dependencies(qtz_module_builder_t moduleBuilder, const qtz_module_t* pDependencies, jem_size_t dependencyCount);


JEM_api qtz_request_t JEM_stdcall qtz_build_module(qtz_module_t* pModule, qtz_module_builder_t builder);





JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_MODULE_H
