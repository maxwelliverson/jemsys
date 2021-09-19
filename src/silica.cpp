//
// Created by maxwe on 2021-08-30.
//

#include "silica.h"


extern "C" {

  struct slc_module_builder {

  };
  struct slc_module {

  };




  JEM_api void JEM_stdcall slc_open_module_builder(slc_module_builder_t* pModuleBuilder) {

  }
  JEM_api void JEM_stdcall slc_module_add_functions(slc_module_builder_t moduleBuilder, const slc_function_descriptor_t* pFunctions, jem_size_t functionCount) {

  }
  JEM_api void JEM_stdcall slc_module_add_types(slc_module_builder_t moduleBuilder, const slc_type_descriptor_t* pTypes, jem_size_t typeCount) {

  }
  JEM_api void JEM_stdcall slc_module_add_dependencies(slc_module_builder_t moduleBuilder, const slc_module_t* pDependencies, jem_size_t dependencyCount) {

  }




  JEM_api slc_status_t JEM_stdcall slc_build_module(slc_module_t* pModule, slc_module_builder_t builder) {

  }
  JEM_api void         JEM_stdcall slc_close_module(slc_module_t module) {

  }
  JEM_api slc_status_t JEM_stdcall slc_load_module(slc_module_t* pModule, const char* path) {

  }
  JEM_api slc_status_t JEM_stdcall slc_save_module(slc_module_t module, const char* path) {

  }

  JEM_api slc_status_t JEM_stdcall slc_module_enumerate_types(slc_module_t module, slc_type_entry_t* types, jem_size_t* typeCount) JEM_noexcept {

  }
  JEM_api slc_status_t JEM_stdcall slc_module_enumerate_functions(slc_module_t module, slc_function_entry_t* functions, jem_size_t* functionCount) JEM_noexcept {

  }


}










