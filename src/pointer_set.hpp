//
// Created by maxwe on 2021-09-21.
//

#ifndef JEMSYS_INTERNAL_POINTER_SET_HPP
#define JEMSYS_INTERNAL_POINTER_SET_HPP

#include <jemsys.h>

#include "iterator.hpp"



namespace jem{
  
  namespace impl {
    /// small_ptr_set_base - This is the common code shared among all the
    /// pointer_set<>'s, which is almost everything.  pointer_set has two modes, one
    /// for small and one for large sets.
    ///
    /// Small sets use an array of pointers allocated in the pointer_set object,
    /// which is treated as a simple array of pointers.  When a pointer is added to
    /// the set, the array is scanned to see if the element already exists, if not
    /// the element is 'pushed back' onto the array.  If we run out of space in the
    /// array, we grow into the 'large set' case.  SmallSet should be used when the
    /// sets are often small.  In this case, no memory allocation is used, and only
    /// light-weight and cache-efficient scanning is used.
    ///
    /// Large sets use a classic exponentially-probed hash table.  Empty buckets are
    /// represented with an illegal pointer value (-1) to allow null pointers to be
    /// inserted.  Tombstones are represented with another illegal pointer value
    /// (-2), to allow deletion.  The hash table is resized when the table is 3/4 or
    /// more.  When this happens, the table is doubled in size.
    ///
    class small_ptr_set_base {
      friend class small_ptr_set_iterator_base;

    protected:
      /// SmallArray - Points to a fixed size set of buckets, used in 'small mode'.
      const void** SmallArray;
      /// CurArray - This is the current set of buckets.  If equal to SmallArray,
      /// then the set is in 'small mode'.
      const void** CurArray;
      /// CurArraySize - The allocated size of CurArray, always a power of two.
      unsigned CurArraySize;

      /// Number of elements in CurArray that contain a value or are a tombstone.
      /// If small, all these elements are at the beginning of CurArray and the rest
      /// is uninitialized.
      unsigned NumNonEmpty;
      /// Number of tombstones in CurArray.
      unsigned NumTombstones;

      // Helpers to copy and move construct a pointer_set.
      small_ptr_set_base(const void** SmallStorage,
                         const small_ptr_set_base& that);
      small_ptr_set_base(const void** SmallStorage, unsigned SmallSize,
                         small_ptr_set_base&& that);

      explicit small_ptr_set_base(const void** SmallStorage, unsigned SmallSize)
          : SmallArray(SmallStorage), CurArray(SmallStorage),
            CurArraySize(SmallSize), NumNonEmpty(0), NumTombstones(0) {
        assert(SmallSize && (SmallSize & (SmallSize - 1)) == 0 &&
               "Initial size must be a power of two!");
      }

      ~small_ptr_set_base() {
        if (!isSmall())
          free(CurArray);
      }

    public:
      using size_type = unsigned;

      small_ptr_set_base& operator=(const small_ptr_set_base&) = delete;

      JEM_nodiscard bool empty() const { return size() == 0; }
      size_type size() const { return NumNonEmpty - NumTombstones; }

      void clear() {
        // If the capacity of the array is huge, and the # elements used is small,
        // shrink the array.
        if (!isSmall()) {
          if (size() * 4 < CurArraySize && CurArraySize > 32)
            return shrink_and_clear();
          // Fill the array with empty markers.
          memset(CurArray, -1, CurArraySize * sizeof(void*));
        }

        NumNonEmpty = 0;
        NumTombstones = 0;
      }

    protected:
      static void* getTombstoneMarker() { return reinterpret_cast<void*>(-2); }

      static void* getEmptyMarker() {
        // Note that -1 is chosen to make clear() efficiently implementable with
        // memset and because it's not a valid pointer value.
        return reinterpret_cast<void*>(-1);
      }

      const void** EndPointer() const {
        return isSmall() ? CurArray + NumNonEmpty : CurArray + CurArraySize;
      }

      /// insert_imp - This returns true if the pointer was new to the set, false if
      /// it was already in the set.  This is hidden from the client so that the
      /// derived class can check that the right type of pointer is passed in.
      std::pair<const void* const*, bool> insert_imp(const void* Ptr) {
        if (isSmall()) {
          // Check to see if it is already in the set.
          const void** LastTombstone = nullptr;
          for (const void **APtr = SmallArray, **E = SmallArray + NumNonEmpty;
               APtr != E; ++APtr) {
            const void* Value = *APtr;
            if (Value == Ptr)
              return std::make_pair(APtr, false);
            if (Value == getTombstoneMarker())
              LastTombstone = APtr;
          }

          // Did we find any tombstone marker?
          if (LastTombstone != nullptr) {
            *LastTombstone = Ptr;
            --NumTombstones;
            return std::make_pair(LastTombstone, true);
          }

          // Nope, there isn't.  If we stay small, just 'pushback' now.
          if (NumNonEmpty < CurArraySize) {
            SmallArray[NumNonEmpty++] = Ptr;
            return std::make_pair(SmallArray + (NumNonEmpty - 1), true);
          }
          // Otherwise, hit the big set case, which will call grow.
        }
        return insert_imp_big(Ptr);
      }

      /// erase_imp - If the set contains the specified pointer, remove it and
      /// return true, otherwise return false.  This is hidden from the client so
      /// that the derived class can check that the right type of pointer is passed
      /// in.
      bool erase_imp(const void* Ptr) {
        const void* const* P = find_imp(Ptr);
        if (P == EndPointer())
          return false;

        const void** Loc = const_cast<const void**>(P);
        assert(*Loc == Ptr && "broken find!");
        *Loc = getTombstoneMarker();
        NumTombstones++;
        return true;
      }

      /// Returns the raw pointer needed to construct an iterator.  If element not
      /// found, this will be EndPointer.  Otherwise, it will be a pointer to the
      /// slot which stores Ptr;
      const void* const* find_imp(const void* Ptr) const {
        if (isSmall()) {
          // Linear search for the item.
          for (const void *const *APtr = SmallArray,
                                 *const *E = SmallArray + NumNonEmpty;
               APtr != E; ++APtr)
            if (*APtr == Ptr)
              return APtr;
          return EndPointer();
        }

        // Big set case.
        auto* Bucket = FindBucketFor(Ptr);
        if (*Bucket == Ptr)
          return Bucket;
        return EndPointer();
      }

    private:
      bool isSmall() const { return CurArray == SmallArray; }

      std::pair<const void* const*, bool> insert_imp_big(const void* Ptr);

      const void* const* FindBucketFor(const void* Ptr) const;
      void shrink_and_clear();

      /// Grow - Allocate a larger backing store for the buckets and move it over.
      void Grow(unsigned NewSize);

    protected:
      /// swap - Swaps the elements of two sets.
      /// Note: This method assumes that both sets have the same small size.
      void swap(small_ptr_set_base& RHS);

      void CopyFrom(const small_ptr_set_base& RHS);
      void MoveFrom(unsigned SmallSize, small_ptr_set_base&& RHS);

    private:
      /// Code shared by MoveFrom() and move constructor.
      void MoveHelper(unsigned SmallSize, small_ptr_set_base&& RHS);
      /// Code shared by CopyFrom() and copy constructor.
      void CopyHelper(const small_ptr_set_base& RHS);
    };

    /// small_ptr_set_iterator_base - This is the common base class shared between all
    /// instances of pointer_set_iterator.
    class small_ptr_set_iterator_base {
    protected:
      const void* const* Bucket;
      const void* const* End;

    public:
      explicit small_ptr_set_iterator_base(const void* const* BP, const void* const* E)
          : Bucket(BP), End(E) {
        AdvanceIfNotValid();
      }

      bool operator==(const small_ptr_set_iterator_base& RHS) const {
        return Bucket == RHS.Bucket;
      }
      bool operator!=(const small_ptr_set_iterator_base& RHS) const {
        return Bucket != RHS.Bucket;
      }

    protected:
      /// AdvanceIfNotValid - If the current bucket isn't valid, advance to a bucket
      /// that is.   This is guaranteed to stop because the end() bucket is marked
      /// valid.
      void AdvanceIfNotValid() {
        assert(Bucket <= End);
        while (Bucket != End &&
               (*Bucket == small_ptr_set_base::getEmptyMarker() ||
                *Bucket == small_ptr_set_base::getTombstoneMarker()))
          ++Bucket;
      }
    };

  }
  
  /// pointer_set_iterator - This implements a const_iterator for pointer_set.
  template <typename PtrTy>
  class pointer_set_iterator : public impl::small_ptr_set_iterator_base{
  public:
    using value_type = PtrTy;
    using reference = PtrTy;
    using pointer = PtrTy;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    explicit pointer_set_iterator(const void *const *BP, const void *const *E)
        : small_ptr_set_iterator_base(BP, E) {}

    // Most methods are provided by the base class.

    const pointer operator*() const {
      assert(Bucket < End);
      return static_cast<pointer>(const_cast<void*>(*Bucket));
    }

    inline pointer_set_iterator& operator++() {
      ++Bucket;
      AdvanceIfNotValid();
      return *this;
    }

    pointer_set_iterator operator++(int) {        // Postincrement
      pointer_set_iterator tmp = *this;
      ++*this;
      return tmp;
    }
  };
  
  namespace impl {

    

    /// RoundUpToPowerOfTwo - This is a helper template that rounds N up to the next
    /// power of two (which means N itself if N is already a power of two).
    template<unsigned N>
    struct RoundUpToPowerOfTwo;

    /// RoundUpToPowerOfTwoH - If N is not a power of two, increase it.  This is a
    /// helper template used to implement RoundUpToPowerOfTwo.
    template<unsigned N, bool isPowerTwo>
    struct RoundUpToPowerOfTwoH {
      enum { Val = N };
    };
    template<unsigned N>
    struct RoundUpToPowerOfTwoH<N, false> {
      enum {
        // We could just use NextVal = N+1, but this converges faster.  N|(N-1) sets
        // the right-most zero bits to one all at once, e.g. 0b0011000 -> 0b0011111.
        Val = RoundUpToPowerOfTwo<(N|(N-1)) + 1>::Val
      };
    };

    template<unsigned N>
    struct RoundUpToPowerOfTwo {
      enum { Val = RoundUpToPowerOfTwoH<N, (N&(N-1)) == 0>::Val };
    };

    /// A templated base class for \c pointer_set which provides the
    /// typesafe interface that is common across all small sizes.
    ///
    /// This is particularly useful for passing around between interface boundaries
    /// to avoid encoding a particular small size in the interface boundary.
    template <typename PtrType>
    class small_ptr_set : public small_ptr_set_base {
      using const_ptr_type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<PtrType>>>;

    protected:
      // Forward constructors to the base.
      using small_ptr_set_base::small_ptr_set_base;

    public:
      using iterator = pointer_set_iterator<PtrType>;
      using const_iterator = pointer_set_iterator<PtrType>;
      using key_type = const_ptr_type;
      using value_type = PtrType;

      small_ptr_set(const small_ptr_set &) = delete;

      /// Inserts Ptr if and only if there is no element in the container equal to
      /// Ptr. The bool component of the returned pair is true if and only if the
      /// insertion takes place, and the iterator component of the pair points to
      /// the element equal to Ptr.
      std::pair<iterator, bool> insert(value_type Ptr) {
        auto p = insert_imp(static_cast<const void*>(Ptr));
        return std::make_pair(makeIterator(p.first), p.second);
      }

      /// Insert the given pointer with an iterator hint that is ignored. This is
      /// identical to calling insert(Ptr), but allows pointer_set to be used by
      /// std::insert_iterator and std::inserter().
      iterator insert(iterator, value_type ptr) {
        return insert(ptr).first;
      }

      /// erase - If the set contains the specified pointer, remove it and return
      /// true, otherwise return false.
      bool erase(value_type ptr) {
        return erase_imp(static_cast<const void*>(ptr));
      }
      /// count - Return 1 if the specified pointer is in the set, 0 otherwise.
      size_type count(key_type ptr) const noexcept {
        return find_imp(static_cast<const void*>(ptr)) != EndPointer();
      }
      iterator find(key_type ptr) const noexcept {
        return makeIterator(find_imp(static_cast<const void*>(ptr)));
      }
      bool contains(key_type ptr) const noexcept {
        return find_imp(static_cast<const void*>(ptr)) != EndPointer();
      }

      template <typename IterT>
      void insert(IterT I, IterT E) {
        for (; I != E; ++I)
          insert(*I);
      }

      void insert(std::initializer_list<PtrType> IL) {
        insert(IL.begin(), IL.end());
      }

      iterator begin() const {
        return makeIterator(CurArray);
      }
      iterator end() const { return makeIterator(EndPointer()); }

    private:
      /// Create an iterator that dereferences to same place as the given pointer.
      iterator makeIterator(const void *const *P) const {
        return iterator(P, EndPointer());
      }
    };

    /// Equality comparison for pointer_set.
    ///
    /// Iterates over elements of LHS confirming that each value from LHS is also in
    /// RHS, and that no additional values are in RHS.
    template <typename PtrType>
    bool operator==(const small_ptr_set<PtrType> &LHS,
                    const small_ptr_set<PtrType> &RHS) {
      if (LHS.size() != RHS.size())
        return false;

      for (const auto *KV : LHS)
        if (!RHS.count(KV))
          return false;

      return true;
    }

    /// Inequality comparison for pointer_set.
    ///
    /// Equivalent to !(LHS == RHS).
    template <typename PtrType>
    bool operator!=(const small_ptr_set<PtrType> &LHS,
                    const small_ptr_set<PtrType> &RHS) {
      return !(LHS == RHS);
    }
  }


  

  /// pointer_set - This class implements a set which is optimized for holding
  /// SmallSize or less elements.  This internally rounds up SmallSize to the next
  /// power of two if it is not already a power of two.  See the comments above
  /// small_ptr_set_base for details of the algorithm.
  template<class PtrType, unsigned SmallSize = 8>
  class pointer_set : public impl::small_ptr_set<PtrType> {
    // In small mode pointer_set uses linear search for the elements, so it is
    // not a good idea to choose this value too high. You may consider using a
    // DenseSet<> instead if you expect many elements in the set.
    static_assert(SmallSize <= 32, "SmallSize should be small");

    using BaseT = impl::small_ptr_set<PtrType>;

    // Make sure that SmallSize is a power of two, round up if not.
    enum { SmallSizePowTwo = impl::RoundUpToPowerOfTwo<SmallSize>::Val };
    /// SmallStorage - Fixed size storage used in 'small mode'.
    const void *SmallStorage[SmallSizePowTwo];

  public:
    pointer_set() : BaseT(SmallStorage, SmallSizePowTwo) {}
    pointer_set(const pointer_set &that) : BaseT(SmallStorage, that) {}
    pointer_set(pointer_set &&that)
        : BaseT(SmallStorage, SmallSizePowTwo, std::move(that)) {}

    template<typename It>
    pointer_set(It I, It E) : BaseT(SmallStorage, SmallSizePowTwo) {
      this->insert(I, E);
    }

    pointer_set(std::initializer_list<PtrType> IL)
        : BaseT(SmallStorage, SmallSizePowTwo) {
      this->insert(IL.begin(), IL.end());
    }

    pointer_set<PtrType, SmallSize> & operator=(const pointer_set<PtrType, SmallSize> &RHS) {
      if (&RHS != this)
        this->CopyFrom(RHS);
      return *this;
    }

    pointer_set<PtrType, SmallSize>& operator=(pointer_set<PtrType, SmallSize> &&RHS) noexcept {
      if (&RHS != this)
        this->MoveFrom(SmallSizePowTwo, std::move(RHS));
      return *this;
    }

    pointer_set<PtrType, SmallSize> &
    operator=(std::initializer_list<PtrType> IL) {
      this->clear();
      this->insert(IL.begin(), IL.end());
      return *this;
    }

    /// swap - Swaps the elements of two sets.
    void swap(pointer_set<PtrType, SmallSize> &RHS) {
      impl::small_ptr_set_base::swap(RHS);
    }
  };

}

#endif//JEMSYS_INTERNAL_POINTER_SET_HPP
