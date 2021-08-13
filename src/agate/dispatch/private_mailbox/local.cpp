//
// Created by maxwe on 2021-07-02.
//

#include <agate/dispatch/private_mailbox/local.h>

#include "../../internal.hpp"

using mailbox_t = agt::private_mailbox*;
using message_t = agt::local_message*;

namespace {
  JEM_forceinline void* message_to_payload(mailbox_t mailbox, message_t msg) noexcept {
    return static_cast<void*>(reinterpret_cast<std::byte*>(msg) + mailbox->payloadOffset);
  }
  JEM_forceinline message_t payload_to_message(mailbox_t mailbox, void* payload) noexcept {
    return reinterpret_cast<message_t>(reinterpret_cast<std::byte*>(payload) - mailbox->payloadOffset);
  }
}

extern "C" {

  JEM_api void*               JEM_stdcall agt_private_mailbox_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept {
    auto mailbox = cast<mailbox_t>(handle);

    assert( mailbox->availableSlotCount > 0 );

    auto message = mailbox->nextFreeSlot;
    mailbox->availableSlotCount -= 1;
    mailbox->nextFreeSlot = message->nextSlot;
    mailbox->prevAcquiredSlot = message;
    return message_to_payload(mailbox, message);
  }

  JEM_api void                JEM_stdcall agt_private_mailbox_release_slot(agt_handle_t handle, void* slot) JEM_noexcept {
    auto mailbox = cast<mailbox_t>(handle);

    auto message = payload_to_message(mailbox, slot);

    message->nextSlot = mailbox->nextFreeSlot;
    ++mailbox->availableSlotCount;

    mailbox->prevReleasedSlot = message;
  }
  JEM_api agt_message_t       JEM_stdcall agt_private_mailbox_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) JEM_noexcept {
    auto mailbox = cast<mailbox_t>(handle);
    auto message = payload_to_message(mailbox, messageSlot);

    mailbox->prevQueuedMessage->nextSlot = message;
    // FIXME: broken?
    ++mailbox->queuedMessageCount;
  }
  JEM_api agt_message_t       JEM_stdcall agt_private_mailbox_receive(agt_handle_t handle) JEM_noexcept;

  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept;
  JEM_api void                JEM_stdcall agt_private_mailbox_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept;
  JEM_api void                JEM_stdcall agt_private_mailbox_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept;


  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept;

  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept;


  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;


  JEM_api bool                JEM_stdcall agt_private_mailbox_discard(agt_message_t message) JEM_noexcept;
  JEM_api agt_status_t        JEM_stdcall agt_private_mailbox_cancel(agt_message_t message) JEM_noexcept;
  JEM_api void                JEM_stdcall agt_private_mailbox_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) JEM_noexcept;







}


