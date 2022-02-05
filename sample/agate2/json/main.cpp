//
// Created by Maxwell on 2022-01-25.
//

#include <agate2.h>

#include <cstdio>
#include <cstdlib>


#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined (WIN64)
#define AGT_function_name __FUNCSIG__
#else
#define AGT_function_name __PRETTY_FUNCTION__
#endif
#define AGT_safe_call(...) if (AgtStatus _sts_X = (__VA_ARGS__); _sts_X != AGT_SUCCESS) catchError(_sts_X, __FILE__, AGT_function_name, __LINE__)


void catchError(AgtStatus status, const char* file, const char* func, int line) noexcept {
  const char* errorString;

  switch(status) {
    case AGT_SUCCESS:
      errorString = "[AGT_SUCCESS]: No error";
      break;
    case AGT_NOT_READY:
      errorString = "[AGT_NOT_READY]: Not yet ready";
      break;
    case AGT_DEFERRED:
      errorString = "[AGT_DEFERRED]: Operation has been deferred";
      break;
    case AGT_CANCELLED:
      errorString = "[AGT_CANCELLED]: Operation has been cancelled";
      break;
    case AGT_TIMED_OUT:
      errorString = "[AGT_TIMED_OUT]: Wait operation has timed out";
      break;
    case AGT_INCOMPLETE_MESSAGE:
      errorString = "[AGT_INCOMPLETE_MESSAGE]: Message has more frames available";
      break;
    case AGT_ERROR_UNKNOWN:
      errorString = "[AGT_ERROR_UNKNOWN]: Unknown error";
      break;
    case AGT_ERROR_UNKNOWN_FOREIGN_OBJECT:
      errorString = "[AGT_ERROR_UNKNOWN_FOREIGN_OBJECT]: Could not retrieve handle to foreign object";
      break;
    case AGT_ERROR_INVALID_OBJECT_ID:
      errorString = "[AGT_ERROR_INVALID_OBJECT_ID]: No object exists with the specified object ID";
      break;
    case AGT_ERROR_EXPIRED_OBJECT_ID:
      errorString = "[AGT_ERROR_EXPIRED_OBJECT_ID]: The object referred to by the specified ID no longer exists";
      break;
    case AGT_ERROR_NULL_HANDLE:
      errorString = "[AGT_ERROR_NULL_HANDLE]: Handle cannot be null";
      break;
    case AGT_ERROR_INVALID_HANDLE_TYPE:
      errorString = "[AGT_ERROR_INVALID_HANDLE_TYPE]: Handle type not recognized (Internal Error)";
      break;
    case AGT_ERROR_NOT_BOUND:
      errorString = "[AGT_ERROR_NOT_BOUND]: Proxy handle not bound to endpoint";
      break;
    case AGT_ERROR_FOREIGN_SENDER:
      errorString = "[AGT_ERROR_FOREIGN_SENDER]: Sender could not be retrieved, as it not shared and is foreign to this process";
      break;
    case AGT_ERROR_STATUS_NOT_SET:
      errorString = "[AGT_ERROR_STATUS_NOT_SET]: Status not set (Debug, Internal)";
      break;
    case AGT_ERROR_UNKNOWN_MESSAGE_TYPE:
      errorString = "[AGT_ERROR_UNKNOWN_MESSAGE_TYPE]: Unknown message ID";
      break;
    case AGT_ERROR_INVALID_FLAGS:
      errorString = "[AGT_ERROR_INVALID_FLAGS]: Invalid flags specified";
      break;
    case AGT_ERROR_INVALID_MESSAGE:
      errorString = "[AGT_ERROR_INVALID_MESSAGE]: Invalid message handle";
      break;
    case AGT_ERROR_INVALID_SIGNAL:
      errorString = "[AGT_ERROR_INVALID_SIGNAL]: Invalid signal handle";
      break;
    case AGT_ERROR_CANNOT_DISCARD:
      errorString = "[AGT_ERROR_CANNOT_DISCARD]: Cannot discard this message";
      break;
    case AGT_ERROR_MESSAGE_TOO_LARGE:
      errorString = "[AGT_ERROR_MESSAGE_TOO_LARGE]: Requested message too large for this handle";
      break;
    case AGT_ERROR_INSUFFICIENT_SLOTS:
      errorString = "[AGT_ERROR_INSUFFICIENT_SLOTS]: Channel is full";
      break;
    case AGT_ERROR_NOT_MULTIFRAME:
      errorString = "[AGT_ERROR_NOT_MULTIFRAME]: Message is not multi-frame";
      break;
    case AGT_ERROR_BAD_SIZE:
      errorString = "[AGT_ERROR_BAD_SIZE]: Bad size";
      break;
    case AGT_ERROR_INVALID_ARGUMENT:
      errorString = "[AGT_ERROR_INVALID_ARGUMENT]: Invalid argument";
      break;
    case AGT_ERROR_BAD_ENCODING_IN_NAME:
      errorString = "[AGT_ERROR_BAD_ENCODING_IN_NAME]: Requested name is not properly encoded. Check the value of AGATE_CTX_ENCODING and ensure it matches the encoding your program uses";
      break;
    case AGT_ERROR_NAME_TOO_LONG:
      errorString = "[AGT_ERROR_NAME_TOO_LONG]: Requested name must not exceed N characters";
      break;
    case AGT_ERROR_NAME_ALREADY_IN_USE:
      errorString = "[AGT_ERROR_NAME_ALREADY_IN_USE]: Requested name is already in use";
      break;
    case AGT_ERROR_BAD_ALLOC:
      errorString = "[AGT_ERROR_BAD_ALLOC]: Internal allocation failure";
      break;
    case AGT_ERROR_MAILBOX_IS_FULL:
      errorString = "[AGT_ERROR_MAILBOX_IS_FULL]: Mailbox is full";
      break;
    case AGT_ERROR_MAILBOX_IS_EMPTY:
      errorString = "[AGT_ERROR_MAILBOX_IS_EMPTY]: No messages available";
      break;
    case AGT_ERROR_TOO_MANY_SENDERS:
      errorString = "[AGT_ERROR_TOO_MANY_SENDERS]: Handle has already registered the maximum number of senders";
      break;
    case AGT_ERROR_TOO_MANY_RECEIVERS:
      errorString = "[AGT_ERROR_TOO_MANY_RECEIVERS]: Handle has already registered the maximum number of receivers";
      break;
    case AGT_ERROR_ALREADY_RECEIVED:
      errorString = "[AGT_ERROR_ALREADY_RECEIVED]: Message was already received";
      break;
    case AGT_ERROR_INITIALIZATION_FAILED:
      errorString = "[AGT_ERROR_INITIALIZATION_FAILED]: Context was not properly initialized";
      break;
    case AGT_ERROR_NOT_YET_IMPLEMENTED:
      errorString = "[AGT_ERROR_NOT_YET_IMPLEMENTED]: Function has not yet been implemented";
      break;
    default:
      errorString = "[???] Invalid error code";
  }

  fprintf(stderr, "%s:%d in function %s: %s\n\n", file, line, func, errorString);
  abort();
}



class JsonAgent {


  static PFN_agtActorMessageProc pProc;
  static PFN_agtActorDtor        pDtor;

  static AgtStatus proc(void* state, const AgtMessageInfo* pMessageInfo) {

  }
  static void dtor(void* state) {

  }

public:



};






int main() {

  AgtContext ctx;
  AGT_safe_call(agtNewContext(&ctx));

  AgtChannelCreateInfo channelCreateInfo;
  AgtHandle sender, receiver;

  channelCreateInfo.scope = AGT_SCOPE_LOCAL;
  channelCreateInfo.name = "agate2.json server";
  channelCreateInfo.maxReceivers = 1;
  channelCreateInfo.maxSenders = 0;
  channelCreateInfo.minCapacity = 2000;
  channelCreateInfo.maxMessageSize = 448;
  channelCreateInfo.asyncHandle = AGT_SYNCHRONIZE;

  AGT_safe_call(agtCreateChannel(ctx, &channelCreateInfo, &sender, &receiver));






  agtCloseHandle(sender);
  agtCloseHandle(receiver);

  AGT_safe_call(agtDestroyContext(ctx));
}