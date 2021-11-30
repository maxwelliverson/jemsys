//
// Created by maxwe on 2021-10-01.
//

#ifndef JEMSYS_AGATE_H
#define JEMSYS_AGATE_H

#include "jemsys.h"

JEM_begin_c_namespace

/*
 *
 *
 * [message]: The discrete units of data sent between entities.
 *
 * [slot]: The memory owned by a given mailbox in which a message can exist. Each mailbox
 *         has a static number of available slots, specified during mailbox creation.
 *
 * [signal]: An opaque object used to communicate a response to the sender of a given
 *           message.
 *
 *
 * [mailbox]: A basic message queue object that is completely agnostic to the contents
 *            of the messages being sent, and how they are to be interpretted. The
 *            types of mailbox are parametrized by locality, the maximum number of
 *            senders, and the maximum number of receivers. To ensure code correctness,
 *            any given atomic entity must attach itself as a sender to a given mailbox
 *            before the entity can send a message to that mailbox. Likewise, any given
 *            atomic entity must attach itself as a receiver to a given mailbox before
 *            the entity can receive messages from that mailbox.
 *
 *            Operations:
 *              - begin_send:   mailbox -> slot
 *              - cancel_send: (mailbox, slot) -> {}
 *              - finish_send: (mailbox, slot) -> signal
 *              - receive:      mailbox -> message
 *
 *              - attach:       mailbox -> bool
 *              - detach:       mailbox -> {}
 *
 *            Types:
 *              - private_mailbox:     no operations are atomic with respect to anything. Cannot be used to communicate between different processes or different threads.
 *              - local_spsc_mailbox:  send and receives are atomic with respect to each other, neither sequential send nor sequential receive operations are atomic with respect to themselves. Cannot be used to communicate between different processes.
 *              - local_mpsc_mailbox:  send and receives are atomic with respect to each other, send is atomic with respect to itself, but receive is not atomic with respect to itself. Cannot be used to communicate between different processes.
 *              - local_spmc_mailbox:  send and receives are atomic with respect to each other, receive is atomic with respect to itself, but send is not atomic with respect to itself. Cannot be used to communicate between different processes.
 *              - local_mpmc_mailbox:  all operations are atomic with respect to each other. Cannot be used to communicate between different processes.
 *              - shared_spsc_mailbox: send and receives are atomic with respect to each other, neither sequential send nor sequential receive operations are atomic with respect to themselves.
 *              - shared_mpsc_mailbox: send and receives are atomic with respect to each other, send is atomic with respect to itself, but receive is not atomic with respect to itself.
 *              - shared_spmc_mailbox: send and receives are atomic with respect to each other, receive is atomic with respect to itself, but send is not atomic with respect to itself.
 *              - shared_mpmc_mailbox: all operations are atomic with respect to each other.

 *
 * [actor]: An opaque state object coupled with a message dispatch table.
 *
 *
 * [agent]: A higher level abstraction that couples an actor with a mailbox. Agents are
 *          a realization of the more general concept of an "entity"
 *
 *   -  synchronous_agent
 *   -  cooperative_agent
 *   -         lazy_agent
 *   -       thread_agent
 *   -  thread_pool_agent
 *   -        proxy_agent
 *   -      virtual_agent
 *   -    corporate_agent
 *   -    scheduled_agent
 *
 *
 *
 *
 *
 * */

typedef enum agt_status_t {
  AGT_SUCCESS,
  AGT_NOT_READY,
  AGT_DEFERRED,
  AGT_CANCELLED,
  AGT_TIMED_OUT,
  AGT_ERROR_UNKNOWN,
  AGT_ERROR_UNBOUND_AGENT,
  AGT_ERROR_FOREIGN_SENDER,
  AGT_ERROR_STATUS_NOT_SET,
  AGT_ERROR_UNKNOWN_MESSAGE_TYPE,
  AGT_ERROR_INVALID_FLAGS,
  AGT_ERROR_INVALID_MESSAGE,
  AGT_ERROR_INVALID_SIGNAL,
  AGT_ERROR_CANNOT_DISCARD,
  AGT_ERROR_MESSAGE_TOO_LARGE,
  AGT_ERROR_INSUFFICIENT_SLOTS,
  AGT_ERROR_BAD_SIZE,
  AGT_ERROR_INVALID_ARGUMENT,
  AGT_ERROR_BAD_ENCODING_IN_NAME,
  AGT_ERROR_NAME_TOO_LONG,
  AGT_ERROR_NAME_ALREADY_IN_USE,
  AGT_ERROR_BAD_ALLOC,
  AGT_ERROR_MAILBOX_IS_FULL,
  AGT_ERROR_MAILBOX_IS_EMPTY,
  AGT_ERROR_TOO_MANY_SENDERS,
  AGT_ERROR_TOO_MANY_RECEIVERS,
  AGT_ERROR_ALREADY_RECEIVED,
  AGT_ERROR_INITIALIZATION_FAILED,
  AGT_ERROR_NOT_YET_IMPLEMENTED
} agt_status_t;

typedef struct agt_signal*   agt_signal_t;
typedef struct agt_cookie*   agt_cookie_t;
typedef struct agt_mailbox*  agt_mailbox_t;

typedef struct agt_agent*    agt_agent_t;
typedef struct agt_agency*   agt_agency_t;


typedef agt_status_t (*agt_agent_acquire_slot_proc_t)(agt_agent_t agent, struct agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout_us);

typedef agt_status_t (*agt_actor_proc_t)(void* actorState, const struct agt_message_t* message);
typedef void         (*agt_actor_dtor_t)(void* actorState);


typedef struct agt_actor_t {
  jem_global_id_t  type;
  agt_actor_proc_t pfnMessageProc;
  agt_actor_dtor_t pfnDtor;
  void*            state;
} agt_actor_t;
typedef struct agt_slot_t {
  agt_cookie_t     cookie;
  jem_global_id_t  id;
  void*            payload;
} agt_slot_t;
typedef struct agt_message_t {
  agt_cookie_t    cookie;
  jem_size_t      size;
  jem_global_id_t id;
  void*           payload;
} agt_message_t;






typedef enum agt_send_flag_bits_t {
  AGT_MUST_CHECK_RESULT = 0x1, // TODO: implement enforcement for AGT_MUST_CHECK_RESULT. Currently does nothing
  AGT_IGNORE_RESULT     = 0x2,
  AGT_FAST_CLEANUP      = 0x4
} agt_send_flag_bits_t;


typedef enum agt_mailbox_scope_t {
  AGT_MAILBOX_SCOPE_LOCAL,
  AGT_MAILBOX_SCOPE_SHARED,
  AGT_MAILBOX_SCOPE_PRIVATE
} agt_mailbox_scope_t;
typedef enum agt_connect_action_t {
  AGT_ATTACH_SENDER,
  AGT_DETACH_SENDER,
  AGT_ATTACH_RECEIVER,
  AGT_DETACH_RECEIVER
} agt_connect_action_t;
typedef enum agt_agency_action_t {
  AGT_AGENCY_ACTION_CONTINUE,
  AGT_AGENCY_ACTION_DEFER,
  AGT_AGENCY_ACTION_YIELD,
  AGT_AGENCY_ACTION_CLOSE
} agt_agency_action_t;


typedef jem_flags32_t agt_send_flags_t;


// typedef agt_agency_action_t (*agt_agency_proc_t)();




typedef struct agt_create_mailbox_params_t {
  jem_size_t          min_slot_count;
  jem_size_t          min_slot_size;
  jem_u32_t           max_senders;
  jem_u32_t           max_receivers;
  agt_mailbox_scope_t scope;
  void*               async_handle_address;
  char                name[JEM_CACHE_LINE*2];
} agt_create_mailbox_params_t;
typedef struct agt_create_agent_params_t {

} agt_create_agent_params_t;
typedef struct agt_create_agency_params_t {

} agt_create_agency_params_t;




/**
 *
 * msgProc = (message) -> status
 * actor = { state, msgProc }
 * agent  = { actor, ref executor }
 * executor = {  }
 * agency = {  }
 *
 *
 * Modes:
 *  - agent :: callable from agt_actor_proc_t functions
 *  -
 *  -
 *  -
 *
 * */




/*
struct agt_mailslot {
  char        reserved[JEM_CACHE_LINE - sizeof(void*)];
  jem_u64_t   messageId;
  char        payload[];
};




typedef struct agt_message_info_t {
  agt_cookie_t    cookie;
  jem_size_t      size;
  jem_global_id_t id;
  void*           payload;
} agt_message_info_t;
*/



/*
 * Object Creation/Destruction
 * */

JEM_api agt_status_t  JEM_stdcall agt_create_mailbox(agt_mailbox_t* pMailbox, const agt_create_mailbox_params_t* params) JEM_noexcept;
JEM_api agt_mailbox_t JEM_stdcall agt_copy_mailbox(agt_mailbox_t mailbox) JEM_noexcept;
JEM_api void          JEM_stdcall agt_close_mailbox(agt_mailbox_t mailbox) JEM_noexcept;




/*
 * Mailboxes
 * */



JEM_api agt_status_t  JEM_stdcall agt_mailbox_connect(agt_mailbox_t mailbox, agt_connect_action_t action, jem_u64_t usTimeout) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_mailbox_acquire_slot(agt_mailbox_t mailbox, agt_slot_t* pSlot, jem_size_t slotSize, jem_u64_t usTimeout) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_mailbox_receive(agt_mailbox_t mailbox, agt_message_t* pMessage, jem_u64_t usTimeout) JEM_noexcept;

JEM_api agt_signal_t  JEM_stdcall agt_send(const agt_slot_t* cpSlot, agt_agent_t sender, agt_send_flags_t flags) JEM_noexcept;
JEM_api void          JEM_stdcall agt_return(const agt_message_t* cpMessage, agt_status_t status) JEM_noexcept;

JEM_api agt_status_t  JEM_stdcall agt_get_sender(const agt_message_t* cpMessage, agt_agent_t* pSender, void* asyncHandle) JEM_noexcept;


/*
 * Messages
 * */



/*
 * Agents
 * */

JEM_api agt_status_t  JEM_stdcall agt_create_agent(agt_agent_t* pAgent, const agt_create_agent_params_t* cpParams) JEM_noexcept;
JEM_api void          JEM_stdcall agt_close_agent(agt_agent_t agent) JEM_noexcept;

JEM_api void          JEM_stdcall agt_set_self(agt_agent_t self) JEM_noexcept;
JEM_api agt_agent_t   JEM_stdcall agt_self() JEM_noexcept;


JEM_api agt_status_t  JEM_stdcall agt_agent_acquire_slot(agt_agent_t agent, agt_slot_t* pSlot, jem_size_t slotSize, jem_u64_t usTimeout) JEM_noexcept;


/*
 * Signals
 * */

JEM_api agt_signal_t  JEM_stdcall agt_signal_copy(agt_signal_t signal) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_signal_wait(agt_signal_t signal, jem_u64_t usTimeout) JEM_noexcept;
JEM_api void          JEM_stdcall agt_signal_discard(agt_signal_t signal) JEM_noexcept;




JEM_end_c_namespace

#endif//JEMSYS_AGATE_H
