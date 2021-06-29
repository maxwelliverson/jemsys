//
// Created by Maxwell on 2021-06-24.
//

#include <agate/dispatch/collective_deputy/local.h>
#include <agate/dispatch/collective_deputy/ipc.h>
#include <agate/dispatch/collective_deputy/dynamic_local.h>
#include <agate/dispatch/collective_deputy/dynamic_ipc.h>
#include <agate/dispatch/lazy_deputy/local.h>
#include <agate/dispatch/lazy_deputy/ipc.h>
#include <agate/dispatch/lazy_deputy/dynamic_local.h>
#include <agate/dispatch/lazy_deputy/dynamic_ipc.h>
#include <agate/dispatch/mpmc_mailbox/local.h>
#include <agate/dispatch/mpmc_mailbox/ipc.h>
#include <agate/dispatch/mpmc_mailbox/dynamic_local.h>
#include <agate/dispatch/mpmc_mailbox/dynamic_ipc.h>
#include <agate/dispatch/mpsc_mailbox/local.h>
#include <agate/dispatch/mpsc_mailbox/ipc.h>
#include <agate/dispatch/mpsc_mailbox/dynamic_local.h>
#include <agate/dispatch/mpsc_mailbox/dynamic_ipc.h>
#include <agate/dispatch/private_mailbox/local.h>
#include <agate/dispatch/private_mailbox/dynamic_local.h>
#include <agate/dispatch/proxy_deputy/local.h>
#include <agate/dispatch/proxy_deputy/ipc.h>
#include <agate/dispatch/proxy_deputy/dynamic_local.h>
#include <agate/dispatch/proxy_deputy/dynamic_ipc.h>
#include <agate/dispatch/spmc_mailbox/local.h>
#include <agate/dispatch/spmc_mailbox/ipc.h>
#include <agate/dispatch/spmc_mailbox/dynamic_local.h>
#include <agate/dispatch/spmc_mailbox/dynamic_ipc.h>
#include <agate/dispatch/spsc_mailbox/local.h>
#include <agate/dispatch/spsc_mailbox/ipc.h>
#include <agate/dispatch/spsc_mailbox/dynamic_local.h>
#include <agate/dispatch/spsc_mailbox/dynamic_ipc.h>
#include <agate/dispatch/thread_deputy/local.h>
#include <agate/dispatch/thread_deputy/ipc.h>
#include <agate/dispatch/thread_deputy/dynamic_local.h>
#include <agate/dispatch/thread_deputy/dynamic_ipc.h>
#include <agate/dispatch/thread_pool_deputy/local.h>
#include <agate/dispatch/thread_pool_deputy/ipc.h>
#include <agate/dispatch/thread_pool_deputy/dynamic_local.h>
#include <agate/dispatch/thread_pool_deputy/dynamic_ipc.h>
#include <agate/dispatch/virtual_deputy/local.h>
#include <agate/dispatch/virtual_deputy/ipc.h>
#include <agate/dispatch/virtual_deputy/dynamic_local.h>
#include <agate/dispatch/virtual_deputy/dynamic_ipc.h>

#include "internal.hpp"


namespace {

  template <auto PFN>
  struct pfn;
  template <typename Ret, typename ...Args, Ret(*PFN)(Args...) noexcept>
  struct pfn<PFN>{
    using type = Ret(JEM_stdcall*)(Args...) noexcept;
  };

#define JEM_define_pfn(fn) using PFN_##fn = typename pfn<(&agt_##fn)>::type
#define JEM_member_pfn(fn) typename pfn<(&agt_##fn)>::type fn
#define JEM_init_pfn(fn, type) .fn = (&agt_##type##_##fn)
#define JEM_init_vtable(type) { \
    JEM_init_pfn(acquire_slot, type), \
    JEM_init_pfn(acquire_many_slots, type), \
    JEM_init_pfn(try_acquire_slot, type),   \
    JEM_init_pfn(try_acquire_many_slots, type), \
    JEM_init_pfn(acquire_slot_ex, type),    \
    JEM_init_pfn(release_slot, type), \
    JEM_init_pfn(release_many_slots, type), \
    JEM_init_pfn(release_slot_ex, type),    \
    JEM_init_pfn(send, type),         \
    JEM_init_pfn(send_many, type),    \
    JEM_init_pfn(send_ex, type),      \
    JEM_init_pfn(receive, type),      \
    JEM_init_pfn(receive_many, type), \
    JEM_init_pfn(try_receive, type),  \
    JEM_init_pfn(try_receive_many, type),   \
    JEM_init_pfn(receive_ex, type),   \
    JEM_init_pfn(discard, type),      \
    JEM_init_pfn(cancel, type),       \
    JEM_init_pfn(query_attributes, type) \
  }

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

    JEM_member_pfn(discard);
    JEM_member_pfn(cancel);
    JEM_member_pfn(query_attributes);
  };

  JEM_forceinline const object_vtable_t& lookup_vtable(agt_handle_t handle) noexcept {

    constexpr static object_vtable_t object_vtable_table[] = {

      JEM_init_vtable(local_mpsc_mailbox),
      JEM_init_vtable(dynamic_local_mpsc_mailbox),
      JEM_init_vtable(ipc_mpsc_mailbox),
      JEM_init_vtable(dynamic_ipc_mpsc_mailbox),

      JEM_init_vtable(local_spsc_mailbox),
      JEM_init_vtable(dynamic_local_spsc_mailbox),
      JEM_init_vtable(ipc_spsc_mailbox),
      JEM_init_vtable(dynamic_ipc_spsc_mailbox),

      JEM_init_vtable(local_mpmc_mailbox),
      JEM_init_vtable(dynamic_local_mpmc_mailbox),
      JEM_init_vtable(ipc_mpmc_mailbox),
      JEM_init_vtable(dynamic_ipc_mpmc_mailbox),

      JEM_init_vtable(local_spmc_mailbox),
      JEM_init_vtable(dynamic_local_spmc_mailbox),
      JEM_init_vtable(ipc_spmc_mailbox),
      JEM_init_vtable(dynamic_ipc_spmc_mailbox),

      JEM_init_vtable(local_thread_deputy),
      JEM_init_vtable(dynamic_local_thread_deputy),
      JEM_init_vtable(ipc_thread_deputy),
      JEM_init_vtable(dynamic_ipc_thread_deputy),

      JEM_init_vtable(local_thread_pool_deputy),
      JEM_init_vtable(dynamic_local_thread_pool_deputy),
      JEM_init_vtable(ipc_thread_pool_deputy),
      JEM_init_vtable(dynamic_ipc_thread_pool_deputy),

      JEM_init_vtable(local_lazy_deputy),
      JEM_init_vtable(dynamic_local_lazy_deputy),
      JEM_init_vtable(ipc_lazy_deputy),
      JEM_init_vtable(dynamic_ipc_lazy_deputy),

      JEM_init_vtable(local_proxy_deputy),
      JEM_init_vtable(dynamic_local_proxy_deputy),
      JEM_init_vtable(ipc_proxy_deputy),
      JEM_init_vtable(dynamic_ipc_proxy_deputy),

      JEM_init_vtable(local_virtual_deputy),
      JEM_init_vtable(dynamic_local_virtual_deputy),
      JEM_init_vtable(ipc_virtual_deputy),
      JEM_init_vtable(dynamic_ipc_virtual_deputy),

      JEM_init_vtable(local_collective_deputy),
      JEM_init_vtable(dynamic_local_collective_deputy),
      JEM_init_vtable(ipc_collective_deputy),
      JEM_init_vtable(dynamic_ipc_collective_deputy),

      JEM_init_vtable(private_mailbox),
      JEM_init_vtable(dynamic_private_mailbox)
    };

    return object_vtable_table[agt_internal_get_object_flags(handle)];
  }
}

extern "C" {
JEM_api void*               JEM_stdcall agt_acquire_slot(agt_handle_t handle, jem_size_t slotSize) noexcept {
  return lookup_vtable(handle).acquire_slot(handle, slotSize);
}
JEM_api void                JEM_stdcall agt_release_slot(agt_handle_t handle, void* slot) noexcept {
  lookup_vtable(handle).release_slot(handle, slot);
}
JEM_api agt_message_t       JEM_stdcall agt_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) noexcept {
  return lookup_vtable(handle).send(handle, messageSlot, flags);
}
JEM_api agt_message_t       JEM_stdcall agt_receive(agt_handle_t handle) noexcept {
  return lookup_vtable(handle).receive(handle);
}

JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) noexcept {
  return lookup_vtable(handle).acquire_many_slots(handle, slotSize, slotCount, slots);
}
JEM_api void                JEM_stdcall agt_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) noexcept {
  lookup_vtable(handle).release_many_slots(handle, slotCount, slots);
}
JEM_api void                JEM_stdcall agt_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) noexcept {
  lookup_vtable(handle).send_many(handle, messageCount, messageSlots, messages, flags);
}
JEM_api agt_status_t        JEM_stdcall agt_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) noexcept {
  return lookup_vtable(handle).receive_many(handle, count, messages);
}


JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) noexcept {
  return lookup_vtable(handle).try_acquire_slot(handle, slotSize, pSlot, timeout_us);
}
JEM_api agt_status_t        JEM_stdcall agt_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) noexcept {
  return lookup_vtable(handle).try_receive(handle, pMessage, timeout_us);
}

JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) noexcept {
  return lookup_vtable(handle).try_acquire_many_slots(handle, slotSize, slotCount, slots, timeout_us);
}
JEM_api agt_status_t        JEM_stdcall agt_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) noexcept {
  return lookup_vtable(handle).try_receive_many(handle, slotCount, message, timeout_us);
}


JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) noexcept {
  return lookup_vtable(handle).acquire_slot_ex(handle, params);
}
JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) noexcept {
  return lookup_vtable(handle).release_slot_ex(handle, params);
}
JEM_api agt_status_t        JEM_stdcall agt_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) noexcept {
  return lookup_vtable(handle).send_ex(handle, params);
}
JEM_api agt_status_t        JEM_stdcall agt_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) noexcept {
  return lookup_vtable(handle).receive_ex(handle, params);
}


JEM_api bool                JEM_stdcall agt_discard(agt_message_t message) noexcept {
  return lookup_vtable(message->parent).discard(message);
}
JEM_api agt_status_t        JEM_stdcall agt_cancel(agt_message_t message) noexcept {
  return lookup_vtable(message->parent).cancel(message);
}

JEM_api void                JEM_stdcall agt_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeTypes, agt_handle_attribute_t* attributes) noexcept {
  lookup_vtable(handle).query_attributes(handle, attributeCount, attributeKinds, attributes);
}
}



