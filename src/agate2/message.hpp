//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_MESSAGE_HPP
#define JEMSYS_AGATE2_MESSAGE_HPP

#include "fwd.hpp"
#include "flags.hpp"

namespace Agt {

  struct JEM_cache_aligned InlineBuffer {};

  struct StagedMessage {
    Handle*      receiver;
    void*        message;
    void*        reserved[2];
    Handle*      returnHandle;
    AgtSize      messageSize;
    AgtMessageId id;
    void*        payload;
  };

  AGT_BITFLAG_ENUM(MessageFlags, AgtUInt32) {
    isShared            = 0x01,
    isOutOfLine         = 0x02,
    isMultiFrame        = 0x04,
    shouldDoFastCleanup = 0x08,
    externalMemory      = 0x10,
    externalOwnership   = 0x20
  };

  AGT_BITFLAG_ENUM(MessageState, AgtUInt32) {
    isQueued    = 0x1,
    isOnHold    = 0x2,
    isCondemned = 0x4
  };

  inline constexpr static MessageState DefaultMessageState = {};

  AgtStatus initMessageArray(Handle* handle, AgtMessage_st* messages, AgtSize messageCount, AgtSize inlineBufferSize) noexcept;

  AgtStatus getMultiFrameMessage(InlineBuffer* inlineBuffer, AgtMultiFrameMessageInfo& messageInfo) noexcept;
  bool      getNextFrame(AgtMessageFrame& frame, AgtMultiFrameMessageInfo& messageInfo) noexcept;


  void initMessage(AgtMessage message) noexcept;

  void setMessageId(AgtMessage message, AgtMessageId id) noexcept;
  void setMessageReturnHandle(AgtMessage message, Handle* returnHandle) noexcept;
  void setMessageAsyncHandle(AgtMessage message, AgtAsync async) noexcept;

  void cleanupMessage(AgtMessage message) noexcept;

}

#endif//JEMSYS_AGATE2_MESSAGE_HPP
