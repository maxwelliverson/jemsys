//
// Created by maxwe on 2021-11-20.
//

#ifndef JEMSYS_SILICA2_H
#define JEMSYS_SILICA2_H

#include <jemsys.h>

#if JEM_compiler_msvc
#pragma push_macro("interface")
#undef interface
#endif


#define SLC_DEF_OBJECT(name) typedef struct name##_st* name
#define SLC_DEF_RESOURCE(name) typedef const struct name##_st* name


JEM_begin_c_namespace



typedef jem_u8_t  SlcUInt8;
typedef jem_u16_t SlcUInt16;
typedef jem_u32_t SlcUInt32;
typedef jem_u64_t SlcUInt64;

typedef SlcUInt32 SlcMemberOffset;
typedef SlcUInt64 SlcOffset;
typedef SlcUInt64 SlcSize;

typedef jem_i32_t SlcBool;




SLC_DEF_OBJECT(SlcRuntime);
SLC_DEF_OBJECT(SlcError);
SLC_DEF_OBJECT(SlcFactory);
SLC_DEF_OBJECT(SlcTypeFactory);
SLC_DEF_OBJECT(SlcInterfaceFactory);
SLC_DEF_OBJECT(SlcModuleFactory);


SLC_DEF_RESOURCE(SlcInterface);
SLC_DEF_RESOURCE(SlcInterfaceMethod);
SLC_DEF_RESOURCE(SlcInterfaceModule);
SLC_DEF_RESOURCE(SlcTypeDescriptor);
SLC_DEF_RESOURCE(SlcArchetype);
SLC_DEF_RESOURCE(SlcModule);





typedef enum SlcStatus {
  SLC_SUCCESS,
  SLC_ERROR_INTERNAL,
  SLC_ERROR_UNKNOWN,
  SLC_ERROR_NULL_TYPE,
  SLC_ERROR_BAD_FLAGS_ON_BASE,
  SLC_ERROR_INHERIT_FROM_FINAL,
  SLC_ERROR_INHERIT_FROM_NON_AGGREGATE,
  SLC_ERROR_INHERIT_FROM_OPAQUE,
  SLC_ERROR_OPAQUE_INSTANCE,
  SLC_ERROR_ABSTRACT_INSTANCE,
  SLC_ERROR_MEMBER_NAME_CLASH
} SlcStatus;


typedef enum SlcTypeModifierFlagBits {
  SLC_DEFAULT_FLAGS        = 0x0000,
  SLC_CONST_QUALIFIED      = 0x0001,
  SLC_VOLATILE_QUALIFIED   = 0x0002,
  SLC_IS_VIRTUAL           = 0x0040,
  SLC_IS_ABSTRACT          = 0x0080,
  SLC_IS_OVERRIDE          = 0x0100,
  SLC_IS_FINAL             = 0x0200,
  SLC_IS_BITFIELD          = 0x0400,
  SLC_IS_NOEXCEPT          = 0x0800,
  SLC_IS_VARIADIC          = 0x1000,
  SLC_IS_OPAQUE            = 0x2000
} SlcTypeModifierFlagBits;
typedef SlcUInt32 SlcTypeModifierFlags;

typedef enum SlcTypeKind {
  SLC_TYPE_VOID,        /// None
  SLC_TYPE_BOOLEAN,     /// None
  SLC_TYPE_BYTE,        /// None
  SLC_TYPE_CHAR,        /// size    <= { [1], 2, 4 }
  SLC_TYPE_INT,         /// size    <= { 1, 2, [4], 8, 16, ... }
  SLC_TYPE_UINT,        /// size    <= { 1, 2, [4], 8, 16, ... }
  SLC_TYPE_FLOAT,       /// size    <= { 2, [4], 8, 16, ... } OR pointer <= SlcFloatTypeInfo
  SLC_TYPE_POINTER,     /// pointer <= SlcType
  SLC_TYPE_ARRAY,       /// pointer <= SlcArrayTypeInfo
  SLC_TYPE_FUNCTION,    /// pointer <= SlcFunctionTypeInfo
  SLC_TYPE_STRUCT,      /// pointer <= SlcAggregateTypeInfo
  SLC_TYPE_UNION,       /// pointer <= SlcAggregateTypeInfo
  SLC_TYPE_BITFIELD,    /// size    <= { any positive integer, no default }
  SLC_TYPE_ENUM,        /// pointer <= SlcEnumTypeInfo
  SLC_TYPE_DESCRIPTOR,  /// pointer <= SlcTypeDescriptor
  SLC_TYPE_ALIAS,       /// ???
  SLC_TYPE_NOMINAL,     /// pointer <= const char* (name of resolved type)
  /*SLC_TYPE_KIND_CHANNEL_SENDER,
  SLC_TYPE_KIND_CHANNEL_RECEIVER,
  SLC_TYPE_KIND_INTERFACE*/
} SlcTypeKind;

typedef struct SlcType {
  SlcTypeKind kind;
  SlcTypeModifierFlags modifiers;
  union {
    const void* pointer;
    SlcSize     size;
  };
} SlcType;

typedef struct SlcAggregateMemberInfo {
  const char* pName;
  SlcSize     nameLength;
  SlcType     type;
  SlcSize     alignment;
} SlcAggregateMemberInfo;
typedef struct SlcEnumValueInfo {
  const char* pName;
  SlcSize     nameLength;
  SlcUInt64   value;
} SlcEnumValueInfo;

typedef struct SlcFloatTypeInfo {
  SlcUInt32 mantissaBits;
  SlcUInt32 exponentBits;
  SlcBool   isSigned;
} SlcFloatTypeInfo;
typedef struct SlcArrayTypeInfo {
  SlcSize size;
  SlcType elementType;
} SlcArrayTypeInfo;
typedef struct SlcFunctionTypeInfo {
  SlcType              returnType;
  SlcTypeModifierFlags modifiers;
  SlcSize              paramCount;
  const SlcType*       pParamTypes;
} SlcFunctionTypeInfo;
typedef struct SlcAggregateTypeInfo {
  const char*                   pName;
  SlcSize                       nameLength;
  SlcSize                       memberCount;
  const SlcAggregateMemberInfo* pMembers;
  SlcSize                       alignment;
} SlcAggregateTypeInfo;
typedef struct SlcEnumTypeInfo {
  const char*             pName;
  SlcSize                 nameLength;
  SlcSize                 valueCount;
  const SlcEnumValueInfo* pValues;
  SlcBool                 isBitflag;
} SlcEnumTypeInfo;








/*typedef enum SlcModuleAttribute {
  SLC_MODULE_ATTRIBUTE_POINTER_SIZE,
  SLC_MODULE_ATTRIBUTE_POINTER_ALIGNMENT
} SlcModuleAttribute;*/

typedef enum SlcAttribute {
  SLC_ATTRIBUTE_NAME,
  SLC_ATTRIBUTE_SIZE,
  SLC_ATTRIBUTE_ALIGNMENT,
  SLC_ATTRIBUTE_OFFSET,
  SLC_ATTRIBUTE_MODIFIERS,
  SLC_ATTRIBUTE_MEMBER_COUNT,
  SLC_ATTRIBUTE_MEMBERS,
  SLC_ATTRIBUTE_INTERFACE_COUNT,
  SLC_ATTRIBUTE_INTERFACES,
  SLC_ATTRIBUTE_ENUM_VALUE_COUNT,
  SLC_ATTRIBUTE_ENUM_VALUES,
  SLC_ATTRIBUTE_RETURN_TYPE,
  SLC_ATTRIBUTE_PARAM_COUNT,
  SLC_ATTRIBUTE_PARAMS,
  SLC_ATTRIBUTE_METHOD_COUNT,
  SLC_ATTRIBUTE_METHODS,
  SLC_ATTRIBUTE_PARENT_COUNT,
  SLC_ATTRIBUTE_PARENTS
} SlcAttribute;



typedef struct SlcQualifiedType {
  SlcTypeDescriptor    descriptor;
  SlcTypeModifierFlags modifiers;
} SlcQualifiedType;

typedef struct SlcAggregateMemberDescriptor {
  SlcTypeDescriptor    type;
  SlcTypeModifierFlags modifiers;
  SlcMemberOffset      offset;
  const char*          name;
} SlcAggregateMemberDescriptor;

typedef struct SlcEnumValue {
  const char* name;
  SlcUInt64   value;
} SlcEnumValue;


typedef struct SlcEnumInfo {
  const char*              pName;
  SlcSize                  nameLength;
  SlcSize                  valueCount;
  const SlcEnumValue*      pValues;
  SlcBool                  isBitflagType;
} SlcEnumInfo;

typedef struct SlcArrayInfo {
  SlcSize          elementCount;
  SlcQualifiedType elementType;
} SlcArrayInfo;

typedef struct SlcInterfaceMethodInfo {
  const char*              pName;
  SlcSize                  nameLength;
  SlcTypeDescriptor        returnType;
  SlcSize                  paramCount;
  const SlcTypeDescriptor* pParams;
} SlcInterfaceMethodInfo;

typedef struct SlcInterfaceInfo {
  const char*          pName;
  SlcSize              nameLength;
  SlcSize              parentCount;
  const SlcInterface*  pParents;
  SlcSize              methodCount;
  const SlcInterfaceMethodInfo* pMethods;
} SlcInterfaceInfo;

typedef struct SlcInterfaceModuleInfo {
  SlcSize             interfaceCount;
  const SlcInterface* pInterfaces;
} SlcInterfaceModuleInfo;



typedef struct SlcAggregateInfo {
  const char*                   pName;
  SlcSize                       nameLength;
  SlcSize                       memberCount;
  const SlcAggregateMemberInfo* pMembers;
  SlcSize                       alignment;
} SlcAggregateInfo;

typedef struct SlcMethodImplementationInfo {
  SlcUInt32   methodId;
  const char* name;
  SlcSize     nameLength;
  void (*     pFunction)();
  SlcSize     bufferSize;
  const void* pBuffer;
} SlcMethodInfo;

typedef struct SlcInterfaceImplmentationInfo {
  SlcInterface                       interface;
  SlcSize                            methodCount;
  const SlcMethodImplementationInfo* pMethods;
} SlcInterfaceImplmentationInfo;

typedef struct SlcArchetypeInfo {
  const char*                          pName;
  SlcSize                              nameLength;
  SlcSize                              alignment;
  SlcSize                              memberCount;
  const SlcAggregateMemberInfo*        pMembers;
  SlcSize                              interfaceCount;
  const SlcInterfaceImplmentationInfo* pInterfaces;
} SlcArchetypeInfo;

typedef struct SlcModuleInfo {
  const char* pName;
  SlcSize     nameLength;
} SlcModuleInfo;




JEM_api SlcError JEM_stdcall slcGetError(SlcRuntime runtime);

JEM_api SlcStatus JEM_stdcall slcCreateInterfaces(SlcRuntime runtime,
                                                  SlcSize interfaceCount,
                                                  SlcInterface* pInterface,
                                                  const SlcInterfaceInfo* pInterfaceInfo);

JEM_api SlcStatus JEM_stdcall slcCreateTypes(SlcRuntime runtime,
                                             SlcSize typeCount,
                                             SlcTypeDescriptor* pTypes,
                                             const Slc);












JEM_end_c_namespace


#if JEM_compiler_msvc
#pragma pop_macro("interface")
#endif


#endif//JEMSYS_SILICA2_H
