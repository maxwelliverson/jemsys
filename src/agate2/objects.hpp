//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "fwd.hpp"

#include "context.hpp"
#include "ipc/offset_ptr.hpp"
#include "atomic.hpp"
#include "vtable.hpp"
#include "flags.hpp"

namespace Agt {

  enum class ObjectType : AgtUInt32 {
    mpscChannel,
    mpmcChannel,
    spmcChannel,
    spscChannel,
    agent,
    socket,
    channelSender,
    channelReceiver,
    thread,
    agency,
    localAsyncData,
    sharedAsyncData,
    privateChannel
  };

  enum class ConnectAction : AgtUInt32 {
    connectSender,
    disconnectSender,
    connectReceiver,
    disconnectReceiver
  };

  JEM_forceinline static AgtHandleType toHandleType(ObjectType type) noexcept;

  AGT_BITFLAG_ENUM(ObjectFlags, AgtHandleFlags) {
    isShared                  = 0x01,
    isBuiltinType             = 0x02,
    isSender                  = 0x04,
    isReceiver                = 0x08,
    isUnderlyingType          = 0x10,
    supportsOutOfLineMsg      = 0x10000,
    supportsMultiFrameMsg     = 0x20000,
    supportsExternalMsgMemory = 0x40000
  };

  constexpr static auto getHandleFlagMask() noexcept {
    if constexpr (AGT_HANDLE_FLAGS_MAX > 0x1000'0001) {
      // 64 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0000'0000'0001ULL)
        return static_cast<AgtUInt64>(-1);
      return AgtUInt64 ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1ULL;
    }
    else {
      // 32 bit path
      if (AGT_HANDLE_FLAGS_MAX == 0x1000'0001)
        return static_cast<AgtUInt32>(-1);
      return AgtUInt32 ((AGT_HANDLE_FLAGS_MAX - 1) << 1) - 1;
    }
  }

  JEM_forceinline static AgtHandleFlags toHandleFlags(ObjectFlags flags) noexcept {
    constexpr static auto Mask = getHandleFlagMask();
    return static_cast<AgtHandleFlags>(flags) & Mask;
  }



  class Id final {
  public:

    inline constexpr static AgtUInt64 EpochBits = 10;
    inline constexpr static AgtUInt64 ProcessIdBits = 22;
    inline constexpr static AgtUInt64 PageIdBits = 16;
    inline constexpr static AgtUInt64 PageOffsetBits = 15;

    JEM_forceinline jem_u16_t getEpoch() const noexcept {
      return epoch;
    }
    JEM_forceinline AgtUInt32 getProcessId() const noexcept {
      return processId;
    }
    JEM_forceinline bool      isShared() const noexcept {
      return shared;
    }
    JEM_forceinline bool      isExportId() const noexcept {
      return processId == 0ULL;
    }
    JEM_forceinline AgtUInt32 getPageId() const noexcept {
      return pageId;
    }
    JEM_forceinline AgtUInt32 getPageOffset() const noexcept {
      return pageOffset;
    }

    JEM_forceinline AgtObjectId toRaw() const noexcept {
      return bits;
    }


    JEM_forceinline static Id makePrivate(AgtUInt32 epoch, AgtUInt32 procId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.epoch      = epoch;
      id.processId  = procId;
      id.shared     = 0;
      id.pageId     = pageId;
      id.pageOffset = pageOffset;
      return id;
    }
    JEM_forceinline static Id makeShared(AgtUInt32 epoch, AgtUInt32 procId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.epoch      = epoch;
      id.processId  = procId;
      id.shared     = 1;
      id.pageId     = pageId;
      id.pageOffset = pageOffset;
      return id;
    }
    JEM_forceinline static Id makeExport(AgtUInt32 epoch, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      return makeShared(epoch, 0, pageId, pageOffset);
    }

    JEM_forceinline static Id convert(AgtObjectId id) noexcept {
      Id realId;
      realId.bits = id;
      return realId;
    }

    JEM_forceinline static Id invalid() noexcept {
      return convert(AGT_INVALID_OBJECT_ID);
    }


    JEM_forceinline friend bool operator==(const Id& a, const Id& b) noexcept {
      return a.bits == b.bits;
    }

  private:

    union {
      struct {
        AgtUInt64 epoch      : EpochBits;
        AgtUInt64 processId  : ProcessIdBits;
        AgtUInt64 shared     : 1;
        AgtUInt64 pageId     : PageIdBits;
        AgtUInt64 pageOffset : PageOffsetBits;
      };
      AgtUInt64 bits;
    };

  };


  class Handle {

    ObjectType  type;
    ObjectFlags flags;
    AgtContext  context;
    Id          localId;

  protected:

    Handle(ObjectType type, ObjectFlags flags, AgtContext context, Id localId) noexcept
        : type(type),
          flags(flags),
          context(context),
          localId(localId)
    {}



  public:

    Handle(const Handle&) = delete;



    JEM_nodiscard JEM_forceinline ObjectType  getType() const noexcept {
      return type;
    }
    JEM_nodiscard JEM_forceinline ObjectFlags getFlags() const noexcept {
      return flags;
    }
    JEM_nodiscard JEM_forceinline AgtContext  getContext() const noexcept {
      return context;
    }
    JEM_nodiscard JEM_forceinline Id          getId() const noexcept {
      return localId;
    }

    JEM_nodiscard JEM_forceinline bool        isShared() const noexcept {
      return static_cast<bool>(getFlags() & ObjectFlags::isShared);
    }


    AgtStatus               duplicate(Handle*& newHandle) noexcept;

    void                    close() noexcept;

    JEM_nodiscard AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    void                    send(AgtMessage message, AgtSendFlags flags) noexcept;
    JEM_nodiscard AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;

    JEM_nodiscard AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept;

  };

  class LocalHandle : public Handle {
    friend class Handle;
  protected:

    LocalHandle(ObjectType type, ObjectFlags flags, AgtContext ctx, Id localId) noexcept
        : Handle(type, flags, ctx, localId){}

    ~LocalHandle() = default;

    virtual AgtStatus localAcquire() noexcept = 0;

    // Releases a single handle reference, and if the reference count has dropped to zero, destroys the object.
    // By delegating responsibility of destruction to this function, a redundant virtual call is saved on the fast path
    virtual void      localRelease() noexcept = 0;

  public:

    virtual AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept = 0;
    virtual void      localSend(AgtMessage message, AgtSendFlags flags) noexcept = 0;
    virtual AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept = 0;

    virtual AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept = 0;

  };

  class SharedObject {

    ObjectType  type;
    ObjectFlags flags;
    Id          exportId;

  protected:

    SharedObject(ObjectType type, ObjectFlags flags, Id exportId) noexcept
        : type(type),
          flags(flags),
          exportId(exportId)
    {}

  public:

    JEM_nodiscard ObjectType  getType() const noexcept {
      return type;
    }
    JEM_nodiscard ObjectFlags getFlags() const noexcept {
      return flags;
    }
    JEM_nodiscard Id          getExportId() const noexcept {
      return exportId;
    }
  };

  class SharedHandle final : public Handle {
    friend class Handle;

    SharedVPtr    const vptr;
    SharedObject* const instance;

    SharedHandle(SharedObject* pInstance, AgtContext context, Id localId) noexcept;




  public:

    JEM_forceinline AgtStatus sharedAcquire() const noexcept {
      return vptr->acquireRef(instance, getContext());
    }
    JEM_forceinline AgtSize   sharedRelease() const noexcept {
      return vptr->releaseRef(instance, getContext());
    }

    JEM_nodiscard JEM_forceinline SharedObject* getInstance() const noexcept {
      return instance;
    }


    JEM_nodiscard JEM_forceinline AgtStatus sharedStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept {
      return vptr->stage(instance, getContext(), pStagedMessage, timeout);
    }
    JEM_forceinline void sharedSend(AgtMessage message, AgtSendFlags flags) noexcept {
      vptr->send(instance, getContext(), message, flags);
    }
    JEM_nodiscard JEM_forceinline AgtStatus sharedReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept {
      return vptr->receive(instance, getContext(), pMessageInfo, timeout);
    }

    JEM_nodiscard JEM_forceinline AgtStatus sharedConnect(Handle* otherHandle, ConnectAction action) noexcept {
      return vptr->connect(instance, getContext(), otherHandle, action);
    }


  };


}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
