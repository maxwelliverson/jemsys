//
// Created by maxwe on 2021-11-22.
//

#ifndef JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
#define JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP

#include "fwd.hpp"

#include "context.hpp"
#include "ipc/offset_ptr.hpp"
#include "atomic.hpp"

namespace Agt {

  enum class ObjectType : AgtUInt32 {
    agent,
    socket,
    mpscChannel,
    mpmcChannel,
    spmcChannel,
    spscChannel,
    privateChannel,
    channelSender,
    channelReceiver,
    thread,
    agency,
    localAsyncData,
    sharedAsyncData
  };

  JEM_forceinline static AgtHandleType toHandleType(ObjectType type) noexcept;

  enum class ObjectFlags : AgtHandleFlags {
    isShared = 0x1
  };

  JEM_forceinline ObjectFlags operator~(ObjectFlags a) noexcept {
    using Int = std::underlying_type_t<ObjectFlags>;
    return static_cast<ObjectFlags>(~static_cast<Int>(a));
  }
  JEM_forceinline ObjectFlags operator|(ObjectFlags a, ObjectFlags b) noexcept {
    using Int = std::underlying_type_t<ObjectFlags>;
    return static_cast<ObjectFlags>(static_cast<Int>(a) | static_cast<Int>(b));
  }
  JEM_forceinline ObjectFlags operator&(ObjectFlags a, ObjectFlags b) noexcept {
    using Int = std::underlying_type_t<ObjectFlags>;
    return static_cast<ObjectFlags>(static_cast<Int>(a) & static_cast<Int>(b));
  }
  JEM_forceinline ObjectFlags operator^(ObjectFlags a, ObjectFlags b) noexcept {
    using Int = std::underlying_type_t<ObjectFlags>;
    return static_cast<ObjectFlags>(static_cast<Int>(a) ^ static_cast<Int>(b));
  }

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

  class Id {
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

  struct HandleHeader {
    ObjectType  type;
    ObjectFlags flags;
    AgtContext  context;
    Id          localId;
  };

  struct ObjectHeader {
    ObjectType     type;
    ObjectFlags    flags;
    void*          reserved;
    Id             id;
    ReferenceCount refCount;
  };

  struct LocalObject {
    ObjectType     type;
    ObjectFlags    flags;
    AgtContext     context;
    Id             id;
    ReferenceCount refCount;
    LocalVPtr      vtable;
  };

  struct SharedObject {
    ObjectType     type;
    ObjectFlags    flags;
    void*          pReserved;
    Id             exportId;
    ReferenceCount refCount;
  };

}

#endif//JEMSYS_AGATE2_INTERNAL_OBJECTS_HPP
