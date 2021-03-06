//
// Created by maxwe on 2021-11-24.
//

#ifndef JEMSYS_AGATE2_POOL_HPP
#define JEMSYS_AGATE2_POOL_HPP

#include <agate2.h>

namespace Agt {
  
  template <typename T>
  inline T* allocArray(jem_size_t arraySize, jem_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>( _aligned_malloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#else
    return static_cast<T*>( std::aligned_alloc(arraySize * sizeof(T), std::max(alignof(T), alignment)) );
#endif
  }
  template <typename T>
  inline void freeArray(T* array, jem_size_t arraySize, jem_size_t alignment) noexcept {
#if defined(_WIN32)
    _aligned_free(array);
#else
    free(array);
#endif
  }
  template <typename T>
  inline T* reallocArray(T* array, jem_size_t newArraySize, jem_size_t oldArraySize, jem_size_t alignment) noexcept {
#if defined(_WIN32)
    return static_cast<T*>(_aligned_realloc(array, newArraySize * sizeof(T), std::max(alignof(T), alignment)));
#else
    auto newArray = allocArray<T>(newArraySize, alignment);
    std::memcpy(newArray, array, oldArraySize * sizeof(T));
    freeArray(array, oldArraySize, alignment);
    return newArray;
#endif
  }

  class Pool {

    using Index = AgtSize;
    using Block = void**;

    struct Slab {
      Index  availableBlocks;
      Block  nextFreeBlock;
      Slab** stackPosition;
    };

    inline constexpr static AgtSize MinimumBlockSize = sizeof(Slab);
    inline constexpr static AgtSize InitialStackSize = 4;
    inline constexpr static AgtSize StackGrowthRate  = 2;
    inline constexpr static AgtSize StackAlignment   = JEM_CACHE_LINE;




    inline Slab* alloc_slab() const noexcept {
#if defined(_WIN32)
      auto result = static_cast<Slab*>(_aligned_malloc(slabAlignment, slabAlignment));

#else
      auto result = static_cast<Slab*>(std::aligned_alloc(slabAlignment, slabAlignment));
#endif
      std::memset(result, 0, slabAlignment);
      return result;
    }
    inline void free_slab(Slab* Slab) const noexcept {
#if defined(_WIN32)
      _aligned_free(Slab);
#else
      std::free(Slab);
#endif
    }

    inline Block lookupBlock(Slab* s, AgtSize blockIndex) const noexcept {
      return reinterpret_cast<Block>(reinterpret_cast<char*>(s) + (blockIndex * blockSize));
    }
    inline Slab*  lookupSlab(void* block) const noexcept {
      return reinterpret_cast<Slab*>(reinterpret_cast<uintptr_t>(block) & slabAlignmentMask);
    }

    inline AgtSize indexOfBlock(Slab* s, Block block) const noexcept {
      return ((std::byte*)block - (std::byte*)s) / blockSize;
    }



    inline bool isEmpty(const Slab* s) const noexcept {
      return s->availableBlocks == blocksPerSlab;
    }
    inline static bool isFull(const Slab* s) noexcept {
      return s->availableBlocks == 0;
    }


    inline void assertSlabIsValid(Slab* s) const noexcept {
      if ( s->availableBlocks == 0 )
        return;
      AgtSize blockCount = 1;
      std::vector<bool> blocks;
      blocks.resize(blocksPerSlab + 1);
      Block currentBlock = s->nextFreeBlock;
      while (blockCount < s->availableBlocks) {
        AgtSize index = indexOfBlock(s, currentBlock);
        assert( !blocks[index] );
        blocks[index] = true;
        ++blockCount;
        currentBlock = (Block)*currentBlock;
      }
    }


    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        Slab** oldStackBase = slabStackBase;
        AgtSize oldStackSize = slabStackTop - oldStackBase;
        AgtSize newStackSize = oldStackSize * StackGrowthRate;
        Slab** newStackBase = reallocArray(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackOnePastTop = newStackBase + oldStackSize;

          for ( Slab** entry = newStackBase; entry != slabStackHead; ++entry )
            (*entry)->stackPosition = entry;
        }
      }

      const auto s = slabStackHead;



      *s = alloc_slab();
      (*s)->availableBlocks = static_cast<jem_u32_t>(blocksPerSlab);
      (*s)->nextFreeBlock   = lookupBlock(*s, 1);
      (*s)->stackPosition = s;

      allocSlab = *s;

      uint32_t i = 1;
      while (  i < blocksPerSlab ) {
        Block block = lookupBlock(*s, i);
        *block = lookupBlock(*s, ++i);
      }

      assertSlabIsValid(*s);

      ++slabStackHead;
    }

    inline void findAllocSlab() noexcept {
      if ( isFull(allocSlab) ) {
        if (fullStackOnePastTop == slabStackHead ) {
          makeNewSlab();
        }
        else {
          if ( allocSlab != *fullStackOnePastTop)
            swapSlabs(allocSlab, *fullStackOnePastTop);
          allocSlab = *++fullStackOnePastTop;
        }
      }
    }

    inline void pruneSlabs(Slab* parentSlab) noexcept {
      if ( isEmpty(parentSlab) ) {
        if ( freeSlab && isEmpty(freeSlab) ) [[unlikely]] {
          removeStackEntry(freeSlab);
        }
        freeSlab = parentSlab;
      }
      else if (parentSlab->stackPosition < fullStackOnePastTop) [[unlikely]] {
        --fullStackOnePastTop;
        if ( parentSlab->stackPosition != fullStackOnePastTop)
          swapSlabs(parentSlab, *fullStackOnePastTop);
      }
    }


    inline static void swapSlabs(Slab* a, Slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }

    inline void removeStackEntry(Slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }


    AgtSize blockSize;
    AgtSize blocksPerSlab;
    Slab**  slabStackBase;
    Slab**  slabStackHead;
    Slab**  slabStackTop;
    Slab**  fullStackOnePastTop;
    Slab*   allocSlab;
    Slab*   freeSlab;
    AgtSize slabAlignment;
    AgtSize slabAlignmentMask;

  public:
    Pool(AgtSize blockSize, AgtSize blocksPerSlab) noexcept
        : blockSize(std::max(blockSize, MinimumBlockSize)),
          blocksPerSlab(blocksPerSlab - 1),
          slabStackBase(allocArray<Slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          fullStackOnePastTop(slabStackHead),
          allocSlab(),
          freeSlab(),
          slabAlignment(std::bit_ceil(this->blockSize * blocksPerSlab)),
          slabAlignmentMask(~(slabAlignment - 1)) {
      std::memset(slabStackBase, 0, sizeof(void*) * InitialStackSize);
      makeNewSlab();
    }

    ~Pool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      AgtSize stackSize = slabStackTop - slabStackBase;
      freeArray(slabStackBase, stackSize, StackAlignment);
    }

    void* alloc_block() noexcept {

      findAllocSlab();

      Block block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<Block>(*block);
      --allocSlab->availableBlocks;

      if (freeSlab == allocSlab)
        freeSlab = nullptr;

      return block;

    }
    void  free_block(void* block_) noexcept {

      auto* parentSlab = lookupSlab(block_);
      assert( ((uintptr_t)block_ - (uintptr_t)parentSlab) % blockSize == 0);
      auto  block = static_cast<Block>(block_);

      *block = parentSlab->nextFreeBlock;
      parentSlab->nextFreeBlock = block;
      ++parentSlab->availableBlocks;

      pruneSlabs(parentSlab);

      allocSlab = parentSlab;
    }

    JEM_nodiscard AgtSize block_size() const noexcept {
      return blockSize;
    }
    JEM_nodiscard AgtSize block_alignment() const noexcept {
      return ((~blockSize) + 1) & blockSize;
    }
  };

  template <AgtSize BlockSize, AgtSize BlocksPerSlab>
  class FixedPool {

    using Index = AgtSize;
    using Block = void**;

    struct Slab {
      Index  availableBlocks;
      Block  nextFreeBlock;
      Slab** stackPosition;
    };

    inline constexpr static AgtSize InitialStackSize  = 4;
    inline constexpr static AgtSize StackGrowthRate   = 2;
    inline constexpr static AgtSize StackAlignment    = JEM_CACHE_LINE;
    inline constexpr static AgtSize SlabSize          = std::bit_ceil(BlockSize * BlocksPerSlab);
    inline constexpr static AgtSize SlabAlignment     = SlabSize;
    inline constexpr static AgtSize SlabAlignmentMask = ~(SlabAlignment - 1);




    inline Slab* new_slab() const noexcept {
#if defined(_WIN32)
      auto result = static_cast<Slab*>(_aligned_malloc(SlabSize, SlabAlignment));

#else
      auto result = static_cast<Slab*>(std::aligned_alloc(SlabSize, SlabAlignment));
#endif
      std::memset(result, 0, SlabSize);
      return result;
    }
    inline void delete_slab(Slab* Slab) const noexcept {
#if defined(_WIN32)
      _aligned_free(Slab);
#else
      std::free(Slab);
#endif
    }

    inline Block lookupBlock(Slab* s, AgtSize blockIndex) const noexcept {
      return reinterpret_cast<Block>(reinterpret_cast<char*>(s) + (blockIndex * BlockSize));
    }
    inline Slab* lookupSlab(void* block) const noexcept {
      return reinterpret_cast<Slab*>(reinterpret_cast<uintptr_t>(block) & SlabAlignmentMask);
    }

    inline AgtSize indexOfBlock(Slab* s, Block block) const noexcept {
      return ((std::byte*)block - (std::byte*)s) / BlockSize;
    }



    inline bool isEmpty(const Slab* s) const noexcept {
      return s->availableBlocks == BlocksPerSlab;
    }
    inline static bool isFull(const Slab* s) noexcept {
      return s->availableBlocks == 0;
    }


    inline void assertSlabIsValid(Slab* s) const noexcept {
      if ( s->availableBlocks == 0 )
        return;
      AgtSize blockCount = 1;
      std::vector<bool> blocks;
      blocks.resize(BlocksPerSlab + 1);
      Block currentBlock = s->nextFreeBlock;
      while (blockCount < s->availableBlocks) {
        AgtSize index = indexOfBlock(s, currentBlock);
        assert( !blocks[index] );
        blocks[index] = true;
        ++blockCount;
        currentBlock = (Block)*currentBlock;
      }
    }


    inline void makeNewSlab() noexcept {

      if ( slabStackHead == slabStackTop ) [[unlikely]] {
        Slab** oldStackBase = slabStackBase;
        AgtSize oldStackSize = slabStackTop - oldStackBase;
        AgtSize newStackSize = oldStackSize * StackGrowthRate;
        Slab** newStackBase = reallocArray(slabStackBase, newStackSize, oldStackSize, StackAlignment);
        if ( slabStackBase != newStackBase ) {
          slabStackBase = newStackBase;
          slabStackTop  = newStackBase + newStackSize;
          slabStackHead = newStackBase + oldStackSize;
          fullStackOnePastTop = newStackBase + oldStackSize;

          for ( Slab** entry = newStackBase; entry != slabStackHead; ++entry )
            (*entry)->stackPosition = entry;
        }
      }

      const auto s = slabStackHead;



      *s = new_slab();
      (*s)->availableBlocks = static_cast<jem_u32_t>(BlocksPerSlab);
      (*s)->nextFreeBlock   = lookupBlock(*s, 1);
      (*s)->stackPosition = s;

      allocSlab = *s;

      uint32_t i = 1;
      while (  i < BlocksPerSlab ) {
        Block block = lookupBlock(*s, i);
        *block = lookupBlock(*s, ++i);
      }

      assertSlabIsValid(*s);

      ++slabStackHead;
    }

    inline void findAllocSlab() noexcept {
      if ( isFull(allocSlab) ) {
        if (fullStackOnePastTop == slabStackHead ) {
          makeNewSlab();
        }
        else {
          if ( allocSlab != *fullStackOnePastTop)
            swapSlabs(allocSlab, *fullStackOnePastTop);
          allocSlab = *++fullStackOnePastTop;
        }
      }
    }

    inline void pruneSlabs(Slab* parentSlab) noexcept {
      if ( isEmpty(parentSlab) ) {
        if ( freeSlab && isEmpty(freeSlab) ) [[unlikely]] {
          removeStackEntry(freeSlab);
        }
        freeSlab = parentSlab;
      }
      else if (parentSlab->stackPosition < fullStackOnePastTop) [[unlikely]] {
        --fullStackOnePastTop;
        if ( parentSlab->stackPosition != fullStackOnePastTop)
          swapSlabs(parentSlab, *fullStackOnePastTop);
      }
    }


    inline static void swapSlabs(Slab* a, Slab* b) noexcept {
      std::swap(*a->stackPosition, *b->stackPosition);
      std::swap(a->stackPosition, b->stackPosition);
    }

    inline void removeStackEntry(Slab* s) noexcept {
      const auto lastPosition = --slabStackHead;
      if ( s->stackPosition != lastPosition ) {
        (*lastPosition)->stackPosition = s->stackPosition;
        std::swap(*s->stackPosition, *lastPosition);
      }
      free_slab(*lastPosition);
    }

    Slab**  slabStackBase;
    Slab**  slabStackHead;
    Slab**  slabStackTop;
    Slab**  fullStackOnePastTop;
    Slab*   allocSlab;
    Slab*   freeSlab;
    AgtSize totalAllocations;

  public:
    FixedPool() noexcept
        : slabStackBase(allocArray<Slab*>(InitialStackSize, StackAlignment)),
          slabStackHead(slabStackBase),
          slabStackTop(slabStackBase + InitialStackSize),
          fullStackOnePastTop(slabStackHead),
          allocSlab(nullptr),
          freeSlab(nullptr),
          totalAllocations(0)
    {
      std::memset(slabStackBase, 0, sizeof(void*) * InitialStackSize);
      makeNewSlab();
    }

    ~FixedPool() noexcept {
      while ( slabStackHead != slabStackBase ) {
        free_slab(*slabStackHead);
        --slabStackHead;
      }

      AgtSize stackSize = slabStackTop - slabStackBase;
      freeArray(slabStackBase, stackSize, StackAlignment);
    }

    JEM_nodiscard void* alloc() noexcept {

      findAllocSlab();

      Block block = allocSlab->nextFreeBlock;
      allocSlab->nextFreeBlock = static_cast<Block>(*block);
      --allocSlab->availableBlocks;
      ++totalAllocations;

      if (freeSlab == allocSlab)
        freeSlab = nullptr;

      return block;

    }
    void  free(void* block_) noexcept {

      auto* parentSlab = lookupSlab(block_);
      assert( ((uintptr_t)block_ - (uintptr_t)parentSlab) % BlockSize == 0);
      auto  block = static_cast<Block>(block_);

      *block = parentSlab->nextFreeBlock;
      parentSlab->nextFreeBlock = block;
      ++parentSlab->availableBlocks;
      --totalAllocations;

      pruneSlabs(parentSlab);

      allocSlab = parentSlab;
    }

    JEM_forceinline AgtSize getAllocationCount() const noexcept {
      return totalAllocations;
    }

    JEM_nodiscard AgtSize blockSize() const noexcept {
      return BlockSize;
    }
    JEM_nodiscard AgtSize blockAlignment() const noexcept {
      return ((~BlockSize) + 1) & BlockSize;
    }
  };


  template <typename T, AgtSize BlocksPerSlab = 255>
  class ObjectPool {
  public:

    ObjectPool() noexcept : fixedPool() {}

    template <typename ...Args>
    JEM_forceinline T*   create(Args&& ... args) noexcept {
      return new(fixedPool.alloc()) T(std::forward<Args>(args)...);
    }

    JEM_forceinline void destroy(T* pObject) noexcept {
      if (pObject != nullptr) {
        void* memory = pObject;
        pObject->~T();
        fixedPool.free(memory);
      }
    }

    JEM_forceinline AgtSize activeObjectCount() const noexcept {
      return fixedPool.getAllocationCount();
    }


  private:
    FixedPool<sizeof(T), BlocksPerSlab> fixedPool;
  };

}

#endif//JEMSYS_AGATE2_POOL_HPP
