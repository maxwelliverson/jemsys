//
// Created by maxwe on 2021-06-01.
//

#include "internal.hpp"

#include <agate/spsc_mailbox.h>
#include <agate/mpsc_mailbox.h>
#include <agate/spmc_mailbox.h>
#include <agate/mpmc_mailbox.h>

namespace {
  using PFN_acquire_slot          = void*(JEM_stdcall*)(agt_handle_t handle, jem_size_t messageSize) noexcept;
  using PFN_try_acquire_slot      = agt_status_t(JEM_stdcall*)(agt_handle_t handle, jem_size_t messageSize, void** pSlot, jem_u64_t timeout_us) noexcept;
  using PFN_acquire_slot_ex       = agt_status_t(JEM_stdcall*)(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) noexcept;
  using PFN_send                  = agt_message_t(JEM_stdcall*)(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) noexcept;
  using PFN_send_and_discard      = void(JEM_stdcall*)(agt_handle_t handle, void* messageSlot);
  using PFN_do_not_send           = void(JEM_stdcall*)(agt_handle_t handle, void* messageSlot);
  using PFN_send_many             = void(JEM_stdcall*)(agt_handle_t handle, void** messageSlots, jem_size_t messageCount, agt_message_t* messages, agt_send_message_flags_t flags) noexcept;
  using PFN_send_and_discard_many = void(JEM_stdcall*)(agt_handle_t handle, void** messageSlots, jem_size_t messageCount);
  using PFN_do_not_send_many      = void(JEM_stdcall*)(agt_handle_t handle, void** messageSlots, jem_size_t messageCount);
  using PFN_receive               = agt_message_t(JEM_stdcall*)(agt_handle_t handle) noexcept;
  using PFN_try_receive           = agt_status_t(JEM_stdcall*)(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) noexcept;
  using PFN_receive_ex            = agt_status_t(JEM_stdcall*)(agt_handle_t handle, const agt_receive_ex_params_t* params) noexcept;

  using PFN_discard               = bool(JEM_stdcall*)(agt_handle_t handle, agt_message_t message) noexcept;
  using PFN_cancel                = agt_status_t(JEM_stdcall*)(agt_handle_t handle, agt_message_t message) noexcept;
  using PFN_query_attributes      = void(JEM_stdcall*)(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_kind_t* attributeKinds, agt_handle_attribute_t* attributes) noexcept;


  struct object_vtable_t{
    PFN_acquire_slot          acquire_slot;
    PFN_try_acquire_slot      try_acquire_slot;
    PFN_acquire_slot_ex       acquire_slot_ex;
    PFN_send                  send;
    PFN_do_not_send           do_not_send;
    PFN_send_and_discard      send_and_discard;
    PFN_send_many             send_many;
    PFN_do_not_send_many      do_not_send_many;
    PFN_send_and_discard_many send_and_discard_many;
    PFN_receive               receive;
    PFN_try_receive           try_receive;
    PFN_receive_ex            receive_ex;
    PFN_discard               discard;
    PFN_cancel                cancel;
    PFN_query_attributes      query_attributes;
  };

  template <typename T>
  struct object_ops{

    using message_type = typename T::message_type;

    static bool valid_message_size(const T*, jem_size_t) noexcept;

    static message_type* acquire_free_slot(T*) noexcept;
    static message_type* try_acquire_free_slot(T*) noexcept;
    static message_type* try_acquire_free_slot_for(T*, jem_u64_t) noexcept;

    static message_type* acquire_queued_message(T*) noexcept;
    static message_type* try_acquire_queued_message(T*) noexcept;
    static message_type* try_acquire_queued_message_for(T*, jem_u64_t) noexcept;

    static void enqueue_message(T*, message_type*) noexcept;
    static void release_message(T*, message_type*) noexcept;
    static void enqueue_messages(T*, message_type*, jem_size_t count) noexcept;
    static void release_messages(T*, message_type*, jem_size_t count) noexcept;
  };
  template <typename T>
  struct dynamic_object_ops{

    using message_type = typename T::message_type;

    static bool       valid_message_size(const T*, jem_size_t) noexcept;
    static jem_size_t translate_message_size(const T* object, jem_size_t messageSize) noexcept;

    static message_type* acquire_free_slot(T* object, jem_size_t messageSize) noexcept;
    static message_type* try_acquire_free_slot(T* object, jem_size_t messageSize) noexcept;
    static message_type* try_acquire_free_slot_for(T* object, jem_size_t messageSize, jem_u64_t timeout_us) noexcept;

    static message_type* acquire_queued_message(T* object) noexcept;
    static message_type* try_acquire_queued_message(T* object) noexcept;
    static message_type* try_acquire_queued_message_for(T* object, jem_u64_t timeout_us) noexcept;

    static void enqueue_message(T* object, message_type* message) noexcept;
    static void release_message(T* object, message_type* message) noexcept;
    static void enqueue_messages(T*, message_type*, jem_size_t count) noexcept;
    static void release_messages(T*, message_type*, jem_size_t count) noexcept;
  };


  template <typename T>
  struct object_vtable_impl{

    using object_t = T*;
    using message_t = typename agt::handle_traits<T>::message_type;


    inline constexpr static bool IsDynamic = T::dynamic_flag != 0;
    inline constexpr static bool IsIpc     = T::ipc_flag != 0;

    using ops = std::conditional_t<IsDynamic, dynamic_object_ops<T>, object_ops<T>>;

    static void*         JEM_stdcall acquire_slot(agt_handle_t handle, jem_size_t messageSize) noexcept {
      const auto object = cast<object_t>(handle);
      message_t message;
      if ( !ops::valid_message_size(object, messageSize) ) [[unlikely]]
        return nullptr;

      if constexpr ( IsDynamic ) {
        jem_size_t actualSize = ops::translate_message_size(object, messageSize);
        return ops::acquire_free_slot(object, actualSize);
      }
      else {
        return ops::acquire_free_slot(object);
      }
    }
    static agt_status_t  JEM_stdcall try_acquire_slot(agt_handle_t handle, jem_size_t messageSize, void** pPayload, jem_u64_t timeout_us) noexcept {
      const auto object = cast<object_t>(handle);
      message_t message;

      if ( !ops::valid_message_size(object, messageSize) ) [[unlikely]]
        return AGT_ERROR_MESSAGE_TOO_LARGE;

      if constexpr ( IsDynamic ) {
        jem_size_t actualSize = ops::translate_message_size(object, messageSize);
        if ( timeout_us == JEM_WAIT ) [[likely]] {
          message = ops::acquire_free_slot(object, actualSize);
        }
        else {
          message = timeout_us == JEM_DO_NOT_WAIT ? ops::try_acquire_free_slot(object, actualSize) : ops::try_acquire_free_slot_for(object, actualSize, timeout_us);
          if ( !message )
            return AGT_ERROR_MAILBOX_IS_FULL;
        }
      }
      else {
        if ( timeout_us == JEM_WAIT ) [[likely]] {
          message = ops::acquire_free_slot(object);
        }
        else {
          message = timeout_us == JEM_DO_NOT_WAIT ? ops::try_acquire_free_slot(object) : ops::try_acquire_free_slot_for(object, timeout_us);
          if ( !message )
            return AGT_ERROR_MAILBOX_IS_FULL;
        }
      }
      JEM_assume( message != nullptr );

      *pPayload = message_to_payload(message);
      return AGT_SUCCESS;
    }
    static agt_status_t  JEM_stdcall acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }

    static agt_message_t JEM_stdcall send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) noexcept {
      const auto object  = cast<object_t>(handle);
      const auto message = static_cast<message_t>(payload_to_message(messageSlot));

      message->flags.set(flags | agt::message_in_use);
      ops::enqueue_message(object, message);
      return message;
    }
    static void          JEM_stdcall do_not_send(agt_handle_t handle, void* messageSlot) noexcept {
      ops::release_message(cast<object_t>(handle), static_cast<message_t>(payload_to_message(messageSlot)));
    }
    static void          JEM_stdcall send_and_discard(agt_handle_t handle, void* messageSlot) noexcept {
      const auto object  = cast<object_t>(handle);
      const auto message = static_cast<message_t>(payload_to_message(messageSlot));

      message->flags.set(agt::message_in_use | agt::message_result_is_discarded);
      ops::enqueue_message(object, message);
    }

    static void          JEM_stdcall send_many(agt_handle_t handle, void** messageSlots, jem_size_t messageCount, agt_message_t* messages, agt_send_message_flags_t flags) noexcept {
      const auto object  = cast<object_t>(handle);

      for ( jem_size_t i = 0; i != messageCount; ++i ) {
        const auto message = static_cast<message_t>(payload_to_message(messageSlots[i]));
        message->flags.set(flags | agt::message_in_use);
        messages[i] = message;
      }

      ops::enqueue_messages(object, messages, messageCount);
    }
    static void          JEM_stdcall do_not_send_many(agt_handle_t handle, void** messageSlots, jem_size_t messageCount) noexcept {
      const auto object  = cast<object_t>(handle);
      const auto messages = reinterpret_cast<message_t*>(alloca(messageCount * sizeof(void*)));
      for ( jem_size_t i = 0; i != messageCount; ++i ) {
        messages[i] = static_cast<message_t>(payload_to_message(messageSlots[i]));
      }
      ops::release_messages(object, messages, messageCount);
    }
    static void          JEM_stdcall send_and_discard_many(agt_handle_t handle, void** messageSlots, jem_size_t messageCount) noexcept {
      const auto object  = cast<object_t>(handle);
      const auto messages = reinterpret_cast<message_t*>(alloca(messageCount * sizeof(void*)));
      for ( jem_size_t i = 0; i != messageCount; ++i ) {
        const auto message = static_cast<message_t>(payload_to_message(messageSlots[i]));
        message->flags.set(agt::message_in_use);
        messages[i] = message;
      }

      ops::enqueue_messages(object, messages, messageCount);
    }

    static agt_message_t JEM_stdcall receive(agt_handle_t handle) noexcept {
      return ops::acquire_queued_message(cast<object_t>(handle));
    }
    static agt_status_t  JEM_stdcall try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) noexcept {
      const auto object = cast<object_t>(handle);
      message_t message;

      if ( timeout_us == JEM_WAIT ) [[likely]] {
        message = ops::acquire_queued_message(object);
      }
      else {
        if ( timeout_us == JEM_DO_NOT_WAIT ) {
          message = ops::try_acquire_queued_message(object);
        }
        else {
          message = ops::try_acquire_queued_message_for(object, timeout_us);
        }
        if ( !message )
          return AGT_ERROR_MAILBOX_IS_EMPTY;
      }
      *pMessage = message;
      return AGT_SUCCESS;
    }
    static agt_status_t  JEM_stdcall receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }

    static bool          JEM_stdcall discard(agt_handle_t handle, agt_message_t message) noexcept {

    }
    static agt_status_t  JEM_stdcall cancel(agt_handle_t handle, agt_message_t message) noexcept {

    }
    static void          JEM_stdcall query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_kind_t* attributeKinds, agt_handle_attribute_t* attributes) noexcept {}
  };

  template <typename T>
  inline constexpr object_vtable_t object_vtable = {
    .acquire_slot     = &object_vtable_impl<T>::acquire_slot,
    .try_acquire_slot = &object_vtable_impl<T>::try_acquire_slot,
    .acquire_slot_ex  = &object_vtable_impl<T>::acquire_slot_ex,
    .send             = &object_vtable_impl<T>::send,
    .do_not_send      = &object_vtable_impl<T>::do_not_send,
    .send_and_discard = &object_vtable_impl<T>::send_and_discard,
    .receive          = &object_vtable_impl<T>::receive,
    .try_receive      = &object_vtable_impl<T>::try_receive,
    .receive_ex       = &object_vtable_impl<T>::receive_ex,
    .discard          = &object_vtable_impl<T>::discard,
    .cancel           = &object_vtable_impl<T>::cancel,
    .query_attributes = &object_vtable_impl<T>::query_attributes
  };

  JEM_forceinline const object_vtable_t& lookup_vtable(agt_handle_t handle) noexcept {

    constexpr static object_vtable_t object_vtable_table[] = {

      object_vtable<agt::local_mpsc_mailbox>,
      object_vtable<agt::dynamic_local_mpsc_mailbox>,
      object_vtable<agt::ipc_mpsc_mailbox>,
      object_vtable<agt::dynamic_ipc_mpsc_mailbox>,

      object_vtable<agt::local_spsc_mailbox>,
      object_vtable<agt::dynamic_local_spsc_mailbox>,
      object_vtable<agt::ipc_spsc_mailbox>,
      object_vtable<agt::dynamic_ipc_spsc_mailbox>,

      object_vtable<agt::local_mpmc_mailbox>,
      object_vtable<agt::dynamic_local_mpmc_mailbox>,
      object_vtable<agt::ipc_mpmc_mailbox>,
      object_vtable<agt::dynamic_ipc_mpmc_mailbox>,

      object_vtable<agt::local_spmc_mailbox>,
      object_vtable<agt::dynamic_local_spmc_mailbox>,
      object_vtable<agt::ipc_spmc_mailbox>,
      object_vtable<agt::dynamic_ipc_spmc_mailbox>,

      object_vtable<agt::local_thread_deputy>,
      object_vtable<agt::dynamic_local_thread_deputy>,
      object_vtable<agt::ipc_thread_deputy>,
      object_vtable<agt::dynamic_ipc_thread_deputy>,

      object_vtable<agt::local_thread_pool_deputy>,
      object_vtable<agt::dynamic_local_thread_pool_deputy>,
      object_vtable<agt::ipc_thread_pool_deputy>,
      object_vtable<agt::dynamic_ipc_thread_pool_deputy>,

      object_vtable<agt::local_lazy_deputy>,
      object_vtable<agt::dynamic_local_lazy_deputy>,
      object_vtable<agt::ipc_lazy_deputy>,
      object_vtable<agt::dynamic_ipc_lazy_deputy>,

      object_vtable<agt::local_proxy_deputy>,
      object_vtable<agt::dynamic_local_proxy_deputy>,
      object_vtable<agt::ipc_proxy_deputy>,
      object_vtable<agt::dynamic_ipc_proxy_deputy>,

      object_vtable<agt::local_virtual_deputy>,
      object_vtable<agt::dynamic_local_virtual_deputy>,
      object_vtable<agt::ipc_virtual_deputy>,
      object_vtable<agt::dynamic_ipc_virtual_deputy>,

      object_vtable<agt::local_collective_deputy>,
      object_vtable<agt::dynamic_local_collective_deputy>,
      object_vtable<agt::ipc_collective_deputy>,
      object_vtable<agt::dynamic_ipc_collective_deputy>
    };

    return object_vtable_table[agt_internal_get_object_flags(handle)];
  }

  // Variable Length Messages = 0x1
  // Interprocess             = 0x2

  // MPSC = 0
  // SPSC = 1
  // MPMC = 2
  // SPMC = 3
  // Thread Deputy = 4
  // Thread Pool Deputy = 5
  // Lazy Deputy = 6
  // Proxy Deputy = 7
  // Virtual Deputy = 8
  // Collective Deputy = 9
}


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


  JEM_api void*               JEM_stdcall agt_acquire_slot(agt_handle_t handle, jem_size_t messageSize) {
    return lookup_vtable(handle).acquire_slot(handle, messageSize);
  }
  JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot(agt_handle_t handle, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_acquire_slot(handle, messageSize, pMessagePayload, timeout_us);
  }
  JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) {
    return lookup_vtable(handle).acquire_slot_ex(handle, params);
  }


  JEM_api agt_message_t       JEM_stdcall agt_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) {

    constexpr static agt_send_message_flags_t InvalidFlagMask   = ~(AGT_SEND_MESSAGE_CANCEL | AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT);
    constexpr static agt_send_message_flags_t InvalidFlagCombo0 = AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT;

    assert(!(flags & InvalidFlagMask));
    assert( (flags & InvalidFlagCombo0 ) != InvalidFlagCombo0 );

    const auto& vtable = lookup_vtable(handle);

    if ( flags & AGT_SEND_MESSAGE_CANCEL ) {
      vtable.do_not_send(handle, messageSlot);
      return nullptr;
    }

    if ( flags & AGT_SEND_MESSAGE_DISCARD_RESULT ) {
      vtable.send_and_discard(handle, messageSlot);
      return nullptr;
    }

    return vtable.send(handle, messageSlot, flags);
  }
  JEM_api agt_status_t        JEM_stdcall agt_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) {

    constexpr static agt_send_message_flags_t InvalidFlagMask   = ~(AGT_SEND_MESSAGE_CANCEL | AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT);
    constexpr static agt_send_message_flags_t InvalidFlagCombo0 = AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT;

    assert( params != nullptr );
    assert(!(params->flags & InvalidFlagMask));
    assert( (params->flags & InvalidFlagCombo0 ) != InvalidFlagCombo0 );
    assert( params->message_count == 0 || params->payloads != nullptr );
    // assert( params->message_count == 0 || params->messages != nullptr );


    if ( params->flags & InvalidFlagMask ) [[unlikely]]
      return AGT_ERROR_INVALID_FLAGS;
    if ( (params->flags & InvalidFlagCombo0 ) == InvalidFlagCombo0 ) [[unlikely]]
      return AGT_ERROR_INVALID_FLAGS;

    if ( params->extra_param_count != 0 || params->extra_params != nullptr ) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;

    const auto& vtable = lookup_vtable(handle);

    if ( params->flags & AGT_SEND_MESSAGE_CANCEL ) [[unlikely]] {
      vtable.do_not_send_many(handle, params->payloads, params->message_count);
      return AGT_CANCELLED;
    }

    if ( params->flags & AGT_SEND_MESSAGE_DISCARD_RESULT ) {
      vtable.send_and_discard_many(handle, params->payloads, params->message_count);
      return AGT_SUCCESS;
    }

    if ( params->messages == nullptr ) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;

    vtable.send_many(handle, params->payloads, params->message_count, params->messages, params->flags);
    return AGT_SUCCESS;
  }

  JEM_api agt_message_t       JEM_stdcall agt_receive(agt_handle_t handle) {
    return lookup_vtable(handle).receive(handle);
  }
  JEM_api agt_status_t        JEM_stdcall agt_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_receive(handle, pMessage, timeout_us);
  }
  JEM_api agt_status_t        JEM_stdcall agt_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) {
    return lookup_vtable(handle).receive_ex(handle, params);
  }


  JEM_api bool                JEM_stdcall agt_discard(agt_message_t message) {
    return lookup_vtable(message->parent).discard(message->parent, message);
  }
  JEM_api agt_status_t        JEM_stdcall agt_cancel(agt_message_t message) {
    return lookup_vtable(message->parent).cancel(message->parent, message);
  }
  JEM_api void                JEM_stdcall agt_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_kind_t* attributeKinds, agt_handle_attribute_t* attributes) {
    lookup_vtable(handle).query_attributes(handle, attributeCount, attributeKinds, attributes);
  }

}