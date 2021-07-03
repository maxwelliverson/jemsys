//
// Created by maxwe on 2021-07-02.
//

#ifndef JEMSYS_INTERNAL_HEAPUTILS_HPP
#define JEMSYS_INTERNAL_HEAPUTILS_HPP

#include "ipc/offset_ptr.hpp"

#include <atomic>

namespace {
  template <typename T>
  class atomic_heap{

    struct node_t{
      T          value;
      jem_size_t size;
    };

    class iterator_t{
    public:
    private:
      jem_size_t   index;
      atomic_heap& heap;
    };

  public:

    using node_type = node_t;



  private:

    JEM_forceinline size_t swap_down(size_t i) noexcept {
      node_t* node = arr_ + i;
    }

    JEM_forceinline static jem_size_t right_child(jem_size_t i) noexcept {
      return (i << 1) + 1;
    }
    JEM_forceinline static jem_size_t left_child(jem_size_t i) noexcept {
      return i << 1;
    }
    JEM_forceinline static jem_size_t parent(jem_size_t i) noexcept {
      return i >> 1;
    }


    node_t* arr_;
  };

  class atomic_ipc_heap{
    struct node_t{
      size_t addrOffset;
      size_t availSize;
    };
  public:

    JEM_nodiscard void* acquire(size_t size) noexcept {

    }
    JEM_nodiscard void* acquire(size_t size, size_t align) noexcept {

    }

    void release(void* address, size_t size) noexcept {

    }



  private:
    ipc::offset_ptr<node_t> arr_;
  };


  class atomic_local_heap{
    struct node_t{
      address_t address;
      size_t    size;
    };
  public:

    JEM_nodiscard void* acquire(size_t size) noexcept {
      size_t index = 1;
      node_t* node = arr_ + index;
      node_t* leftChild, *rightChild;
      leftChild = arr_ + (index * 2);
      rightChild = leftChild + 1;
      while (  ) {

      }
    }
    JEM_nodiscard void* acquire(size_t size, size_t align) noexcept {

    }

    void release(void* address, size_t size) noexcept {

    }

  private:

    JEM_nodiscard size_t array_size() const noexcept {
      return arr_[0].size;
    }
    JEM_nodiscard size_t entry_alignment() const noexcept {
      return reinterpret_cast<size_t>(arr_[0].address);
    }

    void swap_child(node_t* node, size_t index) noexcept {
      const size_t leftIndex = index * 2;
      const size_t rightIndex = leftIndex + 1;

      std::atomic_ref<size_t> parentSize{(arr_ + index)->size};
      std::atomic_ref<size_t> leftSize{(arr_ + leftIndex)->size};
      std::atomic_ref<size_t> rightSize{(arr_ + rightIndex)->size};

      size_t newParentSize;
      size_t oldParentSize = parentSize.load();
      size_t oldLeftSize = leftSize.load();
      size_t oldRightSize = rightSize.load();

      leftXchg:
        if ( !parentSize.compare_exchange_strong(oldParentSize, oldLeftSize) ) {

        }
        newParentSize = oldLeftSize;
        if ( !leftSize.compare_exchange_strong(oldLeftSize, oldParentSize) ) {

        }


      rightXchg:
    }

    node_t* arr_;
  };

}

#endif//JEMSYS_INTERNAL_HEAPUTILS_HPP
