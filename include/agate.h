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


typedef struct agt_signal*  agt_signal_t;
typedef struct agt_message* agt_message_t;
typedef struct agt_mailbox* agt_mailbox_t;
typedef struct agt_agent*   agt_agent_t;

typedef struct agt_slot*    agt_slot_t;
typedef struct agt_actor*   agt_actor_t;
typedef struct agt_self*    agt_self_t;

typedef enum agt_status_t {
  AGT_SUCCESS,
  AGT_NOT_READY,
  AGT_CANCELLED,
  AGT_TIMED_OUT,
  AGT_ERROR_UNKNOWN,
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
  AGT_ERROR_BAD_ALLOC,
  AGT_ERROR_MAILBOX_IS_FULL,
  AGT_ERROR_MAILBOX_IS_EMPTY,
  AGT_ERROR_TOO_MANY_SENDERS,
  AGT_ERROR_TOO_MANY_RECEIVERS,
  AGT_ERROR_ALREADY_RECEIVED,
  AGT_ERROR_NOT_YET_IMPLEMENTED
} agt_status_t;

enum {
  AGT_MUST_CHECK_RESULT = 0x1, // TODO: implement enforcement for AGT_MUST_CHECK_RESULT. Currently does nothing
  AGT_IGNORE_RESULT     = 0x2,
  AGT_FAST_CLEANUP      = 0x4
};
typedef jem_flags32_t agt_send_flags_t;


typedef agt_status_t(*agt_actor_proc_t)(agt_self_t self, jem_u64_t messageId, void* messageData, void* actorState);

typedef const struct agt_mailbox_vtable{
  agt_slot_t(   JEM_stdcall* pfn_acquire_slot)(agt_mailbox_t mailbox, jem_size_t messageSize, jem_u64_t timeout_us) JEM_noexcept;
  void(         JEM_stdcall* pfn_release_slot)(agt_mailbox_t mailbox, agt_slot_t slot) JEM_noexcept;
  agt_signal_t( JEM_stdcall* pfn_send)(agt_mailbox_t mailbox, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept;
  agt_message_t(JEM_stdcall* pfn_receive)(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
}* agt_mailbox_vtable_t;



struct agt_slot{
  char        reserved[JEM_CACHE_LINE - (2 * sizeof(void*))];
  agt_agent_t sender;
  jem_u64_t   id;
  char        payload[];
};
struct agt_self{
  agt_agent_t self;
  agt_actor_t actor;
};
struct agt_actor{
  agt_actor_proc_t proc;
  void*            state;
};



/*
 * Object Creation/Destruction
 * */

JEM_api agt_status_t JEM_stdcall agt_create_mailbox(agt_mailbox_t* pMailbox) JEM_noexcept;
JEM_api void         JEM_stdcall agt_destroy_mailbox(agt_mailbox_t mailbox) JEM_noexcept;

JEM_api agt_status_t JEM_stdcall agt_create_agent(agt_agent_t* pAgent) JEM_noexcept;
JEM_api void         JEM_stdcall agt_destroy_agent(agt_agent_t agent) JEM_noexcept;

JEM_api agt_status_t JEM_stdcall agt_create_actor(agt_actor_t* pActor) JEM_noexcept;
JEM_api void         JEM_stdcall agt_destroy_actor(agt_actor_t actor) JEM_noexcept;



/*
 * Signals
 * */

JEM_api agt_signal_t JEM_stdcall agt_signal_copy(agt_signal_t signal) JEM_noexcept;
JEM_api agt_status_t JEM_stdcall agt_signal_wait(agt_signal_t signal, jem_u64_t timeout_us) JEM_noexcept;
JEM_api void         JEM_stdcall agt_signal_discard(agt_signal_t signal) JEM_noexcept;


/*
 * Messages
 * */

JEM_api void         JEM_stdcall agt_message_raise_signal(agt_message_t message, agt_status_t status) JEM_noexcept;
JEM_api void*        JEM_stdcall agt_message_get_payload(agt_message_t message, jem_size_t* pMessageSize) JEM_noexcept;





/*
 * Mailboxes
 * */

JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_sender(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
JEM_api void          JEM_stdcall agt_mailbox_detach_sender(agt_mailbox_t mailbox) JEM_noexcept;
JEM_api agt_status_t  JEM_stdcall agt_mailbox_attach_receiver(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;
JEM_api void          JEM_stdcall agt_mailbox_detach_receiver(agt_mailbox_t mailbox) JEM_noexcept;

JEM_api agt_slot_t    JEM_stdcall agt_mailbox_acquire_slot(agt_mailbox_t mailbox, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
JEM_api void          JEM_stdcall agt_mailbox_release_slot(agt_mailbox_t mailbox, agt_slot_t slot) JEM_noexcept;
JEM_api agt_signal_t  JEM_stdcall agt_mailbox_send(agt_mailbox_t mailbox, agt_slot_t slot, agt_send_flags_t flags) JEM_noexcept;
JEM_api agt_message_t JEM_stdcall agt_mailbox_receive(agt_mailbox_t mailbox, jem_u64_t timeout_us) JEM_noexcept;

JEM_api agt_mailbox_vtable_t JEM_stdcall agt_mailbox_lookup_vtable(agt_mailbox_t mailbox) JEM_noexcept;


/*
 * Agents
 * */

JEM_api agt_slot_t    JEM_stdcall agt_agent_acquire_slot(agt_agent_t agent, jem_size_t slot_size, jem_u64_t timeout_us) JEM_noexcept;
JEM_api void          JEM_stdcall agt_agent_release_slot(agt_agent_t agent, agt_slot_t slot) JEM_noexcept;
JEM_api agt_signal_t  JEM_stdcall agt_agent_send(agt_agent_t agent, agt_slot_t slot) JEM_noexcept;






JEM_end_c_namespace

#endif//JEMSYS_AGATE_H
