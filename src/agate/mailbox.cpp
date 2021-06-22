//
// Created by maxwe on 2021-06-01.
//

#include "internal.hpp"

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
      agt_start_send_message_spsc,
      agt_start_send_message_mpmc_dynamic,
      agt_start_send_message_mpsc_dynamic,
      agt_start_send_message_spmc_dynamic,
      agt_start_send_message_spsc_dynamic
    };
    inline constexpr PFN_finish_send_message finish_send_message_table[] = {
      agt_finish_send_message_mpmc,
      agt_finish_send_message_mpsc,
      agt_finish_send_message_spmc,
      agt_finish_send_message_spsc,
      agt_finish_send_message_mpmc_dynamic,
      agt_finish_send_message_mpsc_dynamic,
      agt_finish_send_message_spmc_dynamic,
      agt_finish_send_message_spsc_dynamic
    };
    inline constexpr PFN_receive_message receive_message_table[] = {
      agt_receive_message_mpmc,
      agt_receive_message_mpsc,
      agt_receive_message_spmc,
      agt_receive_message_spsc,
      agt_receive_message_mpmc_dynamic,
      agt_receive_message_mpsc_dynamic,
      agt_receive_message_spmc_dynamic,
      agt_receive_message_spsc_dynamic
    };
    inline constexpr PFN_discard_message discard_message_table[] = {
      agt_discard_message_mpmc,
      agt_discard_message_mpsc,
      agt_discard_message_spmc,
      agt_discard_message_spsc,
      agt_discard_message_mpmc_dynamic,
      agt_discard_message_mpsc_dynamic,
      agt_discard_message_spmc_dynamic,
      agt_discard_message_spsc_dynamic
    };
    inline constexpr PFN_cancel_message  cancel_message_table[] = {
      agt_cancel_message_mpmc,
      agt_cancel_message_mpsc,
      agt_cancel_message_spmc,
      agt_cancel_message_spsc,
      agt_cancel_message_mpmc_dynamic,
      agt_cancel_message_mpsc_dynamic,
      agt_cancel_message_spmc_dynamic,
      agt_cancel_message_spsc_dynamic
    };
  }
}


extern "C" {

  JEM_api agt_request_t       JEM_stdcall agt_mailbox_create(agt_mailbox_t* mailbox, const agt_mailbox_create_info_t* createInfo);
  JEM_api void                JEM_stdcall agt_mailbox_destroy(agt_mailbox_t mailbox);

  JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us) {
    switch (mailbox->flags & (AGT_MAILBOX_SINGLE_PRODUCER | AGT_MAILBOX_SINGLE_CONSUMER)) {
      case 0:
        if ( static_cast<agt::mpmc_mailbox*>(mailbox)->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_SUCCESS;
        return AGT_ERROR_TOO_MANY_CONSUMERS;
      case 1:
        if ( static_cast<agt::mpsc_mailbox*>(mailbox)->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_SUCCESS;
        return AGT_ERROR_TOO_MANY_CONSUMERS;
      case 2:
        if ( static_cast<agt::spmc_mailbox*>(mailbox)->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_SUCCESS;
        return AGT_ERROR_TOO_MANY_CONSUMERS;
      case 3:
        if ( static_cast<agt::spsc_mailbox*>(mailbox)->consumerSemaphore.try_acquire_for(timeout_us) )
          return AGT_SUCCESS;
        return AGT_ERROR_TOO_MANY_CONSUMERS;
      JEM_no_default;
    }
  }
  JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us) {}
  JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us) {}
  JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us) {}

  JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_message_size(agt_mailbox_t mailbox) {
    if ( mailbox->flags & AGT_MAILBOX_IS_DYNAMIC )
      return 0;
    return static_cast<agt::static_mailbox*>(mailbox)->msgMemory.size;
  }
  JEM_api agt_mailbox_flags_t JEM_stdcall agt_mailbox_get_flags(agt_mailbox_t mailbox) {
    return mailbox->flags;
  }
  JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_producers(agt_mailbox_t mailbox) {
    if ( mailbox->flags & AGT_MAILBOX_SINGLE_PRODUCER )
      return 1;
    if ( mailbox->flags & AGT_MAILBOX_SINGLE_CONSUMER )
      return static_cast<agt::mpsc_mailbox*>(mailbox)->maxProducers;
    return static_cast<agt::mpmc_mailbox*>(mailbox)->maxProducers;
  }
  JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_consumers(agt_mailbox_t mailbox) {
    if ( mailbox->flags & AGT_MAILBOX_SINGLE_CONSUMER )
      return 1;
    if ( mailbox->flags & AGT_MAILBOX_SINGLE_PRODUCER )
      return static_cast<agt::spmc_mailbox*>(mailbox)->maxConsumers;
    return static_cast<agt::mpmc_mailbox*>(mailbox)->maxConsumers;
  }



  JEM_api agt_status_t        JEM_stdcall agt_start_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us) {
    return (agt::impl::start_send_message_table[mailbox->kind])(mailbox, messageSize, pMessagePayload, timeout_us);
  }
  JEM_api agt_status_t        JEM_stdcall agt_finish_send_message(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags) {
    return (agt::impl::finish_send_message_table[mailbox->kind])(mailbox, pMessage, messagePayload, flags);
  }
  JEM_api agt_status_t        JEM_stdcall agt_receive_message(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us) {
    return (agt::impl::receive_message_table[mailbox->kind])(mailbox, pMessage, timeout_us);
  }
  JEM_api void                JEM_stdcall agt_discard_message(agt_mailbox_t mailbox, agt_message_t message) {
    return (agt::impl::discard_message_table[mailbox->kind])(mailbox, message);
  }
  JEM_api agt_status_t        JEM_stdcall agt_cancel_message(agt_mailbox_t mailbox, agt_message_t message) {
    return (agt::impl::cancel_message_table[mailbox->kind])(mailbox, message);
  }

}