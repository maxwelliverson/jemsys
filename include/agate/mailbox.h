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

enum agt_mailbox_create_flag_bits_e {
  AGT_MAILBOX_CREATE_COHERENT_SEND    = 0x1,
  AGT_MAILBOX_CREATE_COHERENT_RECEIVE = 0x2,
  AGT_MAILBOX_CREATE_IPC_CAPABLE      = 0x4,
  AGT_MAILBOX_CREATE_DYNAMIC_MESSAGES = 0x8
};
enum agt_mailbox_flag_bits_e {
  AGT_MAILBOX_SINGLE_SENDER              = 0x01,
  AGT_MAILBOX_SINGLE_RECEIVER            = 0x02,
  AGT_MAILBOX_COHERENT_SEND              = 0x04,
  AGT_MAILBOX_COHERENT_RECEIVE           = 0x08,
  AGT_MAILBOX_INTERPROCESS_CAPABLE       = 0x10,
  AGT_MAILBOX_IS_DYNAMIC                 = 0x40
};
#define AGT_MAILBOX_KIND_BITMASK (AGT_MAILBOX_SINGLE_SENDER | AGT_MAILBOX_SINGLE_RECEIVER | AGT_MAILBOX_COHERENT_SEND | AGT_MAILBOX_COHERENT_RECEIVE)

typedef enum {
  AGT_MAILBOX_KIND_MPMC   = 0,
  AGT_MAILBOX_KIND_SPMC   = 1,
  AGT_MAILBOX_KIND_MPSC   = 2,
  AGT_MAILBOX_KIND_SPSC   = 3,
  // AGT_MAILBOX_KIND_CMSMR  = 4, // CoherentMultiSenderMultiReceiver
  // AGT_MAILBOX_KIND_CMSSR  = 6, // CoherentMultiSenderSingleReceiver
  // AGT_MAILBOX_KIND_MSCMR  = 8, // MultiSenderCoherentMultiReceiver
  // AGT_MAILBOX_KIND_SSCMR  = 9, // SingleSenderCoherentMultiReceiver
  // AGT_MAILBOX_KIND_CMSCMR = 15 // CoherentMultiSenderCoherentMultiReceiver
} agt_mailbox_kind_t;
typedef enum {
  AGT_MAILBOX_PARAM_TYPE_INTERPROCESS,
  AGT_MAILBOX_PARAM_TYPE_CLEANUP_CALLBACK
} agt_mailbox_param_type_t;

typedef jem_flags32_t agt_mailbox_create_flags_t;
typedef jem_flags32_t agt_mailbox_flags_t;

typedef struct agt_mailbox* agt_mailbox_t;


#define AGT_MAILBOX_CREATE_DEFAULT_FLAGS 0x0


typedef void(JEM_stdcall* agt_mailbox_cleanup_callback_t)(agt_mailbox_t mailbox, void* user_data);

typedef struct {
  const char* name;
  jem_size_t  name_length;
} agt_mailbox_interprocess_params_t;
typedef struct {
  agt_mailbox_cleanup_callback_t callback;
  void*                          data;
} agt_mailbox_cleanup_params_t;
typedef struct {
  jem_u32_t type;
  union{
    void*     pointer;
    jem_u32_t u32;
    jem_u64_t u64;
    float     f32;
    double    f64;
  };
} agt_mailbox_optional_params_t;

typedef struct {
  agt_mailbox_create_flags_t           flags;
  jem_u32_t                            max_senders;
  jem_u32_t                            max_receivers;
  jem_size_t                           slot_count;
  jem_size_t                           slot_size;
  jem_size_t                           optional_param_count;
  const agt_mailbox_optional_params_t* optional_params;
} agt_mailbox_create_info_t;





JEM_api jem_size_t          JEM_stdcall agt_get_dynamic_mailbox_granularity();



JEM_api agt_request_t       JEM_stdcall agt_mailbox_create(agt_mailbox_t* mailbox, const agt_mailbox_create_info_t* createInfo);
JEM_api void                JEM_stdcall agt_mailbox_destroy(agt_mailbox_t mailbox);

JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_consumer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_attach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us);
JEM_api agt_status_t        JEM_stdcall agt_mailbox_detach_producer(agt_mailbox_t mailbox, jem_u64_t timeout_us);

JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_message_size(agt_mailbox_t mailbox);
JEM_api agt_mailbox_flags_t JEM_stdcall agt_mailbox_get_flags(agt_mailbox_t mailbox);
JEM_api agt_mailbox_kind_t  JEM_stdcall agt_mailbox_get_kind(agt_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_senders(agt_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_receivers(agt_mailbox_t mailbox);


JEM_api agt_status_t        JEM_stdcall agt_start_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_message_t       JEM_stdcall agt_finish_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload);
JEM_api agt_status_t        JEM_stdcall agt_receive_message(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_discard_message(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_message(agt_mailbox_t mailbox, agt_message_t message);


JEM_api agt_status_t        JEM_stdcall agt_start_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void** pMessagePayload, jem_u64_t timeout_us);
JEM_api agt_message_t       JEM_stdcall agt_finish_send_message(agt_mailbox_t mailbox, jem_size_t messageSize, void* messagePayload);
JEM_api agt_status_t        JEM_stdcall agt_receive_message(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_discard_message(agt_mailbox_t mailbox, agt_message_t message);
JEM_api agt_status_t        JEM_stdcall agt_cancel_message(agt_mailbox_t mailbox, agt_message_t message);


JEM_end_c_namespace


#endif//JEMSYS_AGATE_MAILBOX_H
