//
// Created by maxwe on 2021-10-27.
//

#include "object_manager.hpp"
#include "ipc/process.hpp"

#include <bit>
#include <ranges>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#pragma comment(lib, "mincore")


/**

    simple_mutex_t     writeLock;
    process_id_t       processId;
    jem_u32_t          nextId;

    jem_u32_t          log2CellsPerPage;

    jem_u32_t          cellCountMax;
    jem_u32_t          cellCountTotal;
    jem_u32_t          cellCountInUse;
    jem_u32_t          cellsPerPage;

    page_base          freeList;
    jem_size_t         freeListSize;
    page*              emptyPage;

    jem::vector<page*> pages;

    cell*              array;

 * */

// inline constexpr static jem_size_t PageSize = (1 << 16);


JEM_forceinline jem_size_t align_to(jem_size_t size, jem_size_t align) noexcept {
  return ((size - 1) | (align - 1)) + 1;
}



jem::object_manager::object_manager(process_id_t procId, jem_u32_t maxObjects, jem_size_t pageSize) noexcept
    : writeLock(),
      processId(procId),
      nextId(0),
      pageSize(pageSize),
      cellsPerPage(pageSize / sizeof(cell)),
      log2CellsPerPage(std::countr_zero(cellsPerPage)),
      cellCountMax(maxObjects),
      cellCountTotal(0),
      cellCountInUse(0),
      activePages(0),
      freeList{&freeList, &freeList},
      freeListSize(0),
      emptyPage(nullptr),
      pages(),
      array()
{
  jem_size_t totalArraySize;
  jem_size_t osMinPageSize = JEM_VIRTUAL_PAGE_SIZE;
  pageSize = align_to(pageSize, osMinPageSize);
  totalArraySize = align_to(cellCountMax * sizeof(cell), pageSize);

  void* reservedMemory = VirtualAlloc(nullptr, totalArraySize, MEM_RESERVE, PAGE_NOACCESS);

  array = static_cast<cell*>(reservedMemory);

  void* placeholder = ipc::this_process.reserve_memory(totalArraySize);
  [[maybe_unused]] bool result = ipc::this_process.split(placeholder, pageSize);
  JEM_assert(result == true);
  array = static_cast<cell*>(placeholder);
  push_front(new_page());
}


jem::object_manager::~object_manager() {
  for (auto page : pages)
    delete page;
  jem_size_t totalArraySize = totalArraySize = align_to(cellCountMax * sizeof(cell), pageSize);
  VirtualFree(array, totalArraySize, MEM_DECOMMIT | MEM_RELEASE);
}


void jem::object_manager::init_page(void* addr, jem_u32_t index) const noexcept {
  auto cells = static_cast<cell*>(addr);
  page* p = pages[index];
  jem_u32_t cellIndex = index * cellsPerPage;
  jem_u32_t endCellIndex = cellIndex + cellsPerPage;

  p->nextFreeCell = cellIndex;
  p->freeCells    = cellsPerPage;
  p->pageIndex    = index;

  while ( cellIndex < endCellIndex ) {
    cell& c = array[cellIndex];
    c.nextFreeCell = ++cellIndex;
  }

}


jem::object_manager::page* jem::object_manager::new_page() noexcept {

  using namespace std::views;

  page* p;
  auto index = static_cast<jem_u32_t>(pages.size());
  void* addr;

  if (activePages == pages.size()) {
    p = new page();
    pages.push_back(p);
    addr = array + (cellsPerPage * index);
    // ipc::this_process.split(addr, pageSize);
  }
  else {
    for (page* p_ : pages | reverse ) {
      --index;
      if ( !p_->isActive ) {
        p = p_;
        break;
      }
    }
    JEM_assert( p != nullptr );
    addr = array + (cellsPerPage * index);
  }

  init_page(VirtualAlloc(addr, pageSize, MEM_COMMIT, PAGE_READWRITE), index);
  // init_page(ipc::this_process.local_alloc(addr, pageSize), index);

  JEM_assert( p != nullptr );

  p->isActive = true;
  p->pageIndex = index;

  ++activePages;

  return p;
}

void                       jem::object_manager::delete_page(page* p) noexcept {
  void* addr = array + (p->pageIndex * cellsPerPage);
  auto result = VirtualFree(addr, pageSize, MEM_DECOMMIT);
  JEM_assert( result == TRUE );
  // ipc::this_process.local_free(addr);
  p->isActive = false;
  --activePages;
}


std::pair<jem::object_manager::cell&, jem_u32_t> jem::object_manager::alloc_cell() noexcept {
  page* allocPage;

  writeLock.acquire();

  if (freeListSize == 0 || is_full((allocPage = front_page()))) {
    allocPage = std::exchange(emptyPage, nullptr);
    if (!allocPage)
      allocPage = new_page();
    push_front(allocPage);
    ++freeListSize;
  }

  JEM_assert(allocPage != nullptr);
  JEM_assert(!is_full(allocPage));

  jem_u32_t nextFreeIndex = allocPage->nextFreeCell;
  cell& c = array[nextFreeIndex];
  allocPage->nextFreeCell = c.nextFreeCell;
  --allocPage->freeCells;
  if (is_full(allocPage))
    move_to_back(allocPage);

  ++cellCountInUse;

  writeLock.release();

  return { c, nextFreeIndex };
}

void jem::object_manager::free_cell(jem_u32_t index) noexcept {
  jem_u32_t pageIndex = index >> log2CellsPerPage;
  JEM_assert(pageIndex < pages.size());

  writeLock.acquire();

  page* p = pages[pageIndex];
  cell& c = array[index];

  c.nextFreeCell = p->nextFreeCell;
  p->nextFreeCell = index;
  ++p->freeCells;
  advance_epoch(c);

  if (is_empty(p)) {
    remove_from_list(p);
    if (auto otherEmpty = std::exchange(emptyPage, p))
      delete_page(otherEmpty);
  }
  else
    move_to_front(p);

  --cellCountInUse;

  writeLock.release();
}