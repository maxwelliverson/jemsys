//
// Created by maxwe on 2021-09-25.
//

#include <catch.hpp>

#include <silica.h>

slc_module_t emptyModule = []() noexcept {
  slc_module_t module;
  slc_module_builder_t moduleBuilder;

  slc_create_module_builder(&moduleBuilder);
  slc_build_module(&module, moduleBuilder);
  slc_destroy_module_builder(moduleBuilder);
  atexit([]{ slc_close_module(emptyModule); });

  return module;
}();

TEST_CASE("slc_module_query_attributes successfully queries attributes", "[silica][module]") {
  SECTION("Query 0 attributes") {
    slc_module_query_attributes(emptyModule, 0, nullptr, nullptr);
    SUCCEED("Yay!");
  }
  SECTION("Query 1 attribute") {
    size_t result;
    slc_attribute_t attribute = SLC_ATTRIBUTE_POINTER_SIZE;
    slc_module_query_attributes(emptyModule, 1, &attribute, &result);
    REQUIRE( result == sizeof(void*) );
  }
  SECTION("Query duplicate attributes") {
    using V = struct __virtual_inheritance V_;
    using VMP = int V::*;
    slc_attribute_t attributes[] = {
      SLC_ATTRIBUTE_POINTER_ALIGNMENT,
      SLC_ATTRIBUTE_VI_MEMBER_POINTER_SIZE,
      SLC_ATTRIBUTE_VI_MEMBER_POINTER_SIZE,
      SLC_ATTRIBUTE_POINTER_SIZE,
      SLC_ATTRIBUTE_VI_MEMBER_POINTER_SIZE,
    };
    auto results = (size_t*)(alloca(std::size(attributes) * sizeof(size_t)));
    slc_module_query_attributes(emptyModule, std::size(attributes), attributes, results);

    REQUIRE(results[0] == alignof(void*));
    REQUIRE(results[1] == sizeof(VMP));
    REQUIRE(results[2] == sizeof(VMP));
    REQUIRE(results[3] == sizeof(void*));
    REQUIRE(results[4] == sizeof(VMP));
  }
  SECTION("Query multiple attributes") {

    struct __virtual_inheritance V;
    struct __single_inheritance S;


    slc_attribute_t attributes[] = {
      SLC_ATTRIBUTE_POINTER_ALIGNMENT,
      SLC_ATTRIBUTE_VI_MEMBER_FUNCTION_POINTER_SIZE,
      SLC_ATTRIBUTE_ABI,
      SLC_ATTRIBUTE_SI_MEMBER_FUNCTION_POINTER_SIZE,
      SLC_ATTRIBUTE_ENDIANNESS
    };
    auto results = (size_t*)(alloca(std::size(attributes) * sizeof(size_t)));
    slc_module_query_attributes(emptyModule, std::size(attributes), attributes, results);

    REQUIRE(results[0] == alignof(void*));
    REQUIRE(results[1] == sizeof(int (V::*)()));
    REQUIRE(results[2] == SLC_ABI_MSVC_x64);
    REQUIRE(results[3] == sizeof(int (S::*)()));
    REQUIRE(results[4] == SLC_NATIVE_ENDIAN);
  }
}