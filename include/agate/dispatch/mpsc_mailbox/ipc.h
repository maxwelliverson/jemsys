//
// Created by Maxwell on 2021-06-24.
//

#ifndef JEMSYS_AGATE_DISPATCH_MPSC_MAILBOX_IPC_H
#define JEMSYS_AGATE_DISPATCH_MPSC_MAILBOX_IPC_H

#include <agate/mailbox.h>

JEM_begin_c_namespace

JEM_api void*               JEM_stdcall agt_ipc_mpsc_mailbox_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept;

JEM_api void                JEM_stdcall agt_ipc_mpsc_mailbox_release_slot(agt_handle_t handle, void* slot) JEM_noexcept;
JEM_api agt_message_t       JEM_stdcall agt_ipc_mpsc_mailbox_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_message_t       JEM_stdcall agt_ipc_mpsc_mailbox_receive(agt_handle_t handle) JEM_noexcept;

JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void                JEM_stdcall agt_ipc_mpsc_mailbox_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void                JEM_stdcall agt_ipc_mpsc_mailbox_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept;


JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept;

JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept;


JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;


JEM_api bool                JEM_stdcall agt_ipc_mpsc_mailbox_discard(agt_message_t message) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_ipc_mpsc_mailbox_cancel(agt_message_t message) JEM_noexcept;
JEM_api void                JEM_stdcall agt_ipc_mpsc_mailbox_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) JEM_noexcept;




JEM_end_c_namespace


#endif//JEMSYS_AGATE_DISPATCH_MPSC_MAILBOX_IPC_H
