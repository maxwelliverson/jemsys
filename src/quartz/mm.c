//
// Created by maxwe on 2021-06-01.
//

#include "internal.h"

#include "quartz/mm.h"


JEM_stdcall qtz_request_t   qtz_alloc_static_mailbox(void **pResultAddr, jem_u32_t messageSlots, jem_u32_t messageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_static_mailbox(void *mailboxAddr);
JEM_stdcall qtz_request_t   qtz_alloc_dynamic_mailbox(void **pResultAddr, jem_u32_t minMailboxSize, jem_u32_t maxMessageSize, const qtz_shared_mailbox_params_t *pSharedParams);
JEM_stdcall void            qtz_free_dynamic_mailbox(void *mailboxAddr);

