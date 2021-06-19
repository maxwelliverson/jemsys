//
// Created by Maxwell on 2021-06-17.
//

#include <agate/spsc_mailbox.h>


extern "C" {

JEM_api agt_status_t  JEM_stdcall agt_start_send_message_spsc(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_message_t JEM_stdcall agt_finish_send_message_spsc(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload);
JEM_api agt_status_t  JEM_stdcall agt_receive_message_spsc(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void          JEM_stdcall agt_discard_message_spsc(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t  JEM_stdcall agt_cancel_message_spsc(agt_mailbox_t mailbox, agt_message_t message);

JEM_api agt_status_t  JEM_stdcall agt_start_send_message_spsc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_message_t JEM_stdcall agt_finish_send_message_spsc_dynamic(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload);
JEM_api agt_status_t  JEM_stdcall agt_receive_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void          JEM_stdcall agt_discard_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t  JEM_stdcall agt_cancel_message_spsc_dynamic(agt_mailbox_t mailbox, agt_message_t message);

}