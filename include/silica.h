//
// Created by maxwe on 2021-08-30.
//

#ifndef JEMSYS_SILICA_H
#define JEMSYS_SILICA_H

#include "jemsys.h"

JEM_begin_c_namespace



#define SLC_KERNEL_VERSION_MOST_RECENT ((jem_u32_t)-1)
#define SLC_DEFAULT_MODULE ((slc_module_t)nullptr)
#define SLC_GLOBAL_MODULE_ID ((slc_local_id_t)0)
  
  

typedef struct slc_module*         slc_module_t;
typedef struct slc_module_builder* slc_module_builder_t;
typedef struct slc_error*          slc_error_t;


typedef enum {
  SLC_SUCCESS,
  SLC_ERROR_INTERNAL,
  SLC_ERROR_UNKNOWN,
  SLC_ERROR_BAD_SIZE,
  SLC_ERROR_INVALID_ARGUMENT,
  SLC_ERROR_BAD_ENCODING_IN_NAME,
  SLC_ERROR_NAME_TOO_LONG,
  SLC_ERROR_INSUFFICIENT_BUFFER_SIZE,
  SLC_ERROR_BAD_ALLOC,
  SLC_ERROR_TOO_MANY_PRODUCERS,
  SLC_ERROR_TOO_MANY_CONSUMERS
} slc_status_t;





typedef enum {
  SLC_TYPE_DEFAULT_FLAGS        = 0x0000,
  SLC_TYPE_CONST_QUALIFIED      = 0x0001,
  SLC_TYPE_VOLATILE_QUALIFIED   = 0x0002,
  SLC_TYPE_MUTABLE_QUALIFIED    = 0x0004,
  SLC_TYPE_PUBLIC_VISIBILITY    = 0x0008,
  SLC_TYPE_PRIVATE_VISIBILITY   = 0x0010,
  SLC_TYPE_PROTECTED_VISIBILITY = 0x0020,
  SLC_TYPE_IS_VIRTUAL           = 0x0040,
  SLC_TYPE_IS_ABSTRACT          = 0x0080,
  SLC_TYPE_IS_OVERRIDE          = 0x0100,
  SLC_TYPE_IS_FINAL             = 0x0200,
  SLC_TYPE_IS_BITFIELD          = 0x0400,
  SLC_TYPE_IS_NOEXCEPT          = 0x0800,
  SLC_TYPE_IS_VARIADIC          = 0x1000,
  SLC_TYPE_IS_OPAQUE            = 0x2000
} slc_type_flags_t;
typedef enum {
  SLC_TYPE_KIND_VOID,
  SLC_TYPE_KIND_BOOLEAN,
  SLC_TYPE_KIND_INTEGRAL,
  SLC_TYPE_KIND_FLOATING_POINT,
  SLC_TYPE_KIND_POINTER,
  SLC_TYPE_KIND_ARRAY,
  SLC_TYPE_KIND_FUNCTION,
  SLC_TYPE_KIND_MEMBER_POINTER,
  SLC_TYPE_KIND_MEMBER_FUNCTION_POINTER,
  SLC_TYPE_KIND_AGGREGATE,
  SLC_TYPE_KIND_ENUMERATOR
} slc_type_kind_t;
typedef enum {
  SLC_SINGLE_INHERITANCE,
  SLC_MULTI_INHERITANCE,
  SLC_VIRTUAL_INHERITANCE
} slc_inheritance_kind_t;

typedef enum {
  SLC_MODULE_FUNCTION_LOCATION_POINTER,
  SLC_MODULE_FUNCTION_LOCATION_CODE_BUFFER,
  SLC_MODULE_FUNCTION_LOCATION_ALIAS
} slc_function_location_t;
typedef enum {
  SLC_ABI_ITANIUM,
  SLC_ABI_MSVC_x86,
  SLC_ABI_MSVC_x64,
#if JEM_64bit
SLC_ABI_MSVC = SLC_ABI_MSVC_x64,
#else
SLC_ABI_MSVC = SLC_ABI_MSVC_x86,
#endif
} slc_abi_t;
typedef enum {
  SLC_CALLCONV_CDECL
} slc_calling_convention_t;

typedef enum {
  SLC_ATTRIBUTE_ABI,
  SLC_ATTRIBUTE_ENDIANNESS,
  SLC_ATTRIBUTE_POINTER_SIZE,
  SLC_ATTRIBUTE_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_SI_MEMBER_POINTER_SIZE,
  SLC_ATTRIBUTE_MI_MEMBER_POINTER_SIZE,
  SLC_ATTRIBUTE_VI_MEMBER_POINTER_SIZE,
  SLC_ATTRIBUTE_SI_MEMBER_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_MI_MEMBER_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_VI_MEMBER_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_SI_MEMBER_FUNCTION_POINTER_SIZE,
  SLC_ATTRIBUTE_MI_MEMBER_FUNCTION_POINTER_SIZE,
  SLC_ATTRIBUTE_VI_MEMBER_FUNCTION_POINTER_SIZE,
  SLC_ATTRIBUTE_SI_MEMBER_FUNCTION_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_MI_MEMBER_FUNCTION_POINTER_ALIGNMENT,
  SLC_ATTRIBUTE_VI_MEMBER_FUNCTION_POINTER_ALIGNMENT,
} slc_attribute_t;

typedef struct slc_type_descriptor_t slc_type_descriptor_t;

typedef struct slc_type_ref_t {
  const slc_type_descriptor_t* type;
  jem_u16_t                    flags;
} slc_type_ref_t;


typedef struct slc_aggregate_member_descriptor_t {
  const slc_type_descriptor_t* type;
  jem_u16_t                    flags;
  jem_u8_t                     bitfield_bits;
  jem_u8_t                     bitfield_offset;
  jem_u32_t                    offset;
  const char*                  name;
} slc_aggregate_member_descriptor_t;
typedef struct slc_aggregate_base_descriptor_t {
  const slc_type_descriptor_t* type;
  jem_u16_t                    flags;
  jem_u32_t                    offset;
} slc_aggregate_base_descriptor_t;
typedef struct slc_enumeration_descriptor_t {
  const char* name;
  jem_u64_t   value;
} slc_enumeration_descriptor_t;
typedef struct slc_function_descriptor_t {
  const slc_type_descriptor_t* type;
  slc_function_location_t      location;
  const char*                  name;
  union{
    void(*function_pointer)();
    struct {
      const void* code_buffer;
      jem_size_t  code_buffer_length;
    };
    const char* aliased_function;
  };
} slc_function_descriptor_t;


typedef struct slc_integral_type_params_t {
  const char* name;
  jem_u32_t   size;
  jem_u32_t   alignment;
  bool        is_signed;
} slc_integral_type_params_t;
typedef struct slc_floating_point_type_params_t {
  const char* name;
  jem_u32_t   size;
  jem_u32_t   alignment;
  bool        is_signed;
  jem_u16_t   exponent_bits;
  jem_u32_t   mantissa_bits;
} slc_floating_point_type_params_t;
typedef struct slc_alias_type_params_t {
  const char*    name;
  slc_type_ref_t target;
} slc_alias_type_params_t;
typedef struct slc_pointer_type_params_t{
  slc_type_ref_t target;
} slc_pointer_type_params_t;
typedef struct slc_reference_type_params_t{
  slc_type_ref_t target;
} slc_reference_type_params_t;
typedef struct slc_array_type_params_t{
  slc_type_ref_t elements;
  jem_u32_t      count;
} slc_array_type_params_t;
typedef struct slc_function_type_params_t{
  slc_calling_convention_t callconv;
  slc_type_ref_t           ret;
  const slc_type_ref_t*    args;
  jem_size_t               arg_count;
  slc_type_flags_t         flags;
} slc_function_type_params_t;
typedef struct slc_member_pointer_type_params_t{
  slc_type_ref_t               pointee;
  const slc_type_descriptor_t* aggregate_type;
} slc_member_pointer_type_params_t;
typedef struct slc_member_function_pointer_type_params_t{
  slc_type_ref_t               function;
  const slc_type_descriptor_t* aggregate_type;
} slc_member_function_pointer_type_params_t;
typedef struct slc_aggregate_type_params_t{
  const char*                                      name;
  slc_type_flags_t                                 flags;
  jem_u32_t                                        alignment;
  const slc_type_ref_t*                            bases;
  jem_size_t                                       base_count;
  const slc_aggregate_member_descriptor_t* const * members;
  jem_size_t                                       member_count;
} slc_aggregate_type_params_t;
typedef struct slc_enumerator_type_params_t{
  const char*                         name;
  const slc_type_descriptor_t*        underlying_type;
  const slc_enumeration_descriptor_t* entries;
  jem_size_t                          entry_count;
  bool                                is_bitflag;
} slc_enumerator_type_params_t;


typedef struct slc_type_params_t {
  slc_type_kind_t kind;
  const void*     params;
} slc_type_params_t;





typedef struct slc_platform_constants_t {
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
} slc_platform_constants_t;

typedef struct slc_type_entry_t {
  jem_local_id_t               local_id;
  jem_global_id_t              global_id;
  const slc_type_descriptor_t* descriptor;
} slc_type_entry_t;
typedef struct slc_function_entry_t {
  jem_local_id_t                   local_id;
  jem_global_id_t                  global_id;
  const slc_function_descriptor_t* descriptor;
} slc_function_entry_t;


struct slc_type_descriptor_t {
  slc_type_kind_t kind;
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
      slc_type_ref_t pointee;
    } pointer;
    struct {
      slc_type_ref_t element;
      jem_size_t     size;
    } array;
    struct {
      slc_type_ref_t        ret;
      const slc_type_ref_t* args;
      jem_u32_t             arg_count;
      jem_u16_t             flags;
    } function;
    struct {
      slc_type_ref_t               pointee;
      const slc_type_descriptor_t* aggregate_type;
    } member_pointer;
    struct {
      slc_type_ref_t               function;
      const slc_type_descriptor_t* aggregate_type;
    } member_function_pointer;
    struct {
      const char*                              name;
      const slc_aggregate_base_descriptor_t*   bases;
      jem_size_t                               base_count;
      const slc_aggregate_member_descriptor_t* members;
      jem_size_t                               member_count;
      jem_u16_t                                flags;
      slc_inheritance_kind_t              inheritance_kind;
    } aggregate;
    struct {
      const char*                         name;
      const slc_type_descriptor_t*        underlying_type;
      const slc_enumeration_descriptor_t* entries;
      jem_u32_t                           entry_count;
      bool                                is_bitflag;
    } enumerator;
  };
};







JEM_api void         JEM_stdcall slc_open_module_builder(slc_module_builder_t* pModuleBuilder);
JEM_api void         JEM_stdcall slc_module_add_functions(slc_module_builder_t moduleBuilder, const slc_function_descriptor_t* pFunctions, jem_size_t functionCount);
JEM_api void         JEM_stdcall slc_module_add_types(slc_module_builder_t moduleBuilder, const slc_type_descriptor_t* pTypes, jem_size_t typeCount);
JEM_api void         JEM_stdcall slc_module_add_dependencies(slc_module_builder_t moduleBuilder, const slc_module_t* pDependencies, jem_size_t dependencyCount);




JEM_api slc_status_t JEM_stdcall slc_build_module(slc_module_t* pModule, slc_module_builder_t builder);
JEM_api void         JEM_stdcall slc_close_module(slc_module_t module);
JEM_api slc_status_t JEM_stdcall slc_load_module(slc_module_t* pModule, const char* path);
JEM_api slc_status_t JEM_stdcall slc_save_module(slc_module_t module, const char* path);

JEM_api slc_status_t JEM_stdcall slc_module_enumerate_types(slc_module_t module, slc_type_entry_t* types, jem_size_t* typeCount) JEM_noexcept;
JEM_api slc_status_t JEM_stdcall slc_module_enumerate_functions(slc_module_t module, slc_function_entry_t* functions, jem_size_t* functionCount) JEM_noexcept;






JEM_end_c_namespace

#endif//JEMSYS_SILICA_H
