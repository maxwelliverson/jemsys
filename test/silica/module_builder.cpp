//
// Created by maxwe on 2021-09-23.
//

#include <catch.hpp>

#include <silica.h>


TEST_CASE("Build a module", "[silica][module]") {
  slc_module_builder_t moduleBuilder;

  slc_create_module_builder(&moduleBuilder);

  SECTION("Open a module.") {
    REQUIRE( moduleBuilder != nullptr );
  }

  slc_destroy_module_builder(moduleBuilder);
}