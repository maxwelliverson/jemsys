//
// Created by maxwe on 2021-10-26.
//

#ifndef JEMSYS_AGATE_INTERNAL_IMPL_MACROS_HPP
#define JEMSYS_AGATE_INTERNAL_IMPL_MACROS_HPP

#define AGT_UNINIT_ID ((jem_global_id_t)-1)

#define AGT_acquire_semaphore(sem, timeout, err) do { \
  switch (timeout) {                                          \
    case JEM_WAIT:                                    \
      sem.acquire();                                  \
      break;                                          \
    case JEM_DO_NOT_WAIT:                             \
      if (!sem.try_acquire())                         \
        return err;                                   \
      break;                                          \
    default:                                          \
      if (!sem.try_acquire_for(timeout))              \
        return err;\
  }                                                    \
  } while(false)

#define AGT_read_msg(msg, kookie) do {        \
      agt_cookie_t _tmp_kookie = kookie;      \
      msg->cookie = _tmp_kookie;              \
      msg->size   = _tmp_kookie->payloadSize; \
      msg->id     = _tmp_kookie->id;          \
      msg->payload = _tmp_kookie->payload;    \
  } while(false)

#define AGT_acquire_next_queued(mbox, timeout) do { \
    switch (timeout) {                                       \
      case JEM_WAIT:                                \
        mbox->queuedMessages.acquire();             \
        break;                                      \
      case JEM_DO_NOT_WAIT:                         \
        if (!mbox->queuedMessages.try_acquire())    \
          return AGT_ERROR_MAILBOX_IS_EMPTY;        \
        break;                                      \
      default:                                      \
        if (!mbox->queuedMessages.try_acquire_for(timeout))  \
          return AGT_ERROR_MAILBOX_IS_EMPTY;\
    }                                                  \
  } while(false)

#define AGT_connect(mbox, act, timeout) do { \
  switch (action) { \
  case AGT_ATTACH_SENDER:                    \
    AGT_acquire_semaphore(mbox ->producerSemaphore, timeout, AGT_ERROR_TOO_MANY_SENDERS);\
    break; \
  case AGT_DETACH_SENDER: \
    mbox->producerSemaphore.release(); \
    break; \
  case AGT_ATTACH_RECEIVER:                  \
    AGT_acquire_semaphore(mbox ->consumerSemaphore, timeout, AGT_ERROR_TOO_MANY_RECEIVERS);\
    break; \
  case AGT_DETACH_RECEIVER: \
    mbox->consumerSemaphore.release(); \
    break; \
    JEM_no_default; \
    }                                        \
  } while(false)

#if !defined(NDEBUG)
#define AGT_init_slot(slot, msg, size) do {  \
    agt_cookie_t _tmp_msg = msg;             \
    slot->cookie  = _tmp_msg;               \
    _tmp_msg->payloadSize = size;  \
    slot->id      = AGT_UNINIT_ID;     \
    slot->payload = _tmp_msg->payload; \
  } while(false)
#define AGT_prep_slot(slot) do { \
  slot->cookie->id = slot->id;   \
  JEM_assert(slot->id != AGT_UNINIT_ID && "message id was not set"); \
  } while(false)
#else
#define AGT_init_slot(msg, slot, size) do { \
    agt_cookie_t _tmp_msg = msg;            \
    slot->cookie  = _tmp_msg;               \
    _tmp_msg->payloadSize = size;  \
    slot->payload = _tmp_msg->payload; \
  } while(false)
#define AGT_prep_slot(slot) do { \
  slot->cookie->id = slot->id;   \
  } while(false)
#endif

#endif//JEMSYS_AGATE_INTERNAL_IMPL_MACROS_HPP
