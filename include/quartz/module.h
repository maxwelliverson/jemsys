//
// Created by maxwe on 2021-06-11.
//

#ifndef JEMSYS_QUARTZ_MODULE_H
#define JEMSYS_QUARTZ_MODULE_H

#include "core.h"

JEM_begin_c_namespace

typedef enum {
  qtz_type_default_flags        = 0x0000,
  qtz_type_const_qualified      = 0x0001,
  qtz_type_volatile_qualified   = 0x0002,
  qtz_type_mutable_qualified    = 0x0004,
  qtz_type_public_visibility    = 0x0008,
  qtz_type_private_visibility   = 0x0010,
  qtz_type_protected_visibility = 0x0020,
  qtz_type_is_virtual           = 0x0040,
  qtz_type_is_abstract          = 0x0080,
  qtz_type_is_override          = 0x0100,
  qtz_type_is_final             = 0x0200,
  qtz_type_is_bitfield          = 0x0400,
  qtz_type_is_noexcept          = 0x0800,
  qtz_type_is_variadic          = 0x1000,
  qtz_type_is_opaque            = 0x2000
} qtz_type_flags_t;
typedef enum {
  qtz_type_kind_void,
  qtz_type_kind_boolean,
  qtz_type_kind_integral,
  qtz_type_kind_floating_point,
  qtz_type_kind_pointer,
  qtz_type_kind_array,
  qtz_type_kind_function,
  qtz_type_kind_member_pointer,
  qtz_type_kind_member_function_pointer,
  qtz_type_kind_aggregate,
  qtz_type_kind_enumerator
} qtz_type_kind_t;
typedef enum {
  qtz_single_inheritance,
  qtz_multi_inheritance,
  qtz_virtual_inheritance
} qtz_type_inheritance_kind_t;

typedef enum {
  QTZ_MODULE_FUNCTION_KIND_POINTER,
  QTZ_MODULE_FUNCTION_KIND_CODE_BUFFER,
  QTZ_MODULE_FUNCTION_KIND_ALIAS
} qtz_module_function_kind_t;
typedef enum {
  QTZ_MODULE_ABI_ITANIUM,
  QTZ_MODULE_ABI_MSVC_x86,
  QTZ_MODULE_ABI_MSVC_x64,
#if JEM_64bit
  QTZ_MODULE_ABI_MSVC = QTZ_MODULE_ABI_MSVC_x64,
#else
  QTZ_MODULE_ABI_MSVC = QTZ_MODULE_ABI_MSVC_x86,
#endif
} qtz_module_abi_t;
typedef enum {
  QTZ_MODULE_CALLING_CONVENTION_CDECL
} qtz_module_calling_convention_t;



typedef struct qtz_type_descriptor_t qtz_type_descriptor_t;

typedef struct qtz_type_ref_t {
  const qtz_type_descriptor_t* type;
  jem_u16_t                    flags;
} qtz_type_ref_t;

typedef struct qtz_aggregate_member_descriptor_t {
  const qtz_type_descriptor_t* type;
  jem_u16_t                    flags;
  jem_u8_t                     bitfield_bits;
  jem_u8_t                     bitfield_offset;
  jem_u32_t                    offset;
  const char*                  name;
} qtz_aggregate_member_descriptor_t;
typedef struct qtz_aggregate_base_descriptor_t {
  const qtz_type_descriptor_t* type;
  jem_u16_t                    flags;
  jem_u32_t                    offset;
} qtz_aggregate_base_descriptor_t;
typedef struct qtz_enumeration_descriptor_t {
  const char* name;
  jem_u64_t   value;
} qtz_enumeration_descriptor_t;
typedef struct qtz_function_descriptor_t {
  const qtz_type_descriptor_t* type;
  qtz_module_function_kind_t   kind;
  const char*                  name;
  union{
    void(*function_pointer)();
    struct {
      const void* code_buffer;
      jem_size_t  code_buffer_length;
    };
    const char* aliased_function;
  };
} qtz_function_descriptor_t;

typedef struct qtz_platform_constants_t {
  jem_size_t pointer_size;
  jem_size_t pointer_alignment;
  jem_size_t si_member_pointer_size;
  jem_size_t mi_member_pointer_size;
  jem_size_t vi_member_pointer_size;
  jem_size_t si_member_pointer_alignment;
  jem_size_t mi_member_pointer_alignment;
  jem_size_t vi_member_pointer_alignment;
  jem_size_t si_member_function_pointer_size;
  jem_size_t mi_member_function_pointer_size;
  jem_size_t vi_member_function_pointer_size;
  jem_size_t si_member_function_pointer_alignment;
  jem_size_t mi_member_function_pointer_alignment;
  jem_size_t vi_member_function_pointer_alignment;
} qtz_platform_constants_t;

typedef struct qtz_module_type_t {
  qtz_local_id_t               local_id;
  qtz_global_id_t              global_id;
  const qtz_type_descriptor_t* descriptor;
} qtz_module_type_t;
typedef struct qtz_module_function_t {
  qtz_local_id_t                   local_id;
  qtz_global_id_t                  global_id;
  const qtz_function_descriptor_t* descriptor;
} qtz_module_function_t;


struct qtz_type_descriptor_t {
  qtz_type_kind_t kind;
  jem_size_t      size;
  jem_size_t      alignment;
  union {
    struct {
      const char* name;
      bool        is_signed;
    } integral;
    struct {
      const char* name;
      bool        is_signed;
      jem_u16_t   exponent_bits;
      jem_u32_t   mantissa_bits;
    } floating_point;
    struct {
      qtz_type_ref_t pointee;
    } pointer;
    struct {
      qtz_type_ref_t element;
      jem_size_t     size;
    } array;
    struct {
      qtz_type_ref_t        ret;
      const qtz_type_ref_t* args;
      jem_u32_t             arg_count;
      jem_u16_t             flags;
    } function;
    struct {
      qtz_type_ref_t               pointee;
      const qtz_type_descriptor_t* aggregate_type;
    } member_pointer;
    struct {
      qtz_type_ref_t               function;
      const qtz_type_descriptor_t* aggregate_type;
    } member_function_pointer;
    struct {
      const char*                              name;
      const qtz_aggregate_base_descriptor_t*   bases;
      jem_size_t                               base_count;
      const qtz_aggregate_member_descriptor_t* members;
      jem_size_t                               member_count;
      jem_u16_t                                flags;
      qtz_type_inheritance_kind_t              inheritance_kind;
    } aggregate;
    struct {
      const char*                         name;
      const qtz_type_descriptor_t*        underlying_type;
      const qtz_enumeration_descriptor_t* entries;
      jem_u32_t                           entry_count;
      bool                                is_bitflag;
    } enumerator;
  };
};

typedef struct qtz_module_builder*  qtz_module_builder_t;


/*typedef struct {
  bool
} qtz_module_builder_params_t;*/




JEM_api qtz_request_t JEM_stdcall qtz_open_module_builder(qtz_module_builder_t* pModuleBuilder);
JEM_api void          JEM_stdcall qtz_module_add_functions(qtz_module_builder_t moduleBuilder, const qtz_function_descriptor_t* pFunctions, jem_size_t functionCount);
JEM_api void          JEM_stdcall qtz_module_add_types(qtz_module_builder_t moduleBuilder, const qtz_type_descriptor_t* pTypes, jem_size_t typeCount);
JEM_api void          JEM_stdcall qtz_module_add_dependencies(qtz_module_builder_t moduleBuilder, const qtz_module_t* pDependencies, jem_size_t dependencyCount);




JEM_api qtz_status_t  JEM_stdcall qtz_build_module(qtz_module_t* pModule, qtz_module_builder_t builder);
JEM_api void          JEM_stdcall qtz_close_module(qtz_module_t module);
JEM_api qtz_status_t  JEM_stdcall qtz_load_module(qtz_module_t* pModule, const char* path);
JEM_api qtz_status_t  JEM_stdcall qtz_save_module(qtz_module_t module, const char* path);

JEM_api qtz_status_t  JEM_stdcall qtz_module_enumerate_types(qtz_module_t module, qtz_type_descriptor_t* types, jem_size_t* typeCount) JEM_noexcept;
JEM_api qtz_status_t  JEM_stdcall qtz_module_enumerate_functions(qtz_module_t module, qtz_function_descriptor_t* functions, jem_size_t* functionCount) JEM_noexcept;






JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_MODULE_H
