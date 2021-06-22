//
// Created by Maxwell on 2021-06-17.
//

#ifndef JEMSYS_AGATE_MPSC_MAILBOX_H
#define JEMSYS_AGATE_MPSC_MAILBOX_H

#include "mailbox.h"

JEM_begin_c_namespace


JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpsc(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpsc(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpsc(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpsc(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpsc(agt_mailbox_t mailbox, agt_message_t message);

JEM_api agt_status_t JEM_stdcall agt_start_send_message_mpsc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t JEM_stdcall agt_finish_send_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t JEM_stdcall agt_receive_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void         JEM_stdcall agt_discard_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t JEM_stdcall agt_cancel_message_mpsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);


JEM_end_c_namespace

#endif//JEMSYS_AGATE_MPSC_MAILBOX_H
