//
// Created by maxwe on 2021-10-26.
//

#ifndef JEMSYS_AGATE_INTERNAL_AGENTS_HPP
#define JEMSYS_AGATE_INTERNAL_AGENTS_HPP

#include <agate.h>

namespace agt {
  struct actor {
    jem_global_id_t  typeId;
    agt_actor_proc_t proc;
    agt_actor_dtor_t dtor;
    void*            state;
  };
  struct driver {

  };
}

extern "C" {

  struct agt_agency {

  };


                              //         Creator process ID       Privacy           Local ID
                              //                 |                   |                 |
  struct agt_agent {          // [             32-63              ][31][             00-30            ]
    jem_global_id_t m_id;     // |00000000000000000000000000000000||0||0000000000000000000000000000000|
    agt_agent_t     m_parent;
    agt_agency_t    m_agency;
    agt::actor      m_actor;
    agt::driver     m_driver;
  };

}

inline static agt_mailbox_t getAgentMailbox(agt_agent_t agent) noexcept;

#endif//JEMSYS_AGATE_INTERNAL_AGENTS_HPP
