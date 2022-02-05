//
// Created by maxwe on 2021-11-25.
//

#include "context.hpp"
#include "objects.hpp"
#include "pool.hpp"
#include "ipc_block.hpp"

#include <cstdlib>
#include <memory>
#include <mutex>

using namespace Agt;

namespace {
  inline constexpr static jem_size_t CellSize      = 32;
  inline constexpr static jem_size_t CellAlignment = 32;

  enum PageFlags : AgtUInt32 {
    page_is_shared = 0x1,
    page_is_active = 0x2
  };
  enum CellFlags : jem_u16_t {
    cell_object_is_private = 0x1,
    cell_object_is_shared  = 0x2,
    cell_is_shared_export  = 0x4,
    cell_is_shared_import  = 0x8
  };

  class Id final {
  public:

    enum ObjectKind {
      eInvalid,
      eObjectInstance,
      eHandle,
      eInternal
    };

    inline constexpr static AgtUInt64 SharedFlagBits = 1;
    inline constexpr static AgtUInt64 KindBits       = 2;
    inline constexpr static AgtUInt64 PageIdBits     = 14;
    inline constexpr static AgtUInt64 PageOffsetBits = 15;
    inline constexpr static AgtUInt64 EpochBits      = 10;
    inline constexpr static AgtUInt64 ProcessIdBits  = 22;
    inline constexpr static AgtUInt64 SegmentIdBits  = ProcessIdBits;

    JEM_forceinline ObjectKind  getKind() const noexcept {
      return static_cast<ObjectKind>(local.kind);
    }
    JEM_forceinline AgtUInt16   getEpoch() const noexcept {
      return local.epoch;
    }
    JEM_forceinline AgtUInt32   getProcessId() const noexcept {
      return local.processId;
    }
    JEM_forceinline AgtUInt32   getSegmentId() const noexcept {
      return shared.segmentId;
    }
    JEM_forceinline bool        isShared() const noexcept {
      return local.isShared;
    }
    JEM_forceinline AgtUInt32   getPageId() const noexcept {
      return local.pageId;
    }
    JEM_forceinline AgtUInt32   getPageOffset() const noexcept {
      return local.pageOffset;
    }

    JEM_forceinline AgtObjectId toRaw() const noexcept {
      return bits;
    }


    JEM_forceinline static Id makeLocal(AgtUInt32 epoch, AgtUInt32 procId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.local.epoch      = epoch;
      id.local.processId  = procId;
      id.local.isShared   = 0;
      id.local.pageId     = pageId;
      id.local.pageOffset = pageOffset;
      return id;
    }
    JEM_forceinline static Id makeShared(AgtUInt32 epoch, AgtUInt32 segmentId, AgtUInt32 pageId, AgtUInt32 pageOffset) noexcept {
      Id id;
      id.shared.epoch      = epoch;
      id.shared.segmentId  = segmentId;
      id.shared.isShared   = 1;
      id.shared.pageId     = pageId;
      id.shared.pageOffset = pageOffset;
      return id;
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
        AgtUInt64 isShared   : SharedFlagBits;
        AgtUInt64 kind       : KindBits;
        AgtUInt64 pageId     : PageIdBits;
        AgtUInt64 pageOffset : PageOffsetBits;
        AgtUInt64 epoch      : EpochBits;
        AgtUInt64 processId  : ProcessIdBits;
      } local;
      struct {
        AgtUInt64 isShared   : SharedFlagBits;
        AgtUInt64 kind       : KindBits;
        AgtUInt64 pageId     : PageIdBits;
        AgtUInt64 pageOffset : PageOffsetBits;
        AgtUInt64 epoch      : EpochBits;
        AgtUInt64 segmentId  : SegmentIdBits;
      } shared;
      AgtUInt64 bits;
    };
  };

  struct alignas(CellAlignment) ObjectInfoCell {
    AgtUInt32     nextFreeCell;
    jem_u16_t     epoch;
    jem_u16_t     flags;

    AgtUInt32     thisIndex;
    AgtUInt32     pageId;
    AgtUInt32     pageOffset;

    AgtHandleType objectType;

    ObjectHeader* object;
  };

  class alignas(JEM_PHYSICAL_PAGE_SIZE) Page {

    simple_mutex_t writeLock;
    atomic_u32_t   refCount;
    jem_flags32_t  flags;
    AgtUInt32      freeCells;
    AgtUInt32      nextFreeCell;
    AgtUInt32      totalCells;
    ObjectInfoCell cells[];

  public:

    Page(jem_size_t pageSize, jem_flags32_t pageFlags = 0) noexcept
        : freeCells((pageSize / CellSize) - 1),
          nextFreeCell(1),
          totalCells(freeCells),
          flags(pageFlags)
    {
      AgtUInt32 index = 0;
      jem_u16_t cellFlags = (pageFlags & page_is_shared) ? (cell_object_is_shared | cell_is_shared_export) : 0;
      while (index < totalCells) {
        auto& c = cell(index);
        c.thisIndex = index;
        c.nextFreeCell = ++index;
        c.epoch = 0;
        c.flags = cellFlags;
      }
    }


    JEM_forceinline ObjectInfoCell& allocCell() noexcept {
      JEM_assert( !full() );
      auto& c = cell(nextFreeCell);
      nextFreeCell = c.nextFreeCell;
      --freeCells;
      return c;
    }
    JEM_forceinline void            freeCell(AgtUInt32 index) noexcept {
      JEM_assert( !empty() );
      auto& c = cell(index);
      c.nextFreeCell = std::exchange(nextFreeCell, index);
      ++c.epoch;
      ++freeCells;
    }

    JEM_forceinline ObjectInfoCell& cell(AgtUInt32 index) const noexcept {
      return const_cast<ObjectInfoCell&>(cells[index]);
    }

    JEM_forceinline bool empty() const noexcept {
      return freeCells == totalCells;
    }
    JEM_forceinline bool full() const noexcept {
      return freeCells == 0;
    }
  };

  struct ListNodeBase {
    ListNodeBase* next;
    ListNodeBase* prev;

    void insertAfter(ListNodeBase* node) noexcept {
      prev = node;
      next = node->next;
      node->prev->next = this;
      node->prev = this;
    }
    void insertBefore(ListNodeBase* node) noexcept {
      prev = node->prev;
      next = node;
      node->next->prev = this;
      node->next = this;
    }
    void moveToAfter(ListNodeBase* node) noexcept {
      remove();
      insertAfter(node);
    }
    void moveToBefore(ListNodeBase* node) noexcept {
      remove();
      insertBefore(node);
    }
    void remove() noexcept {
      next->prev = prev;
      prev->next = next;
    }
  };

  struct ListNode : ListNodeBase {
    Page* page;
  };

  class PageList {

    ListNodeBase base;
    AgtSize      entryCount;

  public:

    PageList() : base{ &base, &base }, entryCount(0){ }


    JEM_forceinline AgtSize   size()  const noexcept {
      return entryCount;
    }
    JEM_forceinline bool      empty() const noexcept {
      return entryCount == 0;
    }

    JEM_forceinline ListNode* front() const noexcept {
      JEM_assert(!empty());
      return static_cast<ListNode*>(base.next);
    }
    JEM_forceinline ListNode* back() const noexcept {
      JEM_assert(!empty());
      return static_cast<ListNode*>(base.prev);
    }

    JEM_forceinline void      pushFront(ListNode* node) noexcept {
      node->insertAfter(&base);
      ++entryCount;
    }
    JEM_forceinline ListNode* popFront() noexcept {
      ListNode* node = front();
      node->remove();
      --entryCount;
      return node;
    }
    JEM_forceinline void      pushBack(ListNode* node) noexcept {
      node->insertBefore(&base);
      ++entryCount;
    }
    JEM_forceinline ListNode* popBack() noexcept {
      ListNode* node = back();
      node->remove();
      --entryCount;
      return node;
    }

    JEM_forceinline void      moveToFront(ListNode* node) noexcept {
      node->moveToAfter(&base);
    }
    JEM_forceinline void      moveToBack(ListNode* node) noexcept {
      node->moveToBefore(&base);
    }

    JEM_forceinline ListNode* peek() const noexcept {
      return front();
    }
  };

  class SharedAllocator {

    struct FreeListEntry;

    struct Segment;

    struct SegmentHeader {
      size_t                          segmentSize;
      AgtUInt32                       allocCount;
      ipc::offset_ptr<FreeListEntry*> rootEntry;
    };

    struct FreeListEntry {
      ipc::offset_ptr<SegmentHeader*> thisSegment;
      size_t                          entrySize;
      ipc::offset_ptr<FreeListEntry*> rightEntry;
      ipc::offset_ptr<FreeListEntry*> leftEntry;
    };

    struct AllocationEntry {
      ipc::offset_ptr<SegmentHeader*> thisSegment;
      size_t                          allocationSize;
      char                            allocation[];
    };

    struct FreeList {


    };

    struct Segment {
      SegmentHeader* header;
      size_t         maxSize;
    };
  };

  class SharedPageMap {

    struct MapNode {
      AgtUInt32 pageId;
      Page*     mappedPage;
    };

    MapNode*  pNodeTable;
    AgtUInt32 tableSize;
    AgtUInt32 bucketCount;
    AgtUInt32 tombstoneCount;

  public:


  };


  static_assert(sizeof(ObjectInfoCell) == CellSize);
  static_assert(alignof(ObjectInfoCell) == CellAlignment);

  using SharedHandlePool = ObjectPool<SharedHandle, (0x1 << 14) / sizeof(SharedHandle)>;
}

extern "C" {

struct AgtSharedContext_st {

};

struct AgtContext_st {

  AgtUInt32        processId;

  SharedPageMap    sharedPageMap;
  PageList         localFreeList;
  ListNode*        emptyLocalPage;

  AgtSize          localPageSize;
  AgtSize          localEntryCount;

  SharedHandlePool handlePool;

  IpcBlock         ipcBlock;

};


}


namespace {

  std::mutex g_cleanupMtx;



}


AgtStatus Agt::createCtx(AgtContext& pCtx) noexcept {
  return AGT_ERROR_NOT_YET_IMPLEMENTED;
}
void      Agt::destroyCtx(AgtContext ctx) noexcept { }



void*     Agt::ctxAllocLocal(AgtContext ctx, AgtSize size, AgtSize alignment) noexcept {
  return _aligned_malloc(size, alignment);
}
void      Agt::ctxFreeLocal(AgtContext ctx, void* memory, AgtSize size, AgtSize alignment) noexcept {
  _aligned_free(memory);
}