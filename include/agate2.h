//
// Created by maxwe on 2021-11-03.
//

#ifndef JEMSYS_AGATE2_H
#define JEMSYS_AGATE2_H

#include <jemsys.h>


#define AGT_INVALID_OBJECT_ID ((AgtObjectId)-1)
#define AGT_NULL_HANDLE ((AgtHandle)JEM_NULL_HANDLE)
#define AGT_SYNCHRONIZE ((AgtAsync)JEM_NULL_HANDLE)


#define AGT_DEFINE_HANDLE(type) typedef struct type##_st* type
#define AGT_DEFINE_DISPATCH_HANDLE(type) typedef struct type##_st* type


JEM_begin_c_namespace


typedef jem_i32_t AgtBool;

typedef jem_u8_t  AgtUInt8;
typedef jem_i8_t  AgtInt8;
typedef jem_u16_t AgtUInt16;
typedef jem_i16_t AgtInt16;
typedef jem_u32_t AgtUInt32;
typedef jem_i32_t AgtInt32;
typedef jem_u64_t AgtUInt64;
typedef jem_i64_t AgtInt64;

typedef jem_u64_t AgtSize;
typedef jem_u64_t AgtTimeout;

typedef jem_u32_t AgtProtocolId;
typedef jem_u64_t AgtMessageId;
typedef jem_u64_t AgtTypeId;
typedef jem_u64_t AgtObjectId;



typedef void* AgtHandle;

AGT_DEFINE_HANDLE(AgtContext);
AGT_DEFINE_HANDLE(AgtAsync);
AGT_DEFINE_HANDLE(AgtSignal);
AGT_DEFINE_HANDLE(AgtMessage);









typedef enum AgtStatus {
  AGT_SUCCESS,
  AGT_NOT_READY,
  AGT_DEFERRED,
  AGT_CANCELLED,
  AGT_TIMED_OUT,
  AGT_ERROR_UNKNOWN,
  AGT_ERROR_UNKNOWN_FOREIGN_OBJECT,
  AGT_ERROR_INVALID_OBJECT_ID,
  AGT_ERROR_EXPIRED_OBJECT_ID,
  AGT_ERROR_NULL_HANDLE,
  AGT_ERROR_INVALID_HANDLE_TYPE,
  AGT_ERROR_NOT_BOUND,
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
} AgtStatus;

typedef enum AgtHandleType {
  AGT_CHANNEL,
  AGT_AGENT,
  AGT_SOCKET,
  AGT_SENDER,
  AGT_RECEIVER,
  AGT_THREAD,
  AGT_AGENCY
} AgtHandleType;

typedef enum AgtHandleFlagBits {
  AGT_OBJECT_IS_SHARED = 0x1,
  AGT_HANDLE_FLAGS_MAX
} AgtHandleFlagBits;
typedef jem_flags32_t AgtHandleFlags;

typedef enum AgtSendFlagBits {
  AGT_WAIT_FOR_ALL      = 0x0,
  AGT_MUST_CHECK_RESULT = 0x1, // TODO: implement enforcement for AGT_MUST_CHECK_RESULT. Currently does nothing
  AGT_IGNORE_RESULT     = 0x2,
  AGT_FAST_CLEANUP      = 0x4,
  AGT_WAIT_FOR_ANY      = 0x8
} AgtSendFlagBits;
typedef jem_flags32_t AgtSendFlags;

typedef enum AgtScope {
  AGT_SCOPE_LOCAL,
  AGT_SCOPE_SHARED,
  AGT_SCOPE_PRIVATE
} AgtScope;

typedef enum AgtAgencyAction {
  AGT_AGENCY_ACTION_CONTINUE,
  AGT_AGENCY_ACTION_DEFER,
  AGT_AGENCY_ACTION_YIELD,
  AGT_AGENCY_ACTION_CLOSE
} AgtAgencyAction;



typedef struct AgtSendInfo {
  AgtSize      messageSize;
  const void*  pMessageBuffer;
  AgtAsync     asyncHandle;
  AgtSendFlags flags;
} AgtSendInfo;

typedef struct AgtStagedMessage {
  void*        cookie;
  AgtMessageId id;
  void*        payload;
} AgtStagedMessage;

typedef struct AgtMessageInfo {
  AgtMessage   message;
  AgtSize      size;
  AgtMessageId id;
  void*        payload;
} AgtMessageInfo;

typedef struct AgtObjectInfo {
  AgtObjectId    id;
  AgtHandle      handle;
  AgtHandleType  type;
  AgtHandleFlags flags;
  AgtObjectId    exportId;
} AgtObjectInfo;




typedef AgtStatus (*PFN_agtAgentStageMessage)(AgtHandle agent, AgtStagedMessage* pStagedMessage, size_t slotSize, AgtTimeout timeout);

typedef AgtStatus (*PFN_agtActorMessageProc)(void* actorState, const AgtMessageInfo* message);
typedef void      (*PFN_agtActorDtor)(void* actorState);

typedef struct AgtActor {
  AgtTypeId               type;
  PFN_agtActorMessageProc pfnMessageProc;
  PFN_agtActorDtor        pfnDtor;
  void*                   state;
} AgtActor;






typedef struct AgtChannelCreateInfo {
  AgtSize     minCapacity;
  AgtSize     maxMessageSize;
  AgtSize     maxSenders;
  AgtSize     maxReceivers;
  AgtScope    scope;
  AgtAsync    asyncHandle;
  const char* name;
} AgtChannelCreateInfo;
typedef struct AgtSocketCreateInfo {

} AgtSocketCreateInfo;
typedef struct AgtAgentCreateInfo {

} AgtAgentCreateInfo;
typedef struct AgtAgencyCreateInfo {

} AgtAgencyCreateInfo;
typedef struct AgtThreadCreateInfo {

} AgtThreadCreateInfo;



JEM_api AgtStatus     JEM_stdcall agtNewContext(AgtContext* pContext) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtDestroyContext(AgtContext context) JEM_noexcept;


JEM_api void          JEM_stdcall agtYieldExecution() JEM_noexcept;


JEM_api AgtStatus     JEM_stdcall agtGetObjectInfo(AgtContext context, AgtObjectInfo* pObjectInfo) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtDuplicateHandle(AgtHandle inHandle, AgtHandle* pOutHandle) JEM_noexcept;
JEM_api void          JEM_stdcall agtCloseHandle(AgtHandle handle) JEM_noexcept;



JEM_api AgtStatus     JEM_stdcall agtCreateChannel(const AgtChannelCreateInfo* cpCreateInfo, AgtHandle* pSender, AgtHandle* pReceiver) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateAgent(const AgtAgentCreateInfo* cpCreateInfo, AgtHandle* pAgent) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateAgency(const AgtAgencyCreateInfo* cpCreateInfo, AgtHandle* pAgency) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtCreateThread(const AgtThreadCreateInfo* cpCreateInfo, AgtHandle* pThread) JEM_noexcept;



JEM_api AgtStatus     JEM_stdcall agtStage(AgtHandle sender, AgtStagedMessage* pStagedMessage, AgtSize messageSize, AgtTimeout usTimeout) JEM_noexcept;
JEM_api void          JEM_stdcall agtSendStaged(const AgtStagedMessage* cpStagedMessage, AgtHandle sender, AgtAsync asyncHandle, AgtSendFlags flags) JEM_noexcept;

JEM_api AgtStatus     JEM_stdcall agtSend(AgtHandle sender, const AgtSendInfo* pSendInfo, AgtTimeout timeout) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtReceive(AgtHandle receiver, AgtMessageInfo* pMessageInfo, AgtTimeout usTimeout) JEM_noexcept;



JEM_api void          JEM_stdcall agtReturn(AgtMessage message, AgtStatus status) JEM_noexcept;




JEM_api AgtStatus     JEM_stdcall agtDispatchMessage(const AgtActor* pActor, const AgtMessageInfo* pMessageInfo) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtExecuteOnThread(AgtThread thread, ) JEM_noexcept;





JEM_api AgtStatus     JEM_stdcall agtGetSenderHandle(AgtMessage message, AgtHandle* pSenderHandle) JEM_noexcept;




/* ========================= [ Async ] ========================= */

JEM_api AgtStatus     JEM_stdcall agtNewAsync(AgtContext ctx, AgtAsync* pAsync) JEM_noexcept;
JEM_api void          JEM_stdcall agtCopyAsync(AgtAsync from, AgtAsync to) JEM_noexcept;
JEM_api void          JEM_stdcall agtClearAsync(AgtAsync async) JEM_noexcept;
JEM_api void          JEM_stdcall agtDestroyAsync(AgtAsync async) JEM_noexcept;

JEM_api AgtStatus     JEM_stdcall agtWait(AgtAsync async, AgtTimeout timeout) JEM_noexcept;
JEM_api AgtStatus     JEM_stdcall agtWaitMany(const AgtAsync* pAsyncs, AgtSize asyncCount, AgtTimeout timeout) JEM_noexcept;


/* ========================= [ Signal ] ========================= */

JEM_api AgtStatus     JEM_stdcall agtNewSignal(AgtContext ctx, AgtSignal* pSignal) JEM_noexcept;
JEM_api void          JEM_stdcall agtAttachSignal(AgtSignal signal, AgtAsync async) JEM_noexcept;
JEM_api void          JEM_stdcall agtRaiseSignal(AgtSignal signal) JEM_noexcept;
JEM_api void          JEM_stdcall agtRaiseManySignals(const AgtSignal* pSignals, AgtSize signalCount) JEM_noexcept;
JEM_api void          JEM_stdcall agtDestroySignal(AgtSignal signal) JEM_noexcept;



// JEM_api AgtStatus     JEM_stdcall agtGetAsync








JEM_end_c_namespace

#endif//JEMSYS_AGATE2_H
