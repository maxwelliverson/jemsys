//
// Created by maxwe on 2021-11-03.
//

// #include "internal.hpp"
#include "channel.hpp"
#include "message.hpp"
#include "async.hpp"

using namespace Agt;

namespace Agt {
  struct JEM_cache_aligned PrivateChannelMessage {
    PrivateChannelMessage* next;
    PrivateChannel*        owner;
    AgtHandle              returnHandle;
    AgtAsyncData           asyncData;
    AgtUInt32              asyncDataKey;
    MessageFlags           flags;
    MessageState           state;
    AgtUInt32              refCount;
    AgtMessageId           id;
    AgtSize                payloadSize;
    InlineBuffer           inlineBuffer[];
  };
  struct JEM_cache_aligned LocalChannelMessage {
    LocalChannelMessage*      next;
    LocalChannel*             owner;
    AgtHandle                 returnHandle;
    AgtAsyncData              asyncData;
    AgtUInt32                 asyncDataKey;
    MessageFlags              flags;
    AtomicFlags<MessageState> state;
    ReferenceCount            refCount;
    AgtMessageId              id;
    AgtSize                   payloadSize;
    InlineBuffer              inlineBuffer[];
  };
  struct JEM_cache_aligned SharedChannelMessage {
    AgtSize                   nextIndex;
    AgtSize                   thisIndex;
    AgtObjectId               returnHandleId;
    AgtSize                   asyncDataOffset;
    AgtUInt32                 asyncDataKey;
    MessageFlags              flags;
    AtomicFlags<MessageState> state;
    ReferenceCount            refCount;
    AgtMessageId              id;
    AgtSize                   payloadSize;
    InlineBuffer              inlineBuffer[];
  };
}

namespace Agt::Impl {

  AgtMessage JEM_stdcall sharedSpScChannelStage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall sharedSpScChannelSend(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
  AgtMessage JEM_stdcall sharedSpScChannelRead(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
  AgtStatus  JEM_stdcall sharedSpScChannelConnect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall sharedSpScChannelMessage(channel* channel, AgtMessage message) JEM_noexcept;

  AgtMessage JEM_stdcall shared_mpsc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_mpsc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
  AgtMessage JEM_stdcall shared_mpsc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
  AgtStatus  JEM_stdcall shared_mpsc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_mpsc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

  AgtMessage JEM_stdcall shared_spmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_spmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
  AgtMessage JEM_stdcall shared_spmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
  AgtStatus  JEM_stdcall shared_spmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_spmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

  AgtMessage JEM_stdcall shared_mpmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_mpmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
  AgtMessage JEM_stdcall shared_mpmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
  AgtStatus  JEM_stdcall shared_mpmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
  void       JEM_stdcall shared_mpmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

}

namespace {

  inline void* getPayload(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    if ((message->flags & MessageFlags::isOutOfLine) == FlagsNotSet) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & MessageFlags::isMultiFrame) == FlagsNotSet) {
        void* resultData;
        std::memcpy(&resultData, message->inlineBuffer, sizeof(void*));
        return resultData;
      }
      return nullptr;
    }
  }

  void zeroMessageData(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    if (void* payload = getPayload(message)) {
      std::memset(payload, 0, message->payloadSize);
    } else {
      AgtMultiFrameMessageInfo mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        AgtMessageFrame frame;
        while(getNextFrame(frame, mfi))
          std::memset(frame.data, 0, frame.size);
      }
    }
  }

  JEM_noinline void doSlowCleanup(void* message_) noexcept {
    auto message = static_cast<PrivateChannelMessage*>(message_);
    zeroMessageData(message);
    message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }

  inline void* getPayload(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    if ((message->flags & MessageFlags::isOutOfLine) == FlagsNotSet) [[likely]]
      return message->inlineBuffer;
    else {
      if ((message->flags & MessageFlags::isMultiFrame) == FlagsNotSet) {
        void* resultData;
        std::memcpy(&resultData, message->inlineBuffer, sizeof(void*));
        return resultData;
      }
      return nullptr;
    }
  }

  void zeroMessageData(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    if (void* payload = getPayload(message)) {
      std::memset(payload, 0, message->payloadSize);
    } else {
      AgtMultiFrameMessageInfo mfi;
      auto status = getMultiFrameMessage(message->inlineBuffer, mfi);
      if (status == AGT_SUCCESS) {
        AgtMessageFrame frame;
        while(getNextFrame(frame, mfi))
          std::memset(frame.data, 0, frame.size);
      }
    }
  }

  JEM_noinline void doSlowCleanup(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    zeroMessageData(message);
    // message->returnHandle = nullptr;
    message->id           = 0;
    message->payloadSize  = 0;
  }





  inline void cleanupMessage(AgtContext ctx, PrivateChannelMessage* message) noexcept {
    message->state    = DefaultMessageState;
    message->refCount = 1;
    message->flags    = FlagsNotSet;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & MessageFlags::shouldDoFastCleanup) == FlagsNotSet)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }
  inline void cleanupMessage(AgtContext ctx, LocalChannelMessage* message) noexcept {
    message->state    = DefaultMessageState;
    message->refCount = ReferenceCount(1);
    message->flags    = FlagsNotSet;
    if (message->asyncData != nullptr)
      asyncDataDrop(message->asyncData, ctx, message->asyncDataKey);
    if ((message->flags & MessageFlags::shouldDoFastCleanup) == FlagsNotSet)
      doSlowCleanup(message);
    //TODO: Release message resources if message is not inline
  }
  inline void cleanupMessage(AgtContext ctx, SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
  }

  inline void destroyMessage(PrivateChannel* channel, PrivateChannelMessage* message) noexcept {
    cleanupMessage(channel->getContext(), message);
    channel->releaseMessage((AgtMessage)message);
  }
  template <std::derived_from<LocalChannel> Channel>
  inline void destroyMessage(Channel* channel, LocalChannelMessage* message) noexcept {
    cleanupMessage(channel->getContext(), message);
    channel->releaseMessage((AgtMessage)message);
  }
  inline void destroyMessage(SharedChannel* channel, SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
  }

  inline void placeHold(PrivateChannelMessage* message) noexcept {
    message->state = message->state | MessageState::isOnHold;
  }
  inline void placeHold(LocalChannelMessage* message) noexcept {
    message->state.set(MessageState::isOnHold);
  }
  inline void placeHold(SharedChannelMessage* message) noexcept {
    // TODO: Implement Shared
    // message->state = message->state | MessageState::isOnHold;
    message->state.set(MessageState::isOnHold);
  }

  inline void releaseHold(PrivateChannel* channel, PrivateChannelMessage* message) noexcept {
    MessageState oldState = message->state;
    message->state = message->state & ~MessageState::isOnHold;
    if ((oldState & MessageState::isCondemned) != FlagsNotSet)
      destroyMessage(channel, message);
  }
  template <typename Channel, typename Msg>
  inline void releaseHold(Channel* channel, Msg* message) noexcept {
    if ( (message->state.fetchAndReset(MessageState::isOnHold) & MessageState::isCondemned) != FlagsNotSet )
      destroyMessage(channel, message);
  }

  inline void condemn(PrivateChannel* channel, PrivateChannelMessage* message) noexcept {
    MessageState oldState = message->state;
    message->state = message->state | MessageState::isCondemned;
    if ((oldState & MessageState::isOnHold) == FlagsNotSet)
      destroyMessage(channel, message);
  }
  template <typename Channel, typename Msg>
  inline void condemn(Channel* channel, Msg* message) noexcept {
    if ( (message->state.fetchAndSet(MessageState::isCondemned) & MessageState::isOnHold) == FlagsNotSet )
      destroyMessage(channel, message);
  }
}



void* PrivateChannel::acquireSlot(AgtTimeout timeout) noexcept {
  --availableSlotCount;
  auto acquiredMsg = nextFreeSlot;
  nextFreeSlot = acquiredMsg->next;
  return acquiredMsg;
}
AgtStatus PrivateChannel::stageOutOfLine(StagedMessage& pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_NOT_READY;
}

AgtStatus PrivateChannel::localStage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( availableSlotCount == 0 ) [[unlikely]]
    return AGT_ERROR_MAILBOX_IS_FULL;
  if ( stagedMessage.messageSize > slotSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }

  auto acquiredMsg = static_cast<PrivateChannelMessage*>(acquireSlot(timeout));

  stagedMessage.receiver = this;
  stagedMessage.message  = acquiredMsg;
  stagedMessage.payload  = acquiredMsg->inlineBuffer;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = slotSize;

  return AGT_SUCCESS;
}
void      PrivateChannel::localSend(AgtMessage message, AgtSendFlags flags) noexcept {}
AgtStatus PrivateChannel::localReceive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus PrivateChannel::localConnect(Handle* otherHandle, ConnectAction action) noexcept {}

void      PrivateChannel::releaseMessage(AgtMessage message) noexcept {
}


AgtStatus LocalSpMcChannel::localStage(AgtStagedMessage& stagedMessage, AgtTimeout timeout) noexcept {}
void      LocalSpMcChannel::localSend(AgtMessage message, AgtSendFlags flags) noexcept {}
AgtStatus LocalSpMcChannel::localReceive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalSpMcChannel::localConnect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalSpScChannel::localStage(AgtStagedMessage& stagedMessage, AgtTimeout timeout) noexcept {}
void      LocalSpScChannel::localSend(AgtMessage message, AgtSendFlags flags) noexcept {}
AgtStatus LocalSpScChannel::localReceive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalSpScChannel::localConnect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalMpMcChannel::localStage(AgtStagedMessage& stagedMessage, AgtTimeout timeout) noexcept {}
void      LocalMpMcChannel::localSend(AgtMessage message, AgtSendFlags flags) noexcept {}
AgtStatus LocalMpMcChannel::localReceive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalMpMcChannel::localConnect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalMpScChannel::localStage(AgtStagedMessage& stagedMessage, AgtTimeout timeout) noexcept {}
void      LocalMpScChannel::localSend(AgtMessage message, AgtSendFlags flags) noexcept {}
AgtStatus LocalMpScChannel::localReceive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalMpScChannel::localConnect(Handle* otherHandle, ConnectAction action) noexcept {}


