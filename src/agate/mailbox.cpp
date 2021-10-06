//
// Created by maxwe on 2021-06-01.
//

#include "internal.hpp"

#include <agate/spsc_mailbox.h>
#include <agate/mpsc_mailbox.h>
#include <agate/spmc_mailbox.h>
#include <agate/mpmc_mailbox.h>

namespace {

  template <auto PFN>
  struct pfn;
  template <typename Ret, typename ...Args, Ret(*PFN)(Args...)>
  struct pfn<PFN>{
    using type = Ret(JEM_stdcall*)(Args...) noexcept;
  };

#define JEM_define_pfn(fn) using PFN_##fn = typename pfn<(&agt_##fn)>::type
#define JEM_member_pfn(fn) typename pfn<(&agt_##fn)>::type fn


  JEM_define_pfn(acquire_slot);
  JEM_define_pfn(acquire_many_slots);
  JEM_define_pfn(try_acquire_slot);
  JEM_define_pfn(try_acquire_many_slots);
  JEM_define_pfn(acquire_slot_ex);

  JEM_define_pfn(release_slot);
  JEM_define_pfn(release_many_slots);
  JEM_define_pfn(release_slot_ex);

  JEM_define_pfn(send);
  JEM_define_pfn(send_many);
  JEM_define_pfn(send_ex);

  JEM_define_pfn(receive);
  JEM_define_pfn(receive_many);
  JEM_define_pfn(try_receive);
  JEM_define_pfn(try_receive_many);
  JEM_define_pfn(receive_ex);


  using PFN_discard               = bool(JEM_stdcall*)(agt_handle_t handle, agt_message_t message) noexcept;
  using PFN_cancel                = agt_status_t(JEM_stdcall*)(agt_handle_t handle, agt_message_t message) noexcept;
  using PFN_query_attributes      = void(JEM_stdcall*)(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) noexcept;


  struct object_vtable_t{
    JEM_member_pfn(acquire_slot);
    JEM_member_pfn(acquire_many_slots);
    JEM_member_pfn(try_acquire_slot);
    JEM_member_pfn(try_acquire_many_slots);
    JEM_member_pfn(acquire_slot_ex);

    JEM_member_pfn(release_slot);
    JEM_member_pfn(release_many_slots);
    JEM_member_pfn(release_slot_ex);

    JEM_member_pfn(send);
    JEM_member_pfn(send_many);
    JEM_member_pfn(send_ex);

    JEM_member_pfn(receive);
    JEM_member_pfn(receive_many);
    JEM_member_pfn(try_receive);
    JEM_member_pfn(try_receive_many);
    JEM_member_pfn(receive_ex);



    PFN_discard               discard;
    PFN_cancel                cancel;
    PFN_query_attributes      query_attributes;
  };

  template <typename T>
  struct common_ops {
    using object_t       = T*;
    using const_object_t = const T*;
    using message_t      = typename agt::handle_traits<T>::message_type;

    static bool valid_message_size(const_object_t object, jem_size_t messageSize) noexcept;
    static bool valid_message_size(const_object_t object, jem_size_t messageSize, jem_size_t messageCount) noexcept;

    static message_t     acquire_queued_message(object_t object) noexcept;
    static message_t try_acquire_queued_message(object_t object) noexcept;
    static message_t try_acquire_queued_message_for(object_t object, jem_u64_t timeout_us) noexcept;

    static bool             acquire_many_queued_messages(object_t object, jem_size_t messageCount, message_t* messages) noexcept;
    static agt_status_t try_acquire_many_queued_messages(object_t object, jem_size_t messageCount, message_t* messages) noexcept;
    static agt_status_t try_acquire_many_queued_messages_for(object_t object, jem_size_t messageCount, message_t* messages, jem_u64_t timeout_us) noexcept;

    static void enqueue_message(object_t object, message_t message) noexcept;
    static void release_message(object_t object, message_t message) noexcept;
    static void enqueue_messages(object_t object, const message_t* messages, jem_size_t count) noexcept;
    static void release_messages(object_t object, const message_t* messages, jem_size_t count) noexcept;
  };
  template <typename T>
  struct object_ops : common_ops<T> {

    using object_t       = T*;
    using const_object_t = const T*;
    using message_t      = typename agt::handle_traits<T>::message_type;

    static message_t     acquire_free_slot(object_t object) noexcept;
    static message_t try_acquire_free_slot(object_t object) noexcept;
    static message_t try_acquire_free_slot_for(object_t object, jem_u64_t timeout_us) noexcept;

    static bool             acquire_many_free_slots(object_t object, jem_size_t messageCount, void** slots) noexcept;
    static agt_status_t try_acquire_many_free_slots(object_t object, jem_size_t messageCount, void** slots) noexcept;
    static agt_status_t try_acquire_many_free_slots_for(object_t object, jem_size_t messageCount, void** slots, jem_u64_t timeout_us) noexcept;


  };
  template <typename T>
  struct dynamic_object_ops : common_ops<T> {

    using object_t       = T*;
    using const_object_t = const T*;
    using message_t      = typename agt::handle_traits<T>::message_type;

    static jem_size_t translate_message_size(const_object_t object, jem_size_t messageSize) noexcept;

    static message_t acquire_free_slot(object_t object, jem_size_t messageSize) noexcept;
    static message_t try_acquire_free_slot(object_t object, jem_size_t messageSize) noexcept;
    static message_t try_acquire_free_slot_for(object_t object, jem_size_t messageSize, jem_u64_t timeout_us) noexcept;

    static bool             acquire_many_free_slots(object_t object, jem_size_t messageSize, jem_size_t messageCount, void** slots) noexcept;
    static agt_status_t try_acquire_many_free_slots(object_t object, jem_size_t messageSize, jem_size_t messageCount, void** slots) noexcept;
    static agt_status_t try_acquire_many_free_slots_for(object_t object, jem_size_t messageSize, jem_size_t messageCount, void** slots, jem_u64_t timeout_us) noexcept;
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
    static agt_status_t  JEM_stdcall acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) noexcept {
      const auto object = cast<object_t>(handle);
      if ( !ops::valid_message_size(object, slotSize, slotCount) ) [[unlikely]]
        return AGT_ERROR_BAD_SIZE;

      if constexpr ( IsDynamic ) {
        jem_size_t actualSize = ops::translate_message_size(object, slotSize);
        if ( !ops::acquire_many_free_slots(object, actualSize, slotCount, slots) )
          return AGT_ERROR_INSUFFICIENT_SLOTS;
      }
      else {
        if ( !ops::acquire_many_free_slots(object, slotCount, slots) )
          return AGT_ERROR_INSUFFICIENT_SLOTS;
      }
      return AGT_SUCCESS;
    }
    static agt_status_t  JEM_stdcall try_acquire_slot(agt_handle_t handle, jem_size_t messageSize, void** pPayload, jem_u64_t timeout_us) noexcept {
      const auto object = cast<object_t>(handle);
      message_t message;

      if ( !ops::valid_message_size(object, messageSize) ) [[unlikely]]
        return AGT_ERROR_BAD_SIZE;

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
    static agt_status_t  JEM_stdcall try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) noexcept {
      const auto object = cast<object_t>(handle);

      if ( timeout_us == JEM_WAIT ) [[unlikely]]
        return acquire_many_slots(handle, slotSize, slotCount, slots);

      if ( !ops::valid_message_size(object, slotSize, slotCount) ) [[unlikely]]
        return AGT_ERROR_BAD_SIZE;

      if constexpr ( IsDynamic ) {
        jem_size_t actualSize = ops::translate_message_size(object, slotSize);
        if ( timeout_us == JEM_DO_NOT_WAIT )
          return ops::try_acquire_many_free_slots(object, actualSize, slotCount, slots);
        return ops::try_acquire_many_free_slots_for(object, actualSize, slotCount, slots, timeout_us);
      }
      else {
        if ( timeout_us == JEM_DO_NOT_WAIT )
          return ops::try_acquire_many_free_slots(object, slotCount, slots);
        return ops::try_acquire_many_free_slots_for(object, slotCount, slots, timeout_us);
      }
    }
    static agt_status_t  JEM_stdcall acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }


    static void          JEM_stdcall release_slot(agt_handle_t handle, void* slot) noexcept {
      ops::release_message(cast<object_t>(handle), static_cast<message_t>(payload_to_message(slot)));
    }
    static void          JEM_stdcall release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) noexcept {
      const auto messages = reinterpret_cast<message_t*>(slots);
      for ( jem_size_t i = 0; i != slotCount; ++i )
        messages[i] = static_cast<message_t>(payload_to_message(slots[i]));
      ops::release_messages(cast<object_t>(handle), messages, slotCount);
    }
    static agt_status_t  JEM_stdcall release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }


    static agt_message_t JEM_stdcall send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) noexcept {

      constexpr static agt_send_message_flags_t InvalidFlagMask   = ~(AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT);
      constexpr static agt_send_message_flags_t InvalidFlagCombo0 = AGT_SEND_MESSAGE_DISCARD_RESULT | AGT_SEND_MESSAGE_MUST_CHECK_RESULT;

      assert(!(flags & InvalidFlagMask));
      assert( (flags & InvalidFlagCombo0 ) != InvalidFlagCombo0 );

      const auto object  = cast<object_t>(handle);
      const auto message = static_cast<message_t>(payload_to_message(messageSlot));

      message->flags.set(flags | agt::signal_in_use);
      ops::enqueue_message(object, message);
      if ( flags & AGT_SEND_MESSAGE_DISCARD_RESULT )
        return nullptr;
      return message;
    }
    static void          JEM_stdcall send_many(agt_handle_t handle, jem_size_t messageCount, void** slots, agt_message_t* messages, agt_send_message_flags_t flags) noexcept {

      assert((flags & AGT_SEND_MESSAGE_DISCARD_RESULT) || messages != nullptr);

      const auto object  = cast<object_t>(handle);
      message_t* const messageList = flags & AGT_SEND_MESSAGE_DISCARD_RESULT ? reinterpret_cast<message_t*>(slots) : reinterpret_cast<message_t*>(messages);

      for ( jem_size_t i = 0; i != messageCount; ++i ) {
        const auto message = static_cast<message_t>(payload_to_message(slots[i]));
        message->flags.set(flags | agt::signal_in_use);
        messageList[i] = message;
      }

      ops::enqueue_messages(object, messageList, messageCount);
    }
    static agt_status_t  JEM_stdcall send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }


    static agt_message_t JEM_stdcall receive(agt_handle_t handle) noexcept {
      return ops::acquire_queued_message(cast<object_t>(handle));
    }
    static agt_status_t  JEM_stdcall receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) noexcept {
      const auto object = cast<object_t>(handle);
      const auto messageList = reinterpret_cast<message_t*>(messages);

      if ( !ops::acquire_many_queued_messages(object, count, messageList))
        return AGT_ERROR_INSUFFICIENT_SLOTS;
      return AGT_SUCCESS;
    }
    static agt_status_t  JEM_stdcall try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) noexcept {

      if ( timeout_us == JEM_WAIT ) [[unlikely]] {
        *pMessage = receive(handle);
        return AGT_SUCCESS;
      }
      const auto object = cast<object_t>(handle);
      message_t message;

      if ( timeout_us == JEM_DO_NOT_WAIT ) {
        message = ops::try_acquire_queued_message(object);
      }
      else {
        message = ops::try_acquire_queued_message_for(object, timeout_us);
      }
      if ( !message )
        return AGT_ERROR_MAILBOX_IS_EMPTY;

      *pMessage = message;
      return AGT_SUCCESS;
    }
    static agt_status_t  JEM_stdcall try_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages, jem_u64_t timeout_us) noexcept {

      if ( timeout_us == JEM_WAIT ) [[unlikely]]
        return receive_many(handle, count, messages);

      const auto object = cast<object_t>(handle);

      if ( timeout_us == JEM_DO_NOT_WAIT )
        return ops::try_acquire_many_queued_messages(object, count, reinterpret_cast<message_t*>(messages));
      return ops::try_acquire_many_queued_messages_for(object, count, reinterpret_cast<message_t*>(messages), timeout_us);
    }
    static agt_status_t  JEM_stdcall receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) noexcept {
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    }


    static bool          JEM_stdcall discard(agt_handle_t handle, agt_message_t message) noexcept {

    }
    static agt_status_t  JEM_stdcall cancel(agt_handle_t handle, agt_message_t message) noexcept {

    }
    static void          JEM_stdcall query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) noexcept {

    }
  };


#define JEM_init_pfn(fn) .fn = (&object_vtable_impl<T>::fn)

  template <typename T>
  inline constexpr object_vtable_t object_vtable = {
    JEM_init_pfn(acquire_slot),
    JEM_init_pfn(acquire_many_slots),
    JEM_init_pfn(try_acquire_slot),
    JEM_init_pfn(try_acquire_many_slots),
    JEM_init_pfn(acquire_slot_ex),
    JEM_init_pfn(release_slot),
    JEM_init_pfn(release_many_slots),
    JEM_init_pfn(release_slot_ex),
    JEM_init_pfn(send),
    JEM_init_pfn(send_many),
    JEM_init_pfn(send_ex),
    JEM_init_pfn(receive),
    JEM_init_pfn(receive_many),
    JEM_init_pfn(try_receive),
    JEM_init_pfn(try_receive_many),
    JEM_init_pfn(receive_ex),
    JEM_init_pfn(discard),
    JEM_init_pfn(cancel),
    JEM_init_pfn(query_attributes)
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
      object_vtable<agt::dynamic_ipc_collective_deputy>,

      object_vtable<agt::private_mailbox>,
      object_vtable<agt::dynamic_private_mailbox>
    };

    return object_vtable_table[agt_internal_get_object_flags(handle)];
  }


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

  /*JEM_api agt_request_t       JEM_stdcall agt_mailbox_create(agt_mailbox_t* mailbox, const agt_mailbox_create_info_t* createInfo);
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
  }*/




  JEM_api void*               JEM_stdcall agt_acquire_slot(agt_handle_t handle, jem_size_t slotSize) {
    return lookup_vtable(handle).acquire_slot(handle, slotSize);
  }
  JEM_api void                JEM_stdcall agt_release_slot(agt_handle_t handle, void* slot) {
    lookup_vtable(handle).release_slot(handle, slot);
  }
  JEM_api agt_message_t       JEM_stdcall agt_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) {
    return lookup_vtable(handle).send(handle, messageSlot, flags);
  }
  JEM_api agt_message_t       JEM_stdcall agt_receive(agt_handle_t handle) {
    return lookup_vtable(handle).receive(handle);
  }

  JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) {
    return lookup_vtable(handle).acquire_many_slots(handle, slotSize, slotCount, slots);
  }
  JEM_api void                JEM_stdcall agt_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) {
    lookup_vtable(handle).release_many_slots(handle, slotCount, slots);
  }
  JEM_api void                JEM_stdcall agt_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) {
    lookup_vtable(handle).send_many(handle, messageCount, messageSlots, messages, flags);
  }
  JEM_api agt_status_t        JEM_stdcall agt_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) {
    return lookup_vtable(handle).receive_many(handle, count, messages);
  }


  JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_acquire_slot(handle, slotSize, pSlot, timeout_us);
  }
  JEM_api agt_status_t        JEM_stdcall agt_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_receive(handle, pMessage, timeout_us);
  }

  JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_acquire_many_slots(handle, slotSize, slotCount, slots, timeout_us);
  }
  JEM_api agt_status_t        JEM_stdcall agt_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) {
    return lookup_vtable(handle).try_receive_many(handle, slotCount, message, timeout_us);
  }


  JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) {
    return lookup_vtable(handle).acquire_slot_ex(handle, params);
  }
  JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) {
    return lookup_vtable(handle).release_slot_ex(handle, params);
  }
  JEM_api agt_status_t        JEM_stdcall agt_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) {
    return lookup_vtable(handle).send_ex(handle, params);
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

  JEM_api void                JEM_stdcall agt_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) {
    lookup_vtable(handle).query_attributes(handle, attributeCount, attributeKinds, attributes);
  }


}