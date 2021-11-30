//
// Created by maxwe on 2021-11-21.
//

#ifndef JEMSYS_QUARTZ2_H
#define JEMSYS_QUARTZ2_H

#include "jemsys.h"

JEM_begin_c_namespace


#define QTZ_KERNEL_VERSION_MOST_RECENT ((jem_u32_t)-1)
#define QTZ_DEFAULT_PRIORITY ((jem_u32_t)0)
#define QTZ_THIS_PROCESS ((qtz_process_t)NULL)



typedef jem_u32_t  QtzU32;
typedef jem_i64_t  QtzU64;
typedef jem_size_t QtzSize;

#define QTZ_DEFINE_HANDLE(type) typedef struct type##_st* type

QTZ_DEFINE_HANDLE(QtzRequest);
QTZ_DEFINE_HANDLE(QtzProcess);

typedef enum QtzStatus {
  QTZ_SUCCESS,
  QTZ_NOT_READY,
  QTZ_DISCARDED,
  QTZ_ERROR_TIMED_OUT,
  QTZ_ERROR_NOT_INITIALIZED,
  QTZ_ERROR_INTERNAL,
  QTZ_ERROR_UNKNOWN,
  QTZ_ERROR_BAD_SIZE,
  QTZ_ERROR_INVALID_KERNEL_VERSION,
  QTZ_ERROR_INVALID_ARGUMENT,
  QTZ_ERROR_BAD_ENCODING_IN_NAME,
  QTZ_ERROR_NAME_TOO_LONG,
  QTZ_ERROR_NAME_ALREADY_IN_USE,
  QTZ_ERROR_INSUFFICIENT_BUFFER_SIZE,
  QTZ_ERROR_BAD_ALLOC,
  QTZ_ERROR_NOT_IMPLEMENTED,
  QTZ_ERROR_UNSUPPORTED_OPERATION,
  QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE
} QtzStatus;




/**
 * @brief Returns a semi-portable handle to the current process.
 *
 * @note Calling this function for immediate use as an argument of qtz_send is much less
 *       efficient than simply using the QTZ_THIS_PROCESS macro.
 *
 * @returns A handle to the current process.
 * */
JEM_api QtzProcess JEM_stdcall qtzGetProcess() JEM_noexcept;

/**
 * @brief Sends a message to process message queues.
 *
 * @param [in, optional] process A handle to the process that will receive this message.
 * If a null pointer is passed, the message is sent to the calling process. The macro QTZ_THIS_PROCESS
 * is intended to be used for this purpose.
 * @param [in, optional] priority The priority of the message. Lower valued integers are higher priority,
 * where 1 is the highest priority. Zero is a special value that indicates the message has default priority.
 * The macro QTZ_DEFAULT_PRIORITY is intended to be used for this purpose. Default priorities are specific
 * to the receiving process, and are not static.
 * @param [in] messageId Identifies the message type.
 * @param [in] messageBuffer A buffer that holds the raw message data. This buffer can be in temporary
 * storage, as its contents are copied during the call to qtz_send. The exact format of the buffer is
 * highly specific to the value of messageId.
 * @param [in] timeoutMicroseconds Desired timeout in microseconds. Ultimately, message storage capacity
 * for any given process is limited, and while it shouldn't be an issue in most circumstances, it is
 * possible for a process' message queue to be full. In these cases, qtz_send will optionally wait for a
 * spot in the queue to open up, or until the amount of time specified by timeoutMicroseconds has passed.
 * The macro JEM_WAIT can be specified here to indicate a (functionally) infinite timeout. Likewise, the
 * macro JEM_DO_NOT_WAIT can be specified to indicate that the call should immediately return if the queue
 * is full. In the case that the send is unsuccessful, a null pointer is returned.
 *
 * */
JEM_api QtzRequest JEM_stdcall qtzSend(QtzProcess process, QtzU32 priority, QtzU32 messageId, const void* messageBuffer, QtzU64 timeoutMicroseconds) JEM_noexcept;

/**
 * @brief Waits for the specified message to finish being processed, and then retrieves the result.
 *
 * @returns [QTZ_NOT_READY] => Indicates the message hasn't yet finished processing.
 *          any other value => The result of the processed message.
 * */
JEM_api QtzStatus  JEM_stdcall qtzWait(QtzRequest message, QtzU64 timeout_us) JEM_noexcept;

/**
 * @brief Discards the specified message, informing the subsystem that the sender is not interested
 * in the result.
 * */
JEM_api void       JEM_stdcall qtzDiscard(QtzRequest message) JEM_noexcept;




JEM_end_c_namespace


#endif//JEMSYS_QUARTZ2_H
