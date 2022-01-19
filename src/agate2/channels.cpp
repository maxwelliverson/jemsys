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


#define AGT_acquire_semaphore(sem, timeout, err) do { \
  switch (timeout) {                                          \
    case JEM_WAIT:                                    \
      (sem).acquire();                                  \
      break;                                          \
    case JEM_DO_NOT_WAIT:                             \
      if (!(sem).try_acquire())                         \
        return err;                                   \
      break;                                          \
    default:                                          \
      if (!(sem).try_acquire_for(timeout))              \
        return err;\
  }                                                    \
  } while(false)

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


  inline agt_cookie_t get_queued_message(local_spmc_mailbox* mailbox) noexcept {
    agt_cookie_t previousReceivedMessage = mailbox->previousReceivedMessage.load();
    agt_cookie_t message;
    do {
      message = previousReceivedMessage->next.address;
    } while( !mailbox->previousReceivedMessage.compare_exchange_weak(previousReceivedMessage, message));
    free_hold(mailbox, previousReceivedMessage);
    return message;
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



LocalChannel::LocalChannel(AgtStatus& status, ObjectType type, AgtContext ctx, AgtObjectId id, AgtSize slotCount, AgtSize slotSize) noexcept
    : LocalHandle(type, FlagsNotSet, ctx, id),
      slotCount(slotCount),
      inlineBufferSize(slotSize),
      messageSlots(nullptr){

  AgtSize messageSize = slotSize + sizeof(LocalChannelMessage);
  void* localChannelMessages;
  localChannelMessages = ctxAllocLocal(ctx, slotCount * messageSize, JEM_CACHE_LINE);
  if (!localChannelMessages) {
    status = AGT_ERROR_BAD_ALLOC;
    return;
  }
  messageSlots = static_cast<std::byte*>(localChannelMessages);
  for ( AgtSize i = 0; i < slotCount; ++i ) {
    auto address = messageSlots + (messageSize * i);
    auto message = new(address) LocalChannelMessage;
    message->next = (LocalChannelMessage*)(address + messageSize);
    message->owner = this;
    message->refCount = ReferenceCount(1);
  }
  status = AGT_SUCCESS;
}

PrivateChannel::PrivateChannel(AgtStatus& status, AgtContext ctx, AgtObjectId id, AgtSize slotCount, AgtSize slotSize) noexcept
    : LocalChannel(status, ObjectType::privateChannel, ctx, id, slotCount, slotSize){

}


AgtStatus PrivateChannel::createInstance(PrivateChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, PrivateChannelSender* sender, PrivateChannelReceiver* receiver) noexcept {

}




void*     PrivateChannel::acquireSlot(AgtTimeout timeout) noexcept {
  --availableSlotCount;
  auto acquiredMsg = nextFreeSlot;
  nextFreeSlot = acquiredMsg->next;
  return acquiredMsg;
}
AgtStatus PrivateChannel::stageOutOfLine(StagedMessage& pStagedMessage, AgtTimeout timeout) noexcept {
  return AGT_NOT_READY;
}

AgtStatus PrivateChannel::acquire() noexcept {}
void      PrivateChannel::release() noexcept {}

AgtStatus PrivateChannel::stage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( availableSlotCount == 0 ) [[unlikely]]
    return AGT_ERROR_MAILBOX_IS_FULL;
  if ( stagedMessage.messageSize > inlineBufferSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }

  auto acquiredMsg = static_cast<PrivateChannelMessage*>(acquireSlot(timeout));

  stagedMessage.receiver = this;
  stagedMessage.message  = acquiredMsg;
  stagedMessage.payload  = acquiredMsg->inlineBuffer;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = inlineBufferSize;

  return AGT_SUCCESS;
}
void      PrivateChannel::send(AgtMessage message_, AgtSendFlags flags) noexcept {
  auto message = (PrivateChannelMessage*)message_;
  // set(message->state, MessageState::);
  placeHold(message);
  prevQueuedMessage->next = message;
  prevQueuedMessage = message;
  ++queuedMessageCount;
}
AgtStatus PrivateChannel::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {
  if ( queuedMessageCount == 0 )
    return AGT_ERROR_MAILBOX_IS_EMPTY;
  auto cookie = prevReceivedMessage->next;
  releaseHold(this, prevReceivedMessage);
  prevReceivedMessage = cookie;
  messageInfo.message = (AgtMessage)cookie;
  messageInfo.size    = cookie->payloadSize;
  messageInfo.id      = cookie->id;
  messageInfo.payload = getPayload(cookie);
  return AGT_SUCCESS;
}
AgtStatus PrivateChannel::connect(Handle* otherHandle, ConnectAction action) noexcept {}

void      PrivateChannel::releaseMessage(AgtMessage message_) noexcept {
  auto message = (PrivateChannelMessage*)message_;
  ++availableSlotCount;
  message->next = nextFreeSlot;
  nextFreeSlot = message;
}


AgtStatus LocalSpMcChannel::acquire() noexcept {}
void      LocalSpMcChannel::release() noexcept {}
AgtStatus LocalSpMcChannel::stage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( stagedMessage.messageSize > inlineBufferSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }
  AGT_acquire_semaphore(slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = acquireSlot();
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = this;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = inlineBufferSize;

  return AGT_SUCCESS;
}
void      LocalSpMcChannel::send(AgtMessage message_, AgtSendFlags flags) noexcept {
  // AGT_prep_slot(slot);
  auto message = (LocalChannelMessage*)message_;
  placeHold(message);
  lastQueuedSlot->next = message;
  lastQueuedSlot = message;
  queuedMessages.release();
}
AgtStatus LocalSpMcChannel::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {
  AGT_acquire_semaphore(queuedMessages, timeout, AGT_ERROR_MAILBOX_IS_EMPTY);
  auto prevReceivedMessage = previousReceivedMessage.load();
  LocalChannelMessage* message;
  do {
    message = prevReceivedMessage->next;
  } while( !previousReceivedMessage.compare_exchange_weak(prevReceivedMessage, message));
  releaseHold(this, prevReceivedMessage);
  messageInfo.message = (AgtMessage)message;
  messageInfo.size    = message->payloadSize;
  messageInfo.id      = message->id;
  messageInfo.payload = getPayload(message);
  return AGT_SUCCESS;
}
AgtStatus LocalSpMcChannel::connect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalSpScChannel::acquire() noexcept {}
void      LocalSpScChannel::release() noexcept {}
AgtStatus LocalSpScChannel::stage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( stagedMessage.messageSize > inlineBufferSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }
  AGT_acquire_semaphore(slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = acquireSlot();
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = this;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = inlineBufferSize;

  return AGT_SUCCESS;
}
void      LocalSpScChannel::send(AgtMessage message_, AgtSendFlags flags) noexcept {
  auto message = (LocalChannelMessage*)message_;
  placeHold(message);
  producerPreviousQueuedMsg->next = message;
  producerPreviousQueuedMsg = message;
  queuedMessages.release();
}
AgtStatus LocalSpScChannel::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalSpScChannel::connect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalMpMcChannel::acquire() noexcept {}
void      LocalMpMcChannel::release() noexcept {}
AgtStatus LocalMpMcChannel::stage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( stagedMessage.messageSize > inlineBufferSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }
  AGT_acquire_semaphore(slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = acquireSlot();
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = this;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = inlineBufferSize;

  return AGT_SUCCESS;
}
void      LocalMpMcChannel::send(AgtMessage message_, AgtSendFlags flags) noexcept {
  auto lastQueued = lastQueuedSlot.load(std::memory_order_acquire);
  auto message = (LocalChannelMessage*)message_;
  placeHold(message);
  while ( !lastQueuedSlot.compare_exchange_weak(lastQueued, message) );
  lastQueued->next = message;
  queuedMessages.release();
}
AgtStatus LocalMpMcChannel::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalMpMcChannel::connect(Handle* otherHandle, ConnectAction action) noexcept {}

AgtStatus LocalMpScChannel::acquire() noexcept {}
void      LocalMpScChannel::release() noexcept {}
AgtStatus LocalMpScChannel::stage(AgtStagedMessage& stagedMessage_, AgtTimeout timeout) noexcept {
  auto& stagedMessage = reinterpret_cast<StagedMessage&>(stagedMessage_);
  if ( stagedMessage.messageSize > inlineBufferSize) [[unlikely]] {
    if (testAny(this->getFlags(), ObjectFlags::supportsOutOfLineMsg))
      return stageOutOfLine(stagedMessage, timeout);
    return AGT_ERROR_MESSAGE_TOO_LARGE;
  }
  AGT_acquire_semaphore(slotSemaphore, timeout, AGT_ERROR_MAILBOX_IS_FULL);

  auto message = acquireSlot();
  stagedMessage.message  = message;
  stagedMessage.payload  = message->inlineBuffer;
  stagedMessage.receiver = this;
  if (stagedMessage.messageSize == 0)
    stagedMessage.messageSize = inlineBufferSize;

  return AGT_SUCCESS;
}
void      LocalMpScChannel::send(AgtMessage message_, AgtSendFlags flags) noexcept {
  auto lastQueued = lastQueuedSlot.load(std::memory_order_acquire);
  auto message = (LocalChannelMessage*)message_;
  placeHold(message);
  while ( !lastQueuedSlot.compare_exchange_weak(lastQueued, message) );
  lastQueued->next = message;
  queuedMessageCount.increase(1);
}
AgtStatus LocalMpScChannel::receive(AgtMessageInfo& messageInfo, AgtTimeout timeout) noexcept {}
AgtStatus LocalMpScChannel::connect(Handle* otherHandle, ConnectAction action) noexcept {}


