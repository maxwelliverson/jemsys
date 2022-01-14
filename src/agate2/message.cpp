//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"
#include "flags.hpp"
#include "atomic.hpp"

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

struct JEM_cache_aligned AgtMessage_st {
  union {
    AgtSize     index;
    AgtMessage  address;
  }   next;
  union {
    AgtHandle   handle;
    AgtObjectId id;
  }   sender;
  AgtSize                   payloadSize;
  AgtAsyncData              asyncData;
  AgtUInt32                 asyncDataKey;
  MessageFlags              flags;
  AtomicFlags<MessageState> state;
  ReferenceCount            refCount;
  AgtObjectId               returnHandleId;
  AgtMessageId              id;
JEM_cache_aligned
  char                      inlineBuffer[];
};

}
