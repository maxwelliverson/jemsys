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
  AGT_HANDLE_ATTRIBUTE_TYPE_MAX_MESSAGE_SIZE,
  AGT_HANDLE_ATTRIBUTE_TYPE_DEFAULT_MESSAGE_SIZE,
  AGT_HANDLE_ATTRIBUTE_TYPE_MAX_CONSUMERS,
  AGT_HANDLE_ATTRIBUTE_TYPE_MAX_PRODUCERS,
  AGT_HANDLE_ATTRIBUTE_TYPE_IS_INTERPROCESS,
  AGT_HANDLE_ATTRIBUTE_TYPE_IS_DYNAMIC,
  AGT_HANDLE_ATTRIBUTE_TYPE_CAN_RECEIVE,
  AGT_HANDLE_ATTRIBUTE_TYPE_CAN_SEND,
  AGT_HANDLE_ATTRIBUTE_TYPE_NAME
} agt_handle_attribute_type_t;
typedef enum {
  AGT_EXT_PARAM_TYPE_BASE_HANDLE,
  AGT_EXT_PARAM_TYPE_MAX_CONSUMERS,
  AGT_EXT_PARAM_TYPE_MAX_PRODUCERS,
  AGT_EXT_PARAM_TYPE_IS_DYNAMIC,
  AGT_EXT_PARAM_TYPE_IS_INTERPROCESS,
  AGT_EXT_PARAM_TYPE_SLOT_COUNT,
  AGT_EXT_PARAM_TYPE_MAX_MESSAGE_SIZE,
  AGT_EXT_PARAM_TYPE_DEFAULT_MESSAGE_SIZE,
  AGT_EXT_PARAM_TYPE_NAME_PARAMS,
  AGT_EXT_PARAM_TYPE_CLEANUP_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_THREAD_DEPUTY_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_THREAD_POOL_DEPUTY_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_LAZY_DEPUTY_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_PROXY_DEPUTY_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_VIRTUAL_DEPUTY_PARAMS,
  AGT_EXT_PARAM_TYPE_CREATE_COLLECTIVE_DEPUTY_PARAMS
} agt_ext_param_type_t;


enum agt_send_message_flag_bits_e{
  AGT_SEND_MESSAGE_DISCARD_RESULT    = 0x4,
  AGT_SEND_MESSAGE_MUST_CHECK_RESULT = 0x8,
  AGT_SEND_MESSAGE_CANCEL            = 0x10
};
enum agt_handle_flag_bits_e{
  AGT_HANDLE_CONSUMER   = 0x1,
  AGT_HANDLE_PRODUCER   = 0x2,
  AGT_HANDLE_NON_OWNING = 0x4
};


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

typedef jem_flags32_t agt_send_message_flags_t;
typedef jem_flags32_t agt_handle_flags_t;


typedef void(JEM_stdcall*      agt_cleanup_callback_t)(agt_handle_t handle, void* user_data);
typedef void(JEM_stdcall*      agt_thread_deputy_proc_t)(agt_handle_t deputy, void* userData);
typedef void(JEM_stdcall*      agt_message_proc_t)(agt_handle_t deputy, agt_message_t message, void* payload, void* userData);
typedef jem_u32_t(JEM_stdcall* agt_message_filter_t)(agt_handle_t deputy, const void* filterParam, void* userData);
typedef bool(JEM_stdcall*      agt_lazy_deputy_status_proc_t)(agt_handle_t deputy, agt_message_t message, jem_size_t checkStatusCount, void* userData);



typedef struct {
  jem_u32_t type;
  union{
    void*        pointer;
    agt_handle_t handle;
    jem_u32_t    u32;
    jem_u64_t    u64;
    float        f32;
    double       f64;
    bool         boolean;
    void(*function)();
  };
} agt_ext_param_t;
typedef union {
  void*      pointer;
  jem_u32_t  u32;
  jem_u64_t  u64;
  float      f32;
  double     f64;
  bool       boolean;
} agt_handle_attribute_t;


typedef struct {
  agt_handle_flags_t     flags;
  jem_size_t             ext_param_count;
  const agt_ext_param_t* ext_params;
} agt_open_handle_params_t;
typedef struct {
  jem_size_t             slot_size;
  jem_size_t             count;
  void**                 slots;
  jem_u64_t              timeout_us;
  jem_size_t             ext_param_count;
  const agt_ext_param_t* ext_params;
} agt_acquire_slot_ex_params_t;
typedef struct {
  jem_size_t             count;
  void**                 slots;
  jem_u64_t              timeout_us;
  jem_size_t             ext_param_count;
  const agt_ext_param_t* ext_params;
} agt_release_slot_ex_params_t;
typedef struct {
  agt_send_message_flags_t flags;
  jem_size_t               count;
  void**                   slots;
  agt_message_t*           messages;
  jem_size_t               ext_param_count;
  const agt_ext_param_t*   ext_params;
} agt_send_ex_params_t;
typedef struct {
  jem_size_t             count;
  agt_message_t*         messages;
  jem_u64_t              timeout_us;
  jem_size_t             ext_param_count;
  const agt_ext_param_t* ext_params;
} agt_receive_ex_params_t;



typedef agt_status_t(JEM_stdcall* agt_virtual_acquire_slot_proc_t)(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_release_slot_proc_t)(agt_handle_t handle, const agt_release_slot_ex_params_t* params, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_send_proc_t)(agt_handle_t handle, const agt_send_ex_params_t* params, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_receive_proc_t)(agt_handle_t handle, const agt_receive_ex_params_t* params, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_discard_proc_t)(agt_handle_t handle, agt_message_t message, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_cancel_proc_t)(agt_handle_t handle, agt_message_t message, void* userData);
typedef agt_status_t(JEM_stdcall* agt_virtual_query_attributes_proc_t)(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeKinds, agt_handle_attribute_t* attributes, void* userData);


typedef struct {
  agt_thread_deputy_proc_t proc;
  void*                    user_data;
} agt_create_thread_deputy_params_t;
typedef struct {
  jem_u32_t          thread_count;
  agt_message_proc_t message_proc;
  void*              message_proc_data;
} agt_create_thread_pool_deputy_params_t;
typedef struct {
  agt_lazy_deputy_status_proc_t status_proc;
  void*                         status_proc_data;
  agt_message_proc_t            message_proc;
  void*                         message_proc_data;
} agt_create_lazy_deputy_params_t;
typedef struct {
  const agt_handle_t*  handles;
  jem_size_t           handle_count;
  agt_message_filter_t filter_proc;
  void*                filter_proc_data;
  agt_message_proc_t   message_proc;
  void*                message_proc_data;
} agt_create_proxy_deputy_params_t;
typedef struct {
  agt_virtual_acquire_slot_proc_t     acquire_slot_proc;
  agt_virtual_release_slot_proc_t     release_slot_proc;
  agt_virtual_send_proc_t             send_proc;
  agt_virtual_receive_proc_t          receive_proc;
  agt_virtual_discard_proc_t          discard_proc;
  agt_virtual_cancel_proc_t           cancel_proc;
  agt_virtual_query_attributes_proc_t query_attributes_proc;
  void*                               user_data;
} agt_create_virtual_deputy_params_t;
typedef struct {
  agt_handle_t*   collective_handles;
  jem_size_t      collective_count;
  agt_ext_param_t backing_params;
} agt_create_collective_deputy_params_t;
typedef struct {
  const char* name;
  jem_size_t  name_length;
} agt_name_params_t;
typedef struct {
  agt_cleanup_callback_t callback;
  void*                  user_data;
} agt_cleanup_params_t;





JEM_api jem_size_t          JEM_stdcall agt_get_dynamic_mailbox_granularity();




JEM_api agt_message_t JEM_stdcall agt_open_handle(agt_handle_t* pHandle, const agt_open_handle_params_t* params) JEM_noexcept;
JEM_api void          JEM_stdcall agt_close_handle(agt_handle_t handle) JEM_noexcept;






JEM_api void*         JEM_stdcall agt_acquire_slot(agt_handle_t handle, jem_size_t slotSize) JEM_noexcept;
JEM_api void          JEM_stdcall agt_release_slot(agt_handle_t handle, void* slot) JEM_noexcept;
JEM_api agt_message_t JEM_stdcall agt_send(agt_handle_t handle, void* messageSlot, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_message_t JEM_stdcall agt_receive(agt_handle_t handle) JEM_noexcept;

JEM_api agt_status_t  JEM_stdcall agt_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void          JEM_stdcall agt_release_many_slots(agt_handle_t handle, jem_size_t slotCount, void** slots) JEM_noexcept;
JEM_api void          JEM_stdcall agt_send_many(agt_handle_t handle, jem_size_t messageCount, void** messageSlots, agt_message_t* messages, agt_send_message_flags_t flags) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_receive_many(agt_handle_t handle, jem_size_t count, agt_message_t* messages) JEM_noexcept;


JEM_api agt_status_t  JEM_stdcall agt_try_acquire_slot(agt_handle_t handle, jem_size_t slotSize, void** pSlot, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_try_receive(agt_handle_t handle, agt_message_t* pMessage, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_try_acquire_many_slots(agt_handle_t handle, jem_size_t slotSize, jem_size_t slotCount, void** slots, jem_u64_t timeout_us) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_try_receive_many(agt_handle_t handle, jem_size_t slotCount, agt_message_t* message, jem_u64_t timeout_us) JEM_noexcept;


JEM_api agt_status_t  JEM_stdcall agt_acquire_slot_ex(agt_handle_t handle, const agt_acquire_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_release_slot_ex(agt_handle_t handle, const agt_release_slot_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_send_ex(agt_handle_t handle, const agt_send_ex_params_t* params) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_receive_ex(agt_handle_t handle, const agt_receive_ex_params_t* params) JEM_noexcept;






JEM_api bool          JEM_stdcall agt_discard(agt_message_t message) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_cancel(agt_message_t message) JEM_noexcept;
JEM_api void          JEM_stdcall agt_query_attributes(agt_handle_t handle, jem_size_t attributeCount, const agt_handle_attribute_type_t* attributeKinds, agt_handle_attribute_t* attributes) JEM_noexcept;





JEM_api agt_status_t        JEM_stdcall agt_get_status(agt_message_t message);
JEM_api void                JEM_stdcall agt_set_status(agt_message_t message, agt_status_t status);
JEM_api agt_status_t        JEM_stdcall agt_notify_sender(agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_wait(agt_message_t message, jem_u64_t timeout_us);
JEM_api void*               JEM_stdcall agt_get_payload(agt_message_t message, jem_size_t* pMessageSize);




JEM_end_c_namespace


#endif//JEMSYS_AGATE_MAILBOX_H
