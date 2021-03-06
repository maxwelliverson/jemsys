//
// Created by maxwe on 2021-06-16.
//

#ifndef JEMSYS_AGATE_MESSAGES_H
#define JEMSYS_AGATE_MESSAGES_H

#include "core.h"

JEM_begin_c_namespace

typedef struct agt_type_descriptor*   agt_type_descriptor_t;
typedef struct agt_member_descriptor* agt_member_descriptor_t;

typedef enum {
  AGT_TYPE_INFO_CLASS_SIZE,
  AGT_TYPE_INFO_CLASS_ALIGNMENT,
  AGT_TYPE_INFO_CLASS_NAME,
  AGT_TYPE_INFO_CLASS_MEMBER_COUNT,
  AGT_TYPE_INFO_CLASS_MEMBER_DESCRIPTORS
} agt_type_info_class_t;
typedef struct {
  agt_type_info_class_t info_class;
  union{
    jem_size_t               integer;
    const char*              string;
    agt_member_descriptor_t* members;
  };
} agt_type_info_t;


JEM_api agt_type_descriptor_t JEM_stdcall agt_lookup_type_by_id(agt_module_t module, jem_u32_t id);
JEM_api agt_type_descriptor_t JEM_stdcall agt_lookup_type_by_name(agt_module_t module, const char* name);

JEM_api bool                  JEM_stdcall agt_get_type_info(agt_type_descriptor_t type, jem_size_t infoCount, agt_type_info_t* infos);




JEM_end_c_namespace

#endif//JEMSYS_AGATE_MESSAGES_H
