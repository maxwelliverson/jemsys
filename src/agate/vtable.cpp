//
// Created by maxwe on 2021-10-04.
//

#include "common.hpp"

using namespace agt;


/*
mailbox_kind_local_spsc,
mailbox_kind_local_spmc,
mailbox_kind_local_mpsc,
mailbox_kind_local_mpmc,
mailbox_kind_shared_spsc,
mailbox_kind_shared_spmc,
mailbox_kind_shared_mpsc,
mailbox_kind_shared_mpmc,
mailbox_kind_private
 */

/*
 * pfn_acquire_slot
 * pfn_release_slot
 * pfn_send
 * pfn_receive
 * pfn_attach
 * pfn_detach
 * pfn_return_message
 * */

const mailbox_vtable agt::vtable_table[] = {
  {
    .pfn_acquire_slot   = impl::local_spsc_acquire_slot,
    .pfn_release_slot   = impl::local_spsc_release_slot,
    .pfn_send           = impl::local_spsc_send,
    .pfn_receive        = impl::local_spsc_receive,
    .pfn_attach         = impl::local_spsc_attach,
    .pfn_detach         = impl::local_spsc_detach,
    .pfn_return_message = impl::local_spsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_spmc_acquire_slot,
    .pfn_release_slot   = impl::local_spmc_release_slot,
    .pfn_send           = impl::local_spmc_send,
    .pfn_receive        = impl::local_spmc_receive,
    .pfn_attach         = impl::local_spmc_attach,
    .pfn_detach         = impl::local_spmc_detach,
    .pfn_return_message = impl::local_spmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_mpsc_acquire_slot,
    .pfn_release_slot   = impl::local_mpsc_release_slot,
    .pfn_send           = impl::local_mpsc_send,
    .pfn_receive        = impl::local_mpsc_receive,
    .pfn_attach         = impl::local_mpsc_attach,
    .pfn_detach         = impl::local_mpsc_detach,
    .pfn_return_message = impl::local_mpsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_mpmc_acquire_slot,
    .pfn_release_slot   = impl::local_mpmc_release_slot,
    .pfn_send           = impl::local_mpmc_send,
    .pfn_receive        = impl::local_mpmc_receive,
    .pfn_attach         = impl::local_mpmc_attach,
    .pfn_detach         = impl::local_mpmc_detach,
    .pfn_return_message = impl::local_mpmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_spsc_acquire_slot,
    .pfn_release_slot   = impl::shared_spsc_release_slot,
    .pfn_send           = impl::shared_spsc_send,
    .pfn_receive        = impl::shared_spsc_receive,
    .pfn_attach         = impl::shared_spsc_attach,
    .pfn_detach         = impl::shared_spsc_detach,
    .pfn_return_message = impl::shared_spsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_spmc_acquire_slot,
    .pfn_release_slot   = impl::shared_spmc_release_slot,
    .pfn_send           = impl::shared_spmc_send,
    .pfn_receive        = impl::shared_spmc_receive,
    .pfn_attach         = impl::shared_spmc_attach,
    .pfn_detach         = impl::shared_spmc_detach,
    .pfn_return_message = impl::shared_spmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_mpsc_acquire_slot,
    .pfn_release_slot   = impl::shared_mpsc_release_slot,
    .pfn_send           = impl::shared_mpsc_send,
    .pfn_receive        = impl::shared_mpsc_receive,
    .pfn_attach         = impl::shared_mpsc_attach,
    .pfn_detach         = impl::shared_mpsc_detach,
    .pfn_return_message = impl::shared_mpsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_mpmc_acquire_slot,
    .pfn_release_slot   = impl::shared_mpmc_release_slot,
    .pfn_send           = impl::shared_mpmc_send,
    .pfn_receive        = impl::shared_mpmc_receive,
    .pfn_attach         = impl::shared_mpmc_attach,
    .pfn_detach         = impl::shared_mpmc_detach,
    .pfn_return_message = impl::shared_mpmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::private_acquire_slot,
    .pfn_release_slot   = impl::private_release_slot,
    .pfn_send           = impl::private_send,
    .pfn_receive        = impl::private_receive,
    .pfn_attach         = impl::private_attach,
    .pfn_detach         = impl::private_detach,
    .pfn_return_message = impl::private_return_message
  }
};

extern "C" {

JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_sender(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_attach(mailbox, true, timeout_us);
}
JEM_api void          JEM_stdcall agt_mailbox_detach_sender(agt_mailbox_t mailbox) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  agt::vtable_table[mailbox->kind].pfn_detach(mailbox, true);
}
JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_receiver(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_attach(mailbox, false, timeout_us);
}
JEM_api void          JEM_stdcall agt_mailbox_detach_receiver(agt_mailbox_t mailbox) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  agt::vtable_table[mailbox->kind].pfn_detach(mailbox, false);
}

JEM_api agt_slot_t    JEM_stdcall agt_mailbox_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_acquire_slot(mailbox, slot_size, timeout_us);
}
JEM_api void          JEM_stdcall agt_mailbox_release_slot(agt_mailbox_t mailbox, agt_slot_t slot) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  vtable_table[mailbox->kind].pfn_release_slot(mailbox, slot);
}
JEM_api agt_signal_t  JEM_stdcall agt_mailbox_send(agt_mailbox_t mailbox, agt_slot_t slot) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_send(mailbox, slot);
}
JEM_api agt_message_t JEM_stdcall agt_mailbox_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_receive(mailbox, timeout_us);
}

JEM_api agt_mailbox_vtable_t JEM_stdcall agt_mailbox_lookup_vtable(agt_mailbox_t mailbox) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return (agt_mailbox_vtable_t)(vtable_table + mailbox->kind);
}



}
