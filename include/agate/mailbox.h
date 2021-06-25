//
// Created by maxwe on 2021-06-02.
//

#ifndef JEMSYS_AGATE_MAILBOX_H
#define JEMSYS_AGATE_MAILBOX_H

#include "core.h"



#define AGT_MAILBOX_ALIGN JEM_CACHE_LINE
#define AGT_MAILBOX_SIZE  (JEM_CACHE_LINE * 4)


JEM_begin_c_namespace

typedef enum {
  AGT_DISCARD_MESSAGE,
  AGT_RETURN_MESSAGE,
  AGT_PRESERVE_MESSAGE
} agt_receive_action_t;
typedef enum {
  AGT_HANDLE_ATTRIBUTE_MAX_MESSAGE_SIZE,
  AGT_HANDLE_ATTRIBUTE_DEFAULT_MESSAGE_SIZE,
  AGT_HANDLE_ATTRIBUTE_MAX_CONSUMERS,
  AGT_HANDLE_ATTRIBUTE_MAX_PRODUCERS,
  AGT_HANDLE_ATTRIBUTE_IS_INTERPROCESS_CAPABLE
} agt_handle_attribute_kind_t;

enum agt_mailbox_create_flag_bits_e {
  AGT_MAILBOX_CREATE_REQUIRES_CLEANUP = 0x01,
  AGT_MAILBOX_CREATE_IPC_CAPABLE      = 0x08
};
enum agt_mailbox_flag_bits_e {
  AGT_MAILBOX_SINGLE_CONSUMER            = 0x01,
  AGT_MAILBOX_SINGLE_PRODUCER            = 0x02,
  AGT_MAILBOX_IS_DYNAMIC                 = 0x04,
  AGT_MAILBOX_INTERPROCESS_CAPABLE       = 0x08
};
enum agt_send_message_flag_bits_e{
  AGT_SEND_MESSAGE_DISCARD_RESULT    = 0x4,
  AGT_SEND_MESSAGE_MUST_CHECK_RESULT = 0x8,
  AGT_SEND_MESSAGE_CANCEL            = 0x10
};
#define AGT_MAILBOX_KIND_BITMASK (AGT_MAILBOX_SINGLE_PRODUCER | AGT_MAILBOX_SINGLE_CONSUMER | AGT_MAILBOX_IS_DYNAMIC)


/*typedef enum {
  AGT_MAILBOX_KIND_MPMC         = 0,
  AGT_MAILBOX_KIND_MPSC         = 1,
  AGT_MAILBOX_KIND_SPMC         = 2,
  AGT_MAILBOX_KIND_SPSC         = 3,
  AGT_MAILBOX_KIND_MPMC_DYNAMIC = 4,
  AGT_MAILBOX_KIND_MPSC_DYNAMIC = 5,
  AGT_MAILBOX_KIND_SPMC_DYNAMIC = 6,
  AGT_MAILBOX_KIND_SPSC_DYNAMIC = 7
} agt_mailbox_kind_t;*/

typedef jem_flags32_t agt_mailbox_create_flags_t;
typedef jem_flags32_t agt_mailbox_flags_t;
typedef jem_flags32_t agt_send_message_flags_t;

typedef struct agt_mailbox* agt_mailbox_t;


#define AGT_MAILBOX_CREATE_DEFAULT_FLAGS 0x0


typedef void(JEM_stdcall* agt_mailbox_cleanup_callback_t)(agt_mailbox_t mailbox, void* user_data);

typedef struct {
  jem_u32_t type;
  union{
    void*      pointer;
    jem_size_t size;
    jem_u32_t  u32;
    jem_u64_t  u64;
    float      f32;
    double     f64;
  };
} agt_ext_param_t;
typedef union {
  void*      pointer;
  jem_size_t size;
  jem_u32_t  u32;
  jem_u64_t  u64;
  float      f32;
  double     f64;
} agt_handle_attribute_t;

typedef struct {
  const char* name;
  jem_size_t  name_length;
} agt_mailbox_interprocess_params_t;
typedef struct {
  agt_mailbox_cleanup_callback_t callback;
  void*                          data;
} agt_mailbox_cleanup_params_t;


typedef struct {
  agt_mailbox_create_flags_t flags;
  jem_u32_t                  max_producers;
  jem_u32_t                  max_consumers;
  jem_size_t                 slot_count;
  jem_size_t                 message_size;
  jem_size_t                 extra_param_count;
  const agt_ext_param_t*     extra_params;
} agt_mailbox_create_info_t;


typedef struct {
  jem_size_t             slot_size;
  jem_size_t             count;
  void**                 slots;
  jem_u64_t              timeout_us;
  jem_size_t             extra_param_count;
  const agt_ext_param_t* extra_params;
} agt_acquire_slot_ex_params_t;
typedef struct {
  jem_size_t             count;
  void**                 slots;
  jem_u64_t              timeout_us;
  jem_size_t             extra_param_count;
  const agt_ext_param_t* extra_params;
} agt_release_slot_ex_params_t;
typedef struct {
  agt_send_message_flags_t flags;
  jem_size_t               count;
  void**                   slots;
  agt_message_t*           messages;
  jem_size_t               extra_param_count;
  const agt_ext_param_t*   extra_params;
} agt_send_ex_params_t;
typedef struct {
  jem_size_t             count;
  agt_message_t*         messages;
  jem_u64_t              timeout_us;
  jem_size_t             extra_param_count;
  const agt_ext_param_t* extra_params;
} agt_receive_ex_params_t;





JEM_api jem_size_t          JEM_stdcall agt_get_dynamic_mailbox_granularity();



JEM_api agt_request_t       JEM_stdcall agt_mailbox_create(agt_mailbox_t* mailbox, const agt_mailbox_create_info_t* createInfo);
JEM_api void                JEM_stdcall agt_mailbox_destroy(agt_mailbox_t mailbox);

JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us);

JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_message_size(agt_mailbox_t mailbox);
JEM_api agt_mailbox_flags_t JEM_stdcall agt_mailbox_get_flags(agt_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_producers(agt_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_consumers(agt_mailbox_t mailbox);









JEM_api void*               JEM_stdcall agt_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept;
JEM_api void                JEM_stdcall agt_release_slot(agt_handle_t handle, void* slot) JEM_noexcept;
JEM_api agt_message_t       JEM_stdcall agt_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_message_t       JEM_stdcall agt_receive(agt_handle_t handle) JEM_noexcept;

JEM_api agt_status_t        JEM_stdcall agt_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void                JEM_stdcall agt_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void                JEM_stdcall agt_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept;


JEM_api agt_status_t        JEM_stdcall agt_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept;

JEM_api agt_status_t        JEM_stdcall agt_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept;


JEM_api agt_status_t        JEM_stdcall agt_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;






JEM_api bool                JEM_stdcall agt_discard(agt_message_t message) JEM_noexcept;
JEM_api agt_status_t        JEM_stdcall agt_cancel(agt_message_t message) JEM_noexcept;
JEM_api void                JEM_stdcall agt_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_kind_t* attributeKinds, agt_handle_attribute_t* attributes) JEM_noexcept;





JEM_api agt_status_t        JEM_stdcall agt_get_status(agt_message_t message);
JEM_api void                JEM_stdcall agt_set_status(agt_message_t message, agt_status_t status);
JEM_api agt_status_t        JEM_stdcall agt_notify_sender(agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_wait(agt_message_t message, jem_u64_t timeout_us);
JEM_api void*               JEM_stdcall agt_get_payload(agt_message_t message, jem_size_t* pMessageSize);








JEM_api agt_status_t        JEM_stdcall agt_start_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_finish_send_message(agt_mailbox_t mailbox, agt_message_t* pMessage, void* messagePayload, agt_send_message_flags_t flags);
JEM_api agt_status_t        JEM_stdcall agt_receive_message(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_discard_message(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_message(agt_mailbox_t mailbox, agt_message_t message);


JEM_end_c_namespace


#endif//JEMSYS_AGATE_MAILBOX_H
