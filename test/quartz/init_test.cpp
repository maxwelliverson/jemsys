//
// Created by maxwe on 2021-09-22.
//

#include <catch.hpp>

#include <quartz/core.h>

#include <thread>
#include <barrier>
#include <latch>

inline std::vector<qtz_status_t> init_on_many_threads(jem_size_t N) noexcept {

  std::vector<std::thread>  threads;
  std::vector<qtz_status_t> statuses;
  std::barrier<>            threadBarrier{(ptrdiff_t)N};

  threads.reserve(N);
  statuses.resize(N);

  for ( auto& status : statuses ) {
    threads.emplace_back([&]() mutable {
      qtz_init_params_t initParams{
        .kernel_version     = 0,
        .kernel_mode        = QTZ_KERNEL_INIT_CREATE_NEW,
        .kernel_access_code = "catch2-test-kernel",
        .message_slot_count = 2047,
        .process_name       = "Multithreaded Init Test Proc",
        .module_count       = 0,
        .modules            = nullptr
      };

      threadBarrier.arrive_and_wait();

      status = qtz_init(&initParams);

    });
  }
  for ( auto&& thread : threads )
    thread.join();

  return statuses;
}

TEST_CASE("Jemsys.Quartz initialization is thread-safe", "[quartz][init][threads]") {

  using Catch::Matchers::Equals;

  /*qtz_init_params_t initParams;
  initParams.message_slot_count = 2047;
  initParams.process_name       = "test process case 1";
  initParams.module_count       = 0;
  initParams.modules            = nullptr;
  initParams.kernel_version     = 0;
  initParams.kernel_mode        = QTZ_KERNEL_INIT_CREATE_NEW;
  initParams.kernel_access_code = "catch2-test-kernel";

  REQUIRE( qtz_init(&initParams) == QTZ_SUCCESS );*/



  REQUIRE_THAT( init_on_many_threads(8), Equals(std::vector{8, QTZ_SUCCESS}) );


  struct log_message{
    size_t structSize;
  };

}