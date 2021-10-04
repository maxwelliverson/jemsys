//
// Created by maxwe on 2021-08-30.
//

#include "silica.h"

#include "vector.hpp"
#include "dictionary.hpp"


#define NOMINMAX
#include <silica.h>
#include <windows.h>

namespace impl{
  namespace {
#if JEM_system_windows
    struct __single_inheritance S;
    struct __multiple_inheritance M;
    struct __virtual_inheritance V;
#else
    struct polymorphic_inheritance{
      int member;
      virtual void member_function();
    };
    struct S : polymorphic_inheritance{
      int  second_member;
      void second_member_function();
    };
    struct secondary_inheritance{
      int second_member;
      virtual void second_member_function();
    };
    struct M : polymorphic_inheritance, secondary_inheritance{};
    struct poly_a : virtual polymorphic_inheritance{
      void member_function() override;
    };
    struct poly_b : virtual polymorphic_inheritance{
      //void member_function() override;
    };
    struct V : poly_a, poly_b{ };
#endif
  }// namespace
}

namespace {

  enum class entry_kind{
    type,
    function,
    dependency,
    dependency_name
  };
  struct generic_module_entry_t{
    entry_kind kind;
    void*      entry;
  };
}

extern "C" {


struct slc_error {
  slc_status_t status;

};

struct slc_module_builder {
  jem::vector<slc_error_t>                    errors;
  jem::vector<const void*>                    dependencies;
  jem::vector<slc_function_descriptor_t*>     functions;
  jem::vector<slc_type_descriptor_t*>         types;

  jem::dictionary<generic_module_entry_t>     uniqueNamedEntries;
  jem::dictionary<slc_type_descriptor_t*>     namedTypes;
  jem::dictionary<slc_function_descriptor_t*> namedFunctions;

  jem::fixed_size_pool                        typePool;
  jem::fixed_size_pool                        functionPool;
};
struct slc_module {
  jem_u32_t  magicNumber; // 0x05111CA0
  alignas(size_t) slc_abi_t  abi;
  alignas(size_t) slc_endianness_t endianness;
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
  jem_size_t totalModuleSize;
  jem_size_t dependencyCount;
  jem_size_t functionCount;
  jem_size_t typeCount;
  jem_size_t functionOffset;
  jem_size_t typeOffset;
  jem_size_t textOffset;
  char       data[1];
};

}

namespace {

  inline constexpr struct slc_platform_constants_t {
    jem_size_t pointer_size = sizeof(void*);
    jem_size_t pointer_alignment = alignof(void*);
    jem_size_t si_member_pointer_size = sizeof(int impl::S::*);
    jem_size_t mi_member_pointer_size = sizeof(int impl::M::*);
    jem_size_t vi_member_pointer_size = sizeof(int impl::V::*);
    jem_size_t si_member_pointer_alignment = alignof(int impl::S::*);
    jem_size_t mi_member_pointer_alignment = alignof(int impl::M::*);
    jem_size_t vi_member_pointer_alignment = alignof(int impl::V::*);
    jem_size_t si_member_function_pointer_size = sizeof(int (impl::S::*)());
    jem_size_t mi_member_function_pointer_size = sizeof(int (impl::M::*)());
    jem_size_t vi_member_function_pointer_size = sizeof(int (impl::V::*)());
    jem_size_t si_member_function_pointer_alignment = alignof(int (impl::S::*)());
    jem_size_t mi_member_function_pointer_alignment = alignof(int (impl::M::*)());
    jem_size_t vi_member_function_pointer_alignment = alignof(int (impl::V::*)());
  } platform_constants;

  inline constexpr jem_u32_t ModuleMagicNumber = 0x05111CA0;

  inline constexpr const unsigned char LittleEndianModuleString[] = { 0x05, 0x11, 0x1C, 0xA0 };
  inline constexpr const unsigned char BigEndianModuleString[]    = { 0xA0, 0x1C, 0x11, 0x05 };

  enum class dependency {
    module_name,
    module_little_endian,
    module_big_endian
  };

  inline dependency kind_of_dependency(const void* dep) noexcept {
    assert( dep != nullptr );
    const char* depStr = static_cast<const char*>(dep);
    if ( memcmp(dep, LittleEndianModuleString, sizeof(LittleEndianModuleString)) == 0 )
      return dependency::module_little_endian;
    if ( memcmp(dep, BigEndianModuleString, sizeof(BigEndianModuleString)) == 0 )
      return dependency::module_big_endian;
    return dependency::module_name;
  }

  class intermediate_module_builder {
    jem::dictionary<jem::vector<size_t*, 0>> stringPoolAndLocations;
    // jem::dictionary<>
  public:


  };

  inline void align_offset(jem_size_t& offset, jem_size_t alignment) noexcept {
    const jem_size_t lower_mask = alignment - 1;
    --offset;
    offset |= lower_mask;
    ++offset;
  }
  inline void update_alignment(jem_size_t& alignment, jem_size_t newAlign) noexcept {
    alignment = std::max(alignment, newAlign);
  }


  inline size_t alignToVirtualPage(size_t size) noexcept {
    static constexpr size_t VirtualPageSize = 1ULL << 16;
    return ((size - 1) | (VirtualPageSize - 1)) + 1;
  }

  template <typename T>
  inline T* allocate_array(size_t count) noexcept {
    if ( count == 0 )
      return nullptr;
    return (T*)calloc(count, sizeof(T));
  }

  inline slc_type_descriptor_t* lookup_type(slc_module_builder_t moduleBuilder, const slc_type_params_t& params) noexcept;

  inline void init_type(slc_module_builder_t moduleBuilder, slc_type_descriptor_t& type, const slc_type_params_t& params) noexcept {
    type.kind = params.kind;
    switch ( params.kind ) {
      case SLC_TYPE_KIND_VOID:
        type.size = 0;
        type.alignment = 0;
        break;
      case SLC_TYPE_KIND_BOOLEAN:
        type.size = 1;
        type.alignment = 1;
        break;
      case SLC_TYPE_KIND_INTEGRAL: {
        const auto& args        = *static_cast<const slc_integral_type_params_t*>(params.params);
        type.size               = args.size;
        type.alignment          = args.alignment;
        type.integral.name      = _strdup(args.name);
        type.integral.is_signed = args.is_signed;
      }
        break;
      case SLC_TYPE_KIND_FLOATING_POINT: {
        const auto& args   = *static_cast<const slc_floating_point_type_params_t*>(params.params);
        type.size                         = args.size;
        type.alignment                    = args.alignment;
        type.floating_point.is_signed     = args.is_signed;
        type.floating_point.exponent_bits = args.exponent_bits;
        type.floating_point.mantissa_bits = args.mantissa_bits;
        type.floating_point.name          = _strdup(args.name);
      }
        break;
      case SLC_TYPE_KIND_ALIAS: {
        const auto& args     = *static_cast<const slc_alias_type_params_t*>(params.params);
        type.size            = args.target.type->size;
        type.alignment       = args.target.type->alignment;
        type.alias.target    = args.target;
        type.alias.name      = _strdup(args.name);
      }
        break;
      case SLC_TYPE_KIND_POINTER: {
        const auto& args     = *static_cast<const slc_pointer_type_params_t*>(params.params);
        type.size            = platform_constants.pointer_size;
        type.alignment       = platform_constants.pointer_alignment;
        type.pointer.pointee = args.target;
      }
        break;
      case SLC_TYPE_KIND_ARRAY: {
        const auto& args     = *static_cast<const slc_array_type_params_t*>(params.params);
        type.size            = args.count == 0 ? 0 : (args.elements.type->size * args.count);
        type.alignment       = args.elements.type->alignment;
        type.array.size      = args.count;
        type.array.element   = args.elements;
      }
        break;
      case SLC_TYPE_KIND_FUNCTION: {
        const auto& args     = *static_cast<const slc_function_type_params_t*>(params.params);
        type.size               = 0;
        type.alignment          = 1;
        type.function.flags     = args.flags;
        type.function.args      = args.args;
        type.function.arg_count = args.arg_count;
        type.function.ret       = args.ret;
        type.function.callconv  = args.callconv;
      }
        break;
      case SLC_TYPE_KIND_MEMBER_POINTER: {
        const auto& args     = *static_cast<const slc_member_pointer_type_params_t*>(params.params);
        type.size            = *((&platform_constants.si_member_pointer_size) + args.aggregate_type->aggregate.inheritance_kind);
        type.alignment       = *((&platform_constants.si_member_pointer_alignment) + args.aggregate_type->aggregate.inheritance_kind);
        type.member_pointer.aggregate_type = args.aggregate_type;
        type.member_pointer.pointee        = args.pointee;
      }
        break;
      case SLC_TYPE_KIND_MEMBER_FUNCTION_POINTER: {
        const auto& args     = *static_cast<const slc_member_function_pointer_type_params_t*>(params.params);
        type.size            = *((&platform_constants.si_member_function_pointer_size) + args.aggregate_type->aggregate.inheritance_kind);
        type.alignment       = *((&platform_constants.si_member_function_pointer_alignment) + args.aggregate_type->aggregate.inheritance_kind);
        type.member_function_pointer.aggregate_type = args.aggregate_type;
        type.member_function_pointer.function       = args.function;
      }
        break;
      case SLC_TYPE_KIND_AGGREGATE: {
        const auto& args     = *static_cast<const slc_aggregate_type_params_t*>(params.params);
        type.aggregate.flags = args.flags;
        type.aggregate.name  = _strdup(args.name);
        jem_size_t  offset = 0;
        jem_size_t  bitfield_offset = 0;


        const jem_size_t baseCount   = args.base_count;
        const jem_size_t memberCount = args.member_count;
        auto* const      basePtr     = allocate_array<slc_aggregate_base_descriptor_t>(baseCount);
        auto* const      memberPtr   = allocate_array<slc_aggregate_member_descriptor_t>(memberCount);

        if ( baseCount > 1 ) {
          type.aggregate.inheritance_kind = SLC_MULTI_INHERITANCE;
        }
        else {
          type.aggregate.inheritance_kind = SLC_SINGLE_INHERITANCE;
        }

        for ( jem_size_t i = 0; i < baseCount;   ++i ) {
          basePtr[i].type   = args.bases[i].type;
          basePtr[i].flags  = args.bases[i].flags;

          align_offset(offset, args.bases[i].type->alignment);

          basePtr[i].offset = offset;
          offset += args.bases[i].type->size;
          update_alignment(type.alignment, args.bases[i].type->alignment);
          if ( basePtr[i].flags & SLC_VIRTUAL_INHERITANCE )
            type.aggregate.inheritance_kind = SLC_VIRTUAL_INHERITANCE;
          else
            type.aggregate.inheritance_kind = std::max(type.aggregate.inheritance_kind, basePtr[i].type->aggregate.inheritance_kind);
        }

        if ( (type.aggregate.flags & SLC_TYPE_IS_VIRTUAL) && offset == 0 ) {
          // vptr
          offset += sizeof(void*);
          update_alignment(type.alignment, alignof(void*));
        }

        for ( jem_size_t i = 0; i < memberCount; ++i ) {
          const jem_size_t member_alignment = args.members[i]->alignment ? args.members[i]->alignment : args.members[i]->type->alignment;
          const jem_size_t type_bits        = args.members[i]->type->size * CHAR_BIT;

          memberPtr[i].name            = _strdup(args.members[i]->name);
          memberPtr[i].type            = args.members[i]->type;
          memberPtr[i].flags           = args.members[i]->flags;

          if ( args.members[i]->flags & SLC_TYPE_IS_BITFIELD ) {
            memberPtr[i].bitfield_bits   = args.members[i]->bitfield_bits;
            if ( bitfield_offset + memberPtr[i].bitfield_bits > type_bits ) {
              bitfield_offset = 0;
              align_offset(offset, member_alignment);
              memberPtr[i].offset = offset;
              offset += memberPtr[i].bitfield_bits / CHAR_BIT;
            }

            memberPtr[i].bitfield_offset = bitfield_offset;
            bitfield_offset += memberPtr[i].bitfield_bits;
          }
          else {
            align_offset(offset, member_alignment);
            memberPtr[i].offset = offset;
            offset += memberPtr[i].type->size;
            memberPtr[i].bitfield_bits = 0;
            memberPtr[i].bitfield_offset = 0;
            bitfield_offset = 0;
          }

          update_alignment(type.alignment, member_alignment);
        }

        align_offset(offset, type.alignment);
        type.size = offset;



        type.aggregate.bases        = basePtr;
        type.aggregate.base_count   = baseCount;

        type.aggregate.member_count = memberCount;
        type.aggregate.members      = memberPtr;
      }
        break;
      case SLC_TYPE_KIND_ENUMERATOR: {
        const auto& args     = *static_cast<const slc_enumerator_type_params_t*>(params.params);
        type.size                       = args.underlying_type->size;
        type.alignment                  = args.underlying_type->alignment;
        type.enumerator.underlying_type = args.underlying_type;
        type.enumerator.name            = _strdup(args.name);
        type.enumerator.is_bitflag      = args.is_bitflag;

        const jem_size_t entryCount = args.entry_count;
        auto* entryPtr = allocate_array<slc_enumeration_descriptor_t>(entryCount);

        for ( jem_size_t i = 0; i < entryCount; ++i ) {
          entryPtr[i].value = args.entries[i].value;
          entryPtr[i].name  = _strdup(args.entries[i].name);
        }

        type.enumerator.entries = entryPtr;
        type.enumerator.entry_count = entryCount;
      }
        break;
      JEM_no_default;
    }
  }

  inline slc_type_descriptor_t* create_type(slc_module_builder_t moduleBuilder, const slc_type_params_t& params) noexcept {
    auto* desc = (slc_type_descriptor_t*)moduleBuilder->typePool.alloc_block();
    init_type(moduleBuilder, *desc, params);
    return desc;
  }

  inline slc_type_descriptor_t* lookup_type(slc_module_builder_t moduleBuilder, const slc_type_params_t& params) noexcept {
    switch ( params.kind ) {
      case SLC_TYPE_KIND_VOID: {
        auto& voidType = moduleBuilder->namedTypes["void"];
        if ( !voidType )
          voidType = create_type(moduleBuilder, params);
        return voidType;
      }
        break;
      case SLC_TYPE_KIND_BOOLEAN: {
        auto& boolType = moduleBuilder->namedTypes["bool"];
        if ( !boolType )
          boolType = create_type(moduleBuilder, params);
        return boolType;
      }
        break;

      case SLC_TYPE_KIND_INTEGRAL:
      case SLC_TYPE_KIND_FLOATING_POINT:
      case SLC_TYPE_KIND_ALIAS:
      case SLC_TYPE_KIND_AGGREGATE:
      case SLC_TYPE_KIND_ENUMERATOR: {
        auto& type = moduleBuilder->namedTypes[ *static_cast<const char* const *>(params.params) ];
        if ( !type )
          type = create_type(moduleBuilder, params);
        else {

        }
      }
        break;


      case SLC_TYPE_KIND_POINTER:
      case SLC_TYPE_KIND_ARRAY:
      case SLC_TYPE_KIND_FUNCTION:
      case SLC_TYPE_KIND_MEMBER_POINTER:
      case SLC_TYPE_KIND_MEMBER_FUNCTION_POINTER:
        break;

    }
  }

  /*class slc_type{
  public:
    virtual
  };*/
}

extern "C" {


  union slc_dependency_desc_t{
    const char*  name;
    slc_module_t handle;
  };


  static_assert(sizeof(slc_type_descriptor_t) == 72);



  JEM_api void JEM_stdcall slc_create_module_builder(slc_module_builder_t* pModuleBuilder) {
    assert( pModuleBuilder != nullptr );
    *pModuleBuilder = new slc_module_builder{
      .typePool     = { sizeof(slc_type_descriptor_t), JEM_VIRTUAL_PAGE_SIZE / sizeof(slc_type_descriptor_t) },
      .functionPool = { sizeof(slc_function_descriptor_t), JEM_VIRTUAL_PAGE_SIZE / sizeof(slc_function_descriptor_t) }
    };
  }
  JEM_api void JEM_stdcall slc_destroy_module_builder(slc_module_builder_t moduleBuilder) {
    delete moduleBuilder;
  }
  JEM_api void JEM_stdcall slc_module_add_functions(slc_module_builder_t moduleBuilder, const slc_function_descriptor_t* pFunctions, jem_size_t functionCount) {

  }
  JEM_api void JEM_stdcall slc_module_add_types(slc_module_builder_t moduleBuilder, const slc_type_descriptor_t* pTypes, jem_size_t typeCount) {

  }
  JEM_api void JEM_stdcall slc_module_add_dependencies(slc_module_builder_t moduleBuilder, const void* pDependencies, jem_size_t dependencyCount) {

  }




  JEM_api slc_status_t JEM_stdcall slc_build_module(slc_module_t* pModule, slc_module_builder_t builder) {
    intermediate_module_builder intermediateModuleBuilder;

    // ...

    // TODO: Define implementation of slc_build_module
    size_t moduleSize = sizeof(slc_module);

    // ...

    void* moduleMemory = VirtualAlloc2(INVALID_HANDLE_VALUE, nullptr, alignToVirtualPage(moduleSize), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE, nullptr, 0);

    if ( !moduleMemory )
      return SLC_ERROR_INTERNAL;

    auto module = new (moduleMemory) slc_module {
      .magicNumber                          = 0x05111CA0,
      .abi                                  = SLC_ABI_MSVC,
      .endianness                           = SLC_NATIVE_ENDIAN,
      .pointer_size                         = sizeof(void*),
      .pointer_alignment                    = alignof(void*),
      .si_member_pointer_size               = sizeof(int impl::S::*),
      .mi_member_pointer_size               = sizeof(int impl::M::*),
      .vi_member_pointer_size               = sizeof(int impl::V::*),
      .si_member_pointer_alignment          = alignof(int impl::S::*),
      .mi_member_pointer_alignment          = alignof(int impl::M::*),
      .vi_member_pointer_alignment          = alignof(int impl::V::*),
      .si_member_function_pointer_size      = sizeof(int (impl::S::*)()),
      .mi_member_function_pointer_size      = sizeof(int (impl::M::*)()),
      .vi_member_function_pointer_size      = sizeof(int (impl::V::*)()),
      .si_member_function_pointer_alignment = alignof(int (impl::S::*)()),
      .mi_member_function_pointer_alignment = alignof(int (impl::M::*)()),
      .vi_member_function_pointer_alignment = alignof(int (impl::V::*)()),
      .totalModuleSize                      = moduleSize,
      .dependencyCount                      = 0,
      .functionCount                        = 0,
      .typeCount                            = 0,
      .functionOffset                       = 0,
      .typeOffset                           = 0,
      .textOffset                           = 0
    };

    // ...

    DWORD prevProt;
    VirtualProtect(module, moduleSize, PAGE_EXECUTE_READ, &prevProt);

    *pModule = module;

    return SLC_SUCCESS;
  }
  JEM_api void         JEM_stdcall slc_close_module(slc_module_t module) {
    if ( module ) {
      DWORD prevProt;
      void* moduleMemory = module;
      size_t moduleSize  = module->totalModuleSize;
      VirtualProtect(module, moduleSize, PAGE_READWRITE, &prevProt);
      module->~slc_module();
      VirtualFree(module, 0/*alignToVirtualPage(moduleSize)*/, MEM_RELEASE);
    }
  }
  JEM_api slc_status_t JEM_stdcall slc_load_module(slc_module_t* pModule, const char* path) {
    return SLC_ERROR_UNKNOWN;
  }
  JEM_api slc_status_t JEM_stdcall slc_save_module(slc_module_t module, const char* path) {
    //Ope
    return SLC_ERROR_UNKNOWN;
  }

  JEM_api void         JEM_stdcall slc_module_query_attributes(slc_module_t module, size_t moduleCount, const slc_attribute_t* attributes, size_t* results) {

    if ( moduleCount != 0 ) {
      assert( attributes != nullptr );
      assert( results != nullptr );
      assert( module != nullptr );
    }

    for ( size_t i = 0; i < moduleCount; ++i )
      results[i] = ((size_t*)(&module->abi))[attributes[i]];
  }

  JEM_api slc_status_t JEM_stdcall slc_module_enumerate_types(slc_module_t module, slc_type_entry_t* types, jem_size_t* typeCount) JEM_noexcept {
    return SLC_ERROR_UNKNOWN;
  }
  JEM_api slc_status_t JEM_stdcall slc_module_enumerate_functions(slc_module_t module, slc_function_entry_t* functions, jem_size_t* functionCount) JEM_noexcept {
    return SLC_ERROR_UNKNOWN;
  }


}










