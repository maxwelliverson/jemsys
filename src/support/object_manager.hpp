//
// Created by maxwe on 2021-10-27.
//

#ifndef JEMSYS_INTERNAL_ID_MANAGER_HPP
#define JEMSYS_INTERNAL_ID_MANAGER_HPP

#include "jemsys.h"

#include "support/atomicutils.hpp"
#include "vector.hpp"

#include <utility>

/**
 * Anatomy of a Global ID
 *
 * |   Home Process ID   |  Privacy  |  LocalID  |
 * |                     |           |           |
 * |       [32-63]       |    [31]   |  [00-30]  |
 * |       32 bits       |   1 bit   |  31 bits  |
 *
 *
 * */

#define JEM_GLOBAL_ID_EPOCH_OFFSET 54
#define JEM_GLOBAL_ID_PROCESS_ID_OFFSET 32

#define JEM_privacy_shared_bit (0x1 << 31)
#define JEM_privacy_private_bit 0x0

#define JEM_GLUE_ALL(a, b, c) a##b##c
#define JEM_privacy_bit(priv) JEM_GLUE_ALL(JEM_privacy_, priv, _bit)
#define JEM_make_global_id(epoch, procId, localId, privacy) \
  (static_cast<jem::global_id_t>((static_cast<jem_u64_t>(static_cast<jem_u16_t>(epoch))  << JEM_GLOBAL_ID_EPOCH_OFFSET)      | \
                                 (static_cast<jem_u64_t>(static_cast<jem_u32_t>(procId)) << JEM_GLOBAL_ID_PROCESS_ID_OFFSET) | \
                                 static_cast<jem_u64_t>(static_cast<jem_u32_t>(localId)) | \
                                 JEM_privacy_bit(privacy)))


namespace jem {

  enum class object_type_t : jem_u32_t;
  enum class epoch_t       : jem_u16_t;
  enum class global_id_t   : jem_u64_t;
  enum class local_id_t    : jem_u32_t;
  enum class process_id_t  : jem_u32_t;

  struct object {
    atomic_u32_t  refCount;
    object_type_t type;
    global_id_t   id;
  };


  inline static constexpr global_id_t invalid_id = static_cast<global_id_t>(static_cast<jem_global_id_t>(-1));


  class object_manager {

    inline static constexpr jem_size_t EpochMaskBits     = 0x03FF;
    inline static constexpr jem_size_t ProcessIdMaskBits = 0x003FFFFF;

    inline static constexpr jem_size_t GlobalIdEpochMask     = 0xFFC0'0000'0000'0000ULL; // 10 bits
    inline static constexpr jem_size_t GlobalIdProcessIdMask = 0x003F'FFFF'0000'0000ULL; // 22 bits
    inline static constexpr jem_size_t GlobalIdPrivacyMask   = 0x0000'0000'8000'0000ULL; //  1 bits
    inline static constexpr jem_size_t GlobalIdLocalIdMask   = 0x0000'0000'7FFF'FFFFULL; // 31 bits




    inline constexpr static jem_u32_t  BadCellId = std::numeric_limits<jem_u32_t>::max();
    inline constexpr static jem_u32_t  BadPageId = std::numeric_limits<jem_u32_t>::max();

    ///

    struct alignas(32) cell {
      union {
        jem_u32_t nextFreeCell;
        bool      isShared;
      };
      epoch_t     epoch;
      object*     localAddress;
      global_id_t importId;
      // 8 free bytes
    };

    struct page_base {
      page_base* next;
      page_base* prev;
    };

    struct page : page_base {
      jem_u32_t freeCells;
      jem_u32_t nextFreeCell;
      jem_u32_t pageIndex;
      bool      isActive;
    };

    static_assert(sizeof(cell) == 32);

    ///

    simple_mutex_t     writeLock;
    process_id_t       processId;
    jem_u32_t          nextId;

    jem_size_t         pageSize;

    jem_u32_t          cellsPerPage;
    jem_u32_t          log2CellsPerPage;

    jem_u32_t          cellCountMax;
    jem_u32_t          cellCountTotal;
    jem_u32_t          cellCountInUse;

    jem_u32_t          activePages;

    page_base          freeList;
    jem_size_t         freeListSize;
    page*              emptyPage;

    jem::vector<page*> pages;

    cell*              array;

    ///

    JEM_forceinline static epoch_t      epoch(global_id_t id) noexcept {
      return static_cast<epoch_t>(static_cast<jem_u16_t>((static_cast<jem_u64_t>(id) & GlobalIdEpochMask) >> JEM_GLOBAL_ID_EPOCH_OFFSET));
    }
    JEM_forceinline static process_id_t process_id(global_id_t id) noexcept {
      return static_cast<process_id_t>(static_cast<jem_u16_t>((static_cast<jem_u64_t>(id) & GlobalIdProcessIdMask) >> JEM_GLOBAL_ID_PROCESS_ID_OFFSET));
    }
    JEM_forceinline static local_id_t   local_id(global_id_t id) noexcept {
      return static_cast<local_id_t>(index(id));
    }
    JEM_forceinline static bool         is_shared(global_id_t id) noexcept {
      return static_cast<jem_u64_t>(id) & GlobalIdPrivacyMask;
    }

    JEM_forceinline static jem_u32_t    index(global_id_t id) noexcept {
      return static_cast<jem_u32_t>(static_cast<jem_u64_t>(id) & GlobalIdLocalIdMask);
    }

    JEM_forceinline cell* lookup_cell(global_id_t id) noexcept {
      JEM_assert(process_id(id) == processId);
      jem_u32_t i = index(id);
      JEM_assert(i < nextId);
      cell& c = array[i];
      if (c.epoch == epoch(id))
        return &c;
      return nullptr;
    }


    page*                       new_page() noexcept;
    void                        init_page(void* addr, jem_u32_t index) const noexcept;
    void                        delete_page(page* p) noexcept;
    std::pair<cell&, jem_u32_t> alloc_cell() noexcept;
    void                        free_cell(jem_u32_t index) noexcept;



    void remove_from_list(page* p) noexcept {
      p->next->prev = p->prev;
      p->prev->next = p->next;
      --freeListSize;
    }

    inline bool is_empty(page* p) const noexcept {
      return cellsPerPage == p->freeCells;
    }
    inline static bool is_full(page* p) noexcept {
      return p->freeCells == 0;
    }

    page* front_page() const noexcept {
      return (page*)freeList.next;
    }
    page* back_page() const noexcept {
      return (page*)freeList.prev;
    }

    void push_front(page* p) noexcept {
      p->prev = &freeList;
      p->next = freeList.next;
      freeList.next->prev = p;
      freeList.next = p;
      ++freeListSize;
    }
    void push_back(page* p) noexcept {
      p->next = &freeList;
      p->prev = freeList.prev;
      freeList.prev->next = p;
      freeList.prev = p;
      ++freeListSize;
    }

    void move_to_front(page* p) noexcept {
      if (freeList.next != p) {
        remove_from_list(p);
        push_front(p);
      }
    }
    void move_to_back(page* p) noexcept {
      if (freeList.prev != p) {
        remove_from_list(p);
        push_back(p);
      }
    }


    bool at_capacity() const noexcept {
      return cellCountMax == cellCountTotal;
    }

    JEM_forceinline static void advance_epoch(cell& c) noexcept {
      (++reinterpret_cast<jem_u16_t&>(c.epoch)) &= EpochMaskBits;
    }






  public:

    object_manager(process_id_t procId, jem_u32_t maxObjects, jem_size_t pageSize) noexcept;

    object_manager(const object_manager&) = delete;

    ~object_manager();



    bool register_private(object* obj) noexcept {

      if (at_capacity()) [[unlikely]]
        return false;

      auto&& [c, index] = alloc_cell();

      c.localAddress = obj;
      c.isShared     = false;
      c.importId     = invalid_id;
      obj->id        = JEM_make_global_id(c.epoch, processId, index, private);

      return true;
    }
    bool register_shared(object* obj) noexcept {
      if (at_capacity()) [[unlikely]]
        return false;

      auto&& [c, index] = alloc_cell();

      c.localAddress = obj;
      c.isShared     = true;
      c.importId     = invalid_id;
      obj->id        = JEM_make_global_id(c.epoch, processId, index, shared);

      // TODO: do extra stuff here for registering the shared object with the kernel.

      return true;
    }

    void release(object* obj) noexcept {
      release(obj->id);
    }
    void release(global_id_t id) noexcept {
      if (cell* c = lookup_cell(id)) {
        // TODO: Maybe try having these handles be the references???
        // --c->localAddress->refCount
        free_cell(index(id));
        // TODO: idk
      }
    }



    object* lookup(global_id_t id) const noexcept {
      jem_u32_t i = index(id);
      const cell& c = array[i];
      return c.epoch == epoch(id) ? c.localAddress : nullptr;
    }
    object* unsafe_lookup(global_id_t id) const noexcept {
      jem_u32_t i = index(id);
      const cell& c = array[i];
      JEM_assert(c.epoch == epoch(id));
      return c.localAddress;
    }


    void* operator new(size_t size) noexcept {
      static char buffer[sizeof(object_manager)] = {};
#if !defined(NDEBUG)
      std::atomic_bool isInUse = false;
      bool wasInUse = isInUse.exchange(true);
      JEM_assert(wasInUse == false);
#endif

      return buffer;
    }
    void  operator delete(void* addr) noexcept { }
  };


}

#endif//JEMSYS_INTERNAL_ID_MANAGER_HPP
