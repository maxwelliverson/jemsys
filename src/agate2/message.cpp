//
// Created by maxwe on 2021-11-28.
//

#include "message.hpp"

#include "ipc/offset_ptr.hpp"
#include "support/atomicutils.hpp"


extern "C" {



struct AgtSharedMessageChainLink_st {
  ipc::offset_ptr<AgtSharedMessageChainLink_st> next;
  ipc::offset_ptr<Agt::SharedChannel>           channel;
};

struct AgtSharedMessageChain_st {
  ipc::offset_ptr<AgtSharedMessageChainLink_st> firstLink;
  AgtUInt32 chainLength;
};


struct AgtSharedMessage_st {
  atomic_u32_t refCount;

};

struct JEM_cache_aligned AgtQueuedMessage_st {
  union {
    jem_size_t       index;
    AgtQueuedMessage address;
  } next;
};

struct JEM_cache_aligned AgtMessage_st {
  union {
    jem_size_t index;
    AgtMessage address;
  }      next;
  union {
    jem_size_t    offset;
    agt::channel* address;
  }      channel;
  jem_size_t       thisIndex;
  AgtSize          payloadSize;
  AgtAsyncData_st  asyncData;
  atomic_flags32_t flags;
  AgtAgent         sender;
  jem_u64_t        id;
  JEM_cache_aligned
    char             payload[];
};

}
