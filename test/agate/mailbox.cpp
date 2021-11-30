//
// Created by maxwe on 2021-10-18.
//

#include <agate.h>
#include <quartz.h>

#include <catch.hpp>


#define mailbox_kind (*(((agt_mailbox_kind_t*)mailbox) + 1))

enum agt_mailbox_kind_t {
  mailbox_kind_local_spsc,
  mailbox_kind_local_spmc,
  mailbox_kind_local_mpsc,
  mailbox_kind_local_mpmc,
  mailbox_kind_shared_spsc,
  mailbox_kind_shared_spmc,
  mailbox_kind_shared_mpsc,
  mailbox_kind_shared_mpmc,
  mailbox_kind_private,
};

TEST_CASE("Create an MPMC mailbox synchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 4;
  params.async_handle_address = nullptr;

 std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_mpmc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an MPSC mailbox synchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 1;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_mpsc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an SPMC mailbox synchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 4;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_spmc );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an SPSC mailbox synchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 1;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_spsc );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create a private mailbox synchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_PRIVATE;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_private );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}


TEST_CASE("Create an MPMC mailbox asynchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 4;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  REQUIRE( request != nullptr );
  REQUIRE( qtz_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_mpmc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an MPSC mailbox asynchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 1;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  REQUIRE( qtz_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_mpsc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an SPMC mailbox asynchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 4;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  REQUIRE( request != nullptr );
  REQUIRE( qtz_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_spmc );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create an SPSC mailbox asynchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 1;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  REQUIRE( request != nullptr );
  REQUIRE( qtz_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_local_spsc );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

TEST_CASE("Create a private mailbox asynchronously", "[agate][mailbox]") {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_PRIVATE;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  REQUIRE( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  REQUIRE( request != nullptr );
  REQUIRE( qtz_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  REQUIRE( mailbox != nullptr );
  REQUIRE( mailbox_kind == mailbox_kind_private );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_SENDER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_DETACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  REQUIRE( agt_mailbox_connect(mailbox, AGT_ATTACH_RECEIVER, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}
