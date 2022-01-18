//
// Created by maxwe on 2021-11-25.
//

#ifndef JEMSYS_AGATE2_FWD_HPP
#define JEMSYS_AGATE2_FWD_HPP

#include <agate2.h>

extern "C" {

AGT_DEFINE_HANDLE(AgtAsyncData);
AGT_DEFINE_HANDLE(AgtMessageData);
AGT_DEFINE_HANDLE(AgtQueuedMessage);
AGT_DEFINE_HANDLE(AgtMessageChainLink);
AGT_DEFINE_HANDLE(AgtMessageChain);

AGT_DEFINE_HANDLE(AgtSharedMessageChainLink);
AGT_DEFINE_HANDLE(AgtSharedMessageChain);
AGT_DEFINE_HANDLE(AgtSharedMessage);
AGT_DEFINE_HANDLE(AgtLocalMessageChainLink);
AGT_DEFINE_HANDLE(AgtLocalMessageChain);
AGT_DEFINE_HANDLE(AgtLocalMessage);

}

namespace Agt {

  enum class ObjectType : AgtUInt32;
  enum class ObjectFlags : AgtHandleFlags;
  enum class ConnectAction : AgtUInt32;

  class Id;
  class Handle;
  class LocalHandle;
  class SharedHandle;
  class SharedObject;
  using LocalObject = LocalHandle;

  struct LocalChannel;
  struct SharedChannel;

  struct SharedVTable;

  using SharedVPtr = const SharedVTable*;

}

#endif//JEMSYS_AGATE2_FWD_HPP
