//
// Created by maxwe on 2021-08-30.
//

#include "silica.h"

#include "vector.hpp"
#include "dictionary.hpp"


#define NOMINMAX
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


extern "C" {

struct slc_module_builder {
  jem::vector<const void*>               dependencies;
  jem::vector<slc_function_descriptor_t> functions;
  jem::vector<slc_type_descriptor_t>     types;
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


  inline size_t alignToVirtualPage(size_t size) noexcept {
    static constexpr size_t VirtualPageSize = 1ULL << 16;
    return ((size - 1) | (VirtualPageSize - 1)) + 1;
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






  JEM_api void JEM_stdcall slc_create_module_builder(slc_module_builder_t* pModuleBuilder) {
    assert( pModuleBuilder != nullptr );
    *pModuleBuilder = new slc_module_builder{};
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










