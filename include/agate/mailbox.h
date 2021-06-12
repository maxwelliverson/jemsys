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

enum agt_mailbox_create_flag_bits_e{
  AGT_MAILBOX_CREATE_COHERENT_SEND    = 0x1,
  AGT_MAILBOX_CREATE_COHERENT_RECEIVE = 0x2,
  AGT_MAILBOX_CREATE_IPC_CAPABLE      = 0x4
};
enum agt_mailbox_flag_bits_e {
  AGT_MAILBOX_SINGLE_SENDER              = 0x01,
  AGT_MAILBOX_SINGLE_RECEIVER            = 0x02,
  AGT_MAILBOX_COHERENT_SEND              = 0x04,
  AGT_MAILBOX_COHERENT_RECEIVE           = 0x08,
  AGT_MAILBOX_INTERPROCESS               = 0x10,
  AGT_MAILBOX_RESIDES_IN_EXTERNAL_BUFFER = 0x20,
  AGT_MAILBOX_IS_DYNAMIC                 = 0x40
};
#define AGT_MAILBOX_KIND_BITMASK (AGT_MAILBOX_SINGLE_SENDER | AGT_MAILBOX_SINGLE_RECEIVER | AGT_MAILBOX_COHERENT_SEND | AGT_MAILBOX_COHERENT_RECEIVE)

typedef enum {
  AGT_MAILBOX_KIND_MSMR   = 0, // MultiSenderMultiReceiver
  AGT_MAILBOX_KIND_SSMR   = 1, // SingleSenderMultiReceiver
  AGT_MAILBOX_KIND_MSSR   = 2, // MultiSenderSingleReceiver,
  AGT_MAILBOX_KIND_SSSR   = 3, // SingleSenderSingleReceiver
  AGT_MAILBOX_KIND_CMSMR  = 4, // CoherentMultiSenderMultiReceiver
  AGT_MAILBOX_KIND_CMSSR  = 6, // CoherentMultiSenderSingleReceiver
  AGT_MAILBOX_KIND_MSCMR  = 8, // MultiSenderCoherentMultiReceiver
  AGT_MAILBOX_KIND_SSCMR  = 9, // SingleSenderCoherentMultiReceiver
  AGT_MAILBOX_KIND_CMSCMR = 15 // CoherentMultiSenderCoherentMultiReceiver
} agt_mailbox_kind_t;

typedef jem_flags32_t agt_mailbox_create_flags_t;
typedef jem_flags32_t agt_mailbox_flags_t;

typedef struct agt_mailbox*                                           agt_mailbox_t;
typedef struct agt_dynamic_mailbox*                                   agt_dynamic_mailbox_t;
typedef JEM_transparent_union_2(agt_mailbox_t, agt_dynamic_mailbox_t) agt_any_mailbox_t;



#define AGT_MAILBOX_CREATE_DEFAULT_FLAGS 0x0


typedef void(JEM_stdcall* PFN_agt_mailbox_destructor)(void* user_data, agt_any_mailbox_t mailbox);

typedef struct {
  const char* name;
} agt_mailbox_interprocess_params_t;
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
  agt_mailbox_flags_t        flags;
  jem_u32_t                  max_senders;
  jem_u32_t                  max_receivers;
  const jem_char_t*          name;
  jem_size_t                 name_length;          // If name is null, name_length is ignored. Otherwise, if name_length is AGT_DONT_CARE, then the string length of name will be determined with strlen. Otherwise, name_length is the length of name.
  void*                      external_buffer;      // If nonnull, must point to a region of memory with at least AGT_MAILBOX_SIZE writeable bytes, into which the mailbox will be constructed. The buffer must outlive all extant references to the mailbox (ie. senders, receivers, and the primary handle).
  jem_size_t                 external_buffer_size; // If external_buffer is nonnull, it must point to a region of memory with at least external_buffer_size writeable bytes. external_buffer_size must be at least AGT_MAILBOX_SIZE bytes, though AGT_MAILBOX_SIZE is not guaranteed to be sufficient.
  PFN_agt_mailbox_destructor destructor_callback;
  void*                      destructor_user_data;
} agt_mailbox_create_info_t;








JEM_api jem_status_t        JEM_stdcall agt_mailbox_create(agt_mailbox_t* mailbox, jem_size_t messageSlots, jem_size_t messageSize, const agt_mailbox_create_info_t*createInfo);
JEM_api jem_status_t        JEM_stdcall agt_dynamic_mailbox_create(agt_dynamic_mailbox_t* mailbox, jem_size_t mailbox_size, jem_size_t max_message_size, const agt_mailbox_create_info_t* create_info);
JEM_api void                JEM_stdcall agt_mailbox_destroy(agt_any_mailbox_t mailbox);


JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_message_size(agt_any_mailbox_t mailbox);
JEM_api agt_mailbox_flags_t JEM_stdcall agt_mailbox_get_flags(agt_any_mailbox_t mailbox);
JEM_api agt_mailbox_kind_t  JEM_stdcall agt_mailbox_get_kind(agt_any_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_senders(agt_any_mailbox_t mailbox);
JEM_api jem_size_t          JEM_stdcall agt_mailbox_get_max_receivers(agt_any_mailbox_t mailbox);

JEM_api bool                JEM_stdcall agt_mailbox_get_name(agt_any_mailbox_t mailbox, const jem_char_t** name, jem_size_t* length);
JEM_api bool                JEM_stdcall agt_mailbox_get_external_buffer(agt_any_mailbox_t mailbox, void** buffer, jem_size_t* size);

JEM_api jem_size_t          JEM_stdcall agt_get_dynamic_mailbox_granularity();




JEM_api jem_status_t        JEM_stdcall agt_start_send(agt_mailbox_t mailbox, void** pMsgAddress, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_finish_send(agt_mailbox_t mailbox, void* msgAddress);
JEM_api jem_status_t        JEM_stdcall agt_start_receive(agt_mailbox_t mailbox, void** pMsgAddress, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_finish_receive(agt_mailbox_t mailbox, void* msgAddress, agt_receive_action_t action);

JEM_api jem_status_t        JEM_stdcall agt_start_dynamic_send(agt_mailbox_t mailbox, void** pMsgAddress, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_finish_dynamic_send(agt_mailbox_t mailbox, void* msgAddress);
JEM_api jem_status_t        JEM_stdcall agt_start_dynamic_receive(agt_mailbox_t mailbox, void** pMsgAddress, jem_u64_t timeout_us);
JEM_api void                JEM_stdcall agt_finish_dynamic_receive(agt_mailbox_t mailbox, void* msgAddress, agt_receive_action_t action);



JEM_end_c_namespace


#endif//JEMSYS_AGATE_MAILBOX_H
