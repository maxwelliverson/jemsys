//
// Created by maxwe on 2021-10-04.
//

#include "common.hpp"

using namespace agt;


const mailbox_vtable agt::vtable_table[] = {
  {
    .pfn_acquire_slot   = impl::local_spsc_acquire_slot,
    .pfn_send           = impl::local_spsc_send,
    .pfn_receive        = impl::local_spsc_receive,
    .pfn_connect        = impl::local_spsc_connect,
    .pfn_return_message = impl::local_spsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_spmc_acquire_slot,
    .pfn_send           = impl::local_spmc_send,
    .pfn_receive        = impl::local_spmc_receive,
    .pfn_connect        = impl::local_spmc_connect,
    .pfn_return_message = impl::local_spmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_mpsc_acquire_slot,
    .pfn_send           = impl::local_mpsc_send,
    .pfn_receive        = impl::local_mpsc_receive,
    .pfn_connect  = impl::local_mpsc_connect,
    .pfn_return_message = impl::local_mpsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::local_mpmc_acquire_slot,
    .pfn_send           = impl::local_mpmc_send,
    .pfn_receive        = impl::local_mpmc_receive,
    .pfn_connect  = impl::local_mpmc_connect,
    .pfn_return_message = impl::local_mpmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_spsc_acquire_slot,
    .pfn_send           = impl::shared_spsc_send,
    .pfn_receive        = impl::shared_spsc_receive,
    .pfn_connect  = impl::shared_spsc_connect,
    .pfn_return_message = impl::shared_spsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_spmc_acquire_slot,
    .pfn_send           = impl::shared_spmc_send,
    .pfn_receive        = impl::shared_spmc_receive,
    .pfn_connect  = impl::shared_spmc_connect,
    .pfn_return_message = impl::shared_spmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_mpsc_acquire_slot,
    .pfn_send           = impl::shared_mpsc_send,
    .pfn_receive        = impl::shared_mpsc_receive,
    .pfn_connect  = impl::shared_mpsc_connect,
    .pfn_return_message = impl::shared_mpsc_return_message
  },
  {
    .pfn_acquire_slot   = impl::shared_mpmc_acquire_slot,
    .pfn_send           = impl::shared_mpmc_send,
    .pfn_receive        = impl::shared_mpmc_receive,
    .pfn_connect  = impl::shared_mpmc_connect,
    .pfn_return_message = impl::shared_mpmc_return_message
  },
  {
    .pfn_acquire_slot   = impl::private_acquire_slot,
    .pfn_send           = impl::private_send,
    .pfn_receive        = impl::private_receive,
    .pfn_connect  = impl::private_connect,
    .pfn_return_message = impl::private_return_message
  }
};

extern "C" {

/*JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_sender(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_connect(mailbox, agt::MAILBOX_ROLE_SENDER, agt::MAILBOX_OP_KIND_ATTACH, timeout_us);
}
JEM_api void          JEM_stdcall agt_mailbox_detach_sender(agt_mailbox_t mailbox) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  agt::vtable_table[mailbox->kind].pfn_connect(mailbox, agt::MAILBOX_ROLE_SENDER, agt::MAILBOX_OP_KIND_DETACH, 0);
}
JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_receiver(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_connect(mailbox, agt::MAILBOX_ROLE_RECEIVER, agt::MAILBOX_OP_KIND_ATTACH, timeout_us);
}
JEM_api void          JEM_stdcall agt_mailbox_detach_receiver(agt_mailbox_t mailbox) JEM_noexcept {
  assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  agt::vtable_table[mailbox->kind].pfn_connect(mailbox, agt::MAILBOX_ROLE_RECEIVER, agt::MAILBOX_OP_KIND_DETACH, 0);
}*/

/*
JEM_api agt_mailslot_t    JEM_stdcall agt_mailbox_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_acquire_slot(mailbox, slot_size, timeout_us);
}
JEM_api agt_signal_t  JEM_stdcall agt_mailbox_send(agt_mailbox_t mailbox, agt_mailslot_t slot, agt_send_flags_t flags) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_send(mailbox, slot, flags);
}
JEM_api agt_cookie_t JEM_stdcall agt_mailbox_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return vtable_table[mailbox->kind].pfn_receive(mailbox, timeout_us);
}
*/


JEM_api agt_status_t   JEM_stdcall agt_mailbox_connect(agt_mailbox_t mailbox, agt_connect_action_t action, jem_u64_t timeout_us) JEM_noexcept {
  JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_connect(mailbox, action, timeout_us);
}
JEM_api agt_status_t   JEM_stdcall agt_mailbox_acquire_slot(agt_mailbox_t mailbox, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us) JEM_noexcept {
  JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_acquire_slot(mailbox, slot, slotSize, timeout_us);
}
JEM_api agt_signal_t   JEM_stdcall agt_mailbox_send(agt_mailbox_t mailbox, const agt_slot_t* slot, agt_send_flags_t flags) JEM_noexcept {
  JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_send(mailbox, slot, flags);
}
JEM_api agt_signal_t   JEM_stdcall agt_send(const agt_slot_t* slot, agt_agent_t sender, agt_send_flags_t flags) JEM_noexcept {
  // JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  agt_mailbox_t mailbox = to_mailbox(slot);
  /*if (sender == nullptr) [[likely]]
    sender = agt_self();*/
  slot->cookie->sender = sender ? sender : agt_self();
  JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_send(mailbox, slot, flags);
}
JEM_api agt_status_t   JEM_stdcall agt_mailbox_receive(agt_mailbox_t mailbox, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept {
  JEM_assert(mailbox->kind < MAILBOX_KIND_MAX_ENUM);
  return agt::vtable_table[mailbox->kind].pfn_receive(mailbox, message, timeout_us);
}


/*JEM_api void          JEM_stdcall agt_mailbox_release_slot(agt_mailbox_t mailbox, agt_mailslot_t slot) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  vtable_table[mailbox->kind].pfn_release_slot(mailbox, slot);
}*/
/*JEM_api agt_mailbox_vtable_t JEM_stdcall agt_mailbox_lookup_vtable(agt_mailbox_t mailbox) JEM_noexcept {
  assert( mailbox->kind < MAILBOX_KIND_MAX_ENUM );
  return (agt_mailbox_vtable_t)(vtable_table + mailbox->kind);
}*/



}
