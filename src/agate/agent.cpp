//
// Created by maxwe on 2021-10-04.
//

#include "agents.hpp"
#include "common.hpp"


#include <cstring>

inline constexpr static jem_size_t AgentSizeOffset = sizeof(void*);

inline static void push_agent(agt_slot_t* slot, agt_agent_t agent) noexcept {
  std::memcpy(slot->payload, &agent, AgentSizeOffset);
  slot->payload = reinterpret_cast<char*>(slot->payload) + AgentSizeOffset;
}

inline static agt_agent_t pop_agent(agt_message_t* message) noexcept {
  agt_agent_t agent;
  std::memcpy(&agent, message->payload, AgentSizeOffset);
  message->payload = reinterpret_cast<char*>(message->payload) + AgentSizeOffset;
  message->size -= AgentSizeOffset;
  return agent;
}


inline static bool try_process_next_message(agt_mailbox_t mailbox, jem_u64_t usTimeout) noexcept {
  agt_agent_t    agent;
  agt_message_t  messageValue;
  agt_message_t* message;
  agt_status_t   result;

  message = &messageValue;

  result = agt_mailbox_receive(mailbox, message, usTimeout);

  if (result == AGT_ERROR_MAILBOX_IS_EMPTY)
    return false;

  JEM_assert(result == AGT_SUCCESS);

  agent = pop_agent(message);

  agt_set_self(agent);

  result = agent->m_actor.proc(agent->m_actor.state, message);

  if (result != AGT_DEFERRED)
    agt_return(message, result);

  return true;
}



extern "C" {


  // TODO: Implement agt_create_agent
  // TODO: Implement agt_destroy_agent
  JEM_api agt_status_t  JEM_stdcall agt_create_agent(agt_agent_t* pAgent, const agt_create_agent_params_t* cpParams) JEM_noexcept {
    return AGT_ERROR_NOT_YET_IMPLEMENTED;
  }
  JEM_api void          JEM_stdcall agt_destroy_agent(agt_agent_t agent) JEM_noexcept {}



  JEM_api agt_status_t JEM_stdcall agt_agent_acquire_slot(agt_agent_t agent, agt_slot_t* slot, jem_size_t slotSize, jem_u64_t timeout) JEM_noexcept {

    constexpr static jem_size_t AgentSizeOffset = sizeof(void*);

    agt_status_t  status;

    if (agent == nullptr /*|| slot == nullptr*/) [[unlikely]]
      return AGT_ERROR_INVALID_ARGUMENT;

    status = agt_mailbox_acquire_slot(getAgentMailbox(agent), slot, slotSize + AgentSizeOffset, timeout);

    if ( status == AGT_SUCCESS ) [[likely]]
      push_agent(slot, agent);

    return status;
  }
}