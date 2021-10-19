//
// Created by maxwe on 2021-10-18.
//

#include <agate.h>
#include <quartz.h>

#include <cstring>

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

void test_create_mpmc_mailbox_sync() {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;



  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 4;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_mpmc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_mpsc_mailbox_sync() {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 4;
  params.max_receivers = 1;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_mpsc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_spmc_mailbox_sync() {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 4;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_spmc );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_spsc_mailbox_sync() {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_LOCAL;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.max_senders = 1;
  params.max_receivers = 1;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_spsc );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_private_mailbox_sync() {
  agt_mailbox_t mailbox;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_PRIVATE;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.async_handle_address = nullptr;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_private );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}


void test_create_mpmc_mailbox_async() {
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

  assert( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  assert( request != nullptr );
  assert( qtz_request_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_mpmc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_mpsc_mailbox_async() {
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

  assert( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  assert( qtz_request_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_mpsc );


  for (jem_u32_t senders = 0; senders < params.max_senders; ++senders) {
    assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_spmc_mailbox_async() {
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

  assert( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  assert( request != nullptr );
  assert( qtz_request_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_spmc );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );

  for (jem_u32_t receivers = 0; receivers < params.max_receivers; ++receivers) {
    assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  }

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_spsc_mailbox_async() {
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

  assert( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  assert( request != nullptr );
  assert( qtz_request_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_local_spsc );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

void test_create_private_mailbox_async() {
  agt_mailbox_t mailbox;
  qtz_request_t request;
  agt_create_mailbox_params_t params;

  params.scope = AGT_MAILBOX_SCOPE_PRIVATE;
  params.min_slot_size = JEM_CACHE_LINE;
  params.min_slot_count = 120;
  params.async_handle_address = &request;

  std::memset(params.name, 0, sizeof(params.name));

  assert( agt_create_mailbox(&mailbox, &params) == AGT_DEFERRED );
  assert( request != nullptr );
  assert( qtz_request_wait(request, JEM_WAIT) == QTZ_SUCCESS );
  assert( mailbox != nullptr );
  assert( mailbox_kind == mailbox_kind_private );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_SENDERS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );
  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_ERROR_TOO_MANY_RECEIVERS );

  agt_mailbox_detach_sender(mailbox);

  assert( agt_mailbox_attach_sender(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_mailbox_detach_receiver(mailbox);

  assert( agt_mailbox_attach_receiver(mailbox, JEM_DO_NOT_WAIT) == AGT_SUCCESS );

  agt_close_mailbox(mailbox);
}

int main() {
  test_create_mpmc_mailbox_sync();
  test_create_mpmc_mailbox_async();
  test_create_mpsc_mailbox_sync();
  test_create_mpsc_mailbox_async();
  test_create_spmc_mailbox_sync();
  test_create_spmc_mailbox_async();
  test_create_spsc_mailbox_sync();
  test_create_spsc_mailbox_async();
  test_create_private_mailbox_sync();
  test_create_private_mailbox_async();
}