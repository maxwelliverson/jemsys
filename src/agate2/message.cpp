//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"
#include "flags.hpp"
#include "atomic.hpp"
#include "channel.hpp"

#include "ipc/offset_ptr.hpp"


using namespace Agt;

namespace {

}

extern "C" {



struct AgtSharedMessageChainLink_st {
  ipc::offset_ptr<AgtSharedMessageChainLink_st> next;
  ipc::offset_ptr<Agt::SharedChannel>           channel;
};

struct AgtSharedMessageChain_st {
  ipc::offset_ptr<AgtSharedMessageChainLink_st> firstLink;
  AgtUInt32 chainLength;
};



struct JEM_cache_aligned AgtQueuedMessage_st {
  union {
    jem_size_t       index;
    AgtQueuedMessage address;
  } next;
};

struct JEM_cache_aligned AgtSharedMessage_st {
  AgtSize             nextIndex;
  AgtObjectId         returnHandleId;
  AgtSize             asyncDataOffset;
  AgtUInt32           asyncDataKey;
  AgtSize             payloadSize;
  AgtMessageId        id;
JEM_cache_aligned
  char                inlineBuffer[];
};
struct JEM_cache_aligned AgtLocalMessage_st {
  AgtLocalMessage_st* next;
  AgtHandle           returnHandle;
  AgtAsyncData        asyncData;
  AgtUInt32           asyncDataKey;
  AgtSize             payloadSize;
  AgtMessageId        id;
JEM_cache_aligned
  char                inlineBuffer[];
};

}


bool  Agt::initMessageArray(LocalChannelHeader* owner) noexcept {
  const AgtSize slotCount = owner->slotCount;
  const AgtSize messageSize = owner->inlineBufferSize + sizeof(AgtMessage_st);
  auto messageSlots = (std::byte*)ctxAllocLocal(owner->context, slotCount * messageSize, JEM_CACHE_LINE);
  if (messageSlots) [[likely]] {
    for ( AgtSize i = 0; i < slotCount; ++i ) {
      auto address = messageSlots + (messageSize * i);
      auto message = new(address) AgtMessage_st;
      message->next = (AgtMessage)(address + messageSize);
      message->owner = owner;
      message->refCount = 1;
    }

    owner->messageSlots = messageSlots;

    return true;
  }

  return false;
}

void* Agt::initMessageArray(SharedHandleHeader* owner, SharedChannelHeader* channel) noexcept {
  AgtSize slotCount   = channel->slotCount;
  AgtSize messageSize = channel->inlineBufferSize + sizeof(AgtMessage_st);
  AgtSize arraySize = slotCount * messageSize;
  auto messageSlots = (std::byte*)ctxSharedAlloc(owner->context,
                                                 arraySize,
                                                 JEM_CACHE_LINE,
                                                 channel->messageSlotsId);

  if (messageSlots) [[likely]] {

    for ( AgtSize offset = 0; offset < arraySize;) {
      auto message = new(messageSlots + offset) AgtMessage_st;
      message->nextOffset = (offset += messageSize);
      message->owner = nullptr; // TODO: Fix, this obviously doesn't work for shared channels
      message->refCount = 1;
      message->flags = MessageFlags::isShared;
    }

  }

  return messageSlots;

}