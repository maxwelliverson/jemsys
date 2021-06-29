//
// Created by Maxwell on 2021-06-17.
//

#ifndef JEMSYS_AGATE_MPMC_MAILBOX_H
#define JEMSYS_AGATE_MPMC_MAILBOX_H

#include "mailbox.h"

JEM_begin_c_namespace

/*JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpmc(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpmc(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpmc(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpmc(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpmc(agt_mailbox_t mailbox, agt_message_t message);

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpmc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpmc_dynamic(agt_mailbox_t mailbox, agt_message_t message);*/




JEM_api void*               JEM_stdcall agt_acquire_slot_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize);
JEM_api void                JEM_stdcall agt_release_slot_dynamic_local_mpmc_mailbox(agt_handle_t handle, void* slot);
JEM_api agt_message_t       JEM_stdcall agt_send_dynamic_local_mpmc_mailbox(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags);
JEM_api agt_message_t       JEM_stdcall agt_receive_dynamic_local_mpmc_mailbox(agt_handle_t handle);

JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_release_many_slots_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_send_many_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags);
JEM_api agt_status_t        JEM_stdcall agt_receive_many_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t count, agt_message_t* messages);


JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_dynamic_local_mpmc_mailbox(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us);

JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_many_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us);


JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex_dynamic_local_mpmc_mailbox(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex_dynamic_local_mpmc_mailbox(agt_handle_t handle, const agt_release_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_send_ex_dynamic_local_mpmc_mailbox(agt_handle_t handle, const agt_send_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_receive_ex_dynamic_local_mpmc_mailbox(agt_handle_t handle, const agt_receive_ex_params_t* params);


JEM_api bool                JEM_stdcall agt_discard_dynamic_local_mpmc_mailbox(agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_dynamic_local_mpmc_mailbox(agt_message_t message);
JEM_api void                JEM_stdcall agt_query_attributes_dynamic_local_mpmc_mailbox(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes);


JEM_api void*               JEM_stdcall agt_acquire_slot_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize);
JEM_api void                JEM_stdcall agt_release_slot_ipc_mpmc_mailbox(agt_handle_t handle, void* slot);
JEM_api agt_message_t       JEM_stdcall agt_send_ipc_mpmc_mailbox(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags);
JEM_api agt_message_t       JEM_stdcall agt_receive_ipc_mpmc_mailbox(agt_handle_t handle);

JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_release_many_slots_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_send_many_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags);
JEM_api agt_status_t        JEM_stdcall agt_receive_many_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t count, agt_message_t* messages);


JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_ipc_mpmc_mailbox(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us);

JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_many_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us);


JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex_ipc_mpmc_mailbox(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex_ipc_mpmc_mailbox(agt_handle_t handle, const agt_release_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_send_ex_ipc_mpmc_mailbox(agt_handle_t handle, const agt_send_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_receive_ex_ipc_mpmc_mailbox(agt_handle_t handle, const agt_receive_ex_params_t* params);


JEM_api bool                JEM_stdcall agt_discard_ipc_mpmc_mailbox(agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_ipc_mpmc_mailbox(agt_message_t message);
JEM_api void                JEM_stdcall agt_query_attributes_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes);



JEM_api void*               JEM_stdcall agt_acquire_slot_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize);
JEM_api void                JEM_stdcall agt_release_slot_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, void* slot);
JEM_api agt_message_t       JEM_stdcall agt_send_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags);
JEM_api agt_message_t       JEM_stdcall agt_receive_dynamic_ipc_mpmc_mailbox(agt_handle_t handle);

JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_release_many_slots_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, void** slots);
JEM_api void                JEM_stdcall agt_send_many_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags);
JEM_api agt_status_t        JEM_stdcall agt_receive_many_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t count, agt_message_t* messages);


JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us);

JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_try_receive_many_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us);


JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, const agt_release_slot_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_send_ex_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, const agt_send_ex_params_t* params);
JEM_api agt_status_t        JEM_stdcall agt_receive_ex_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, const agt_receive_ex_params_t* params);


JEM_api bool                JEM_stdcall agt_discard_dynamic_ipc_mpmc_mailbox(agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_dynamic_ipc_mpmc_mailbox(agt_message_t message);
JEM_api void                JEM_stdcall agt_query_attributes_dynamic_ipc_mpmc_mailbox(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes);




JEM_end_c_namespace

#endif//JEMSYS_AGATE_MPMC_MAILBOX_H
