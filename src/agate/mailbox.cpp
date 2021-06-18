//
// Created by maxwe on 2021-06-01.
//

#include "mailbox.h"

#include <agate/spsc_mailbox.h>
#include <agate/mpsc_mailbox.h>
#include <agate/spmc_mailbox.h>
#include <agate/mpmc_mailbox.h>

namespace agt::impl{
  namespace {
    inline constexpr PFN_start_send_message start_send_message_table[] = {
      agt_start_send_message_mpmc,
      agt_start_send_message_mpsc,
      agt_start_send_message_spmc,
      agt_start_send_message_spsc
    };
    inline constexpr PFN_finish_send_message finish_send_message_table[] = {
      agt_finish_send_message_mpmc,
      agt_finish_send_message_mpsc,
      agt_finish_send_message_spmc,
      agt_finish_send_message_spsc
    };
    inline constexpr PFN_receive_message receive_message_table[] = {
      agt_receive_message_mpmc,
      agt_receive_message_mpsc,
      agt_receive_message_spmc,
      agt_receive_message_spsc
    };
    inline constexpr PFN_discard_message discard_message_table[] = {
      agt_discard_message_mpmc,
      agt_discard_message_mpsc,
      agt_discard_message_spmc,
      agt_discard_message_spsc
    };
    inline constexpr PFN_cancel_message  cancel_message_table[] = {
      agt_cancel_message_mpmc,
      agt_cancel_message_mpsc,
      agt_cancel_message_spmc,
      agt_cancel_message_spsc
    };
  }
}


extern "C" {

JEM_api agt_status_t        JEM_stdcall agt_start_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us) {

}
JEM_api agt_message_t       JEM_stdcall agt_finish_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload) {

}
JEM_api agt_status_t        JEM_stdcall agt_receive_message(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us) {

}
JEM_api void                JEM_stdcall agt_discard_message(agt_mailbox_t mailbox, agt_message_t message) {

}
JEM_api agt_status_t        JEM_stdcall agt_cancel_message(agt_mailbox_t mailbox, agt_message_t message) {

}

}