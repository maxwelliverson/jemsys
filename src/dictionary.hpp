//
// Created by maxwe on 2021-09-18.
//

#ifndef JEMSYS_INTERNAL_DICTIONARY_HPP
#define JEMSYS_INTERNAL_DICTIONARY_HPP


#include "quartz/core.h"

#include <utility>
#include <string_view>
#include <memory>
#include <ranges>
#include <bit>

namespace qtz{

  struct nothing_t{};
  inline constexpr static nothing_t nothing{};

  template <typename BeginIter, typename EndIter = BeginIter>
  class range_view{
  public:

    using iterator = BeginIter;
    using sentinel = EndIter;

    range_view(const range_view&) = default;
    range_view(range_view&&) noexcept = default;

    range_view(iterator b, sentinel e) noexcept : begin_(std::move(b)), end_(std::move(e)){}
    template <typename Rng> requires(std::ranges::range<Rng> && !std::same_as<std::remove_cvref_t<Rng>, range_view>)
    range_view(Rng&& rng) noexcept
        : begin_(std::ranges::begin(rng)),
          end_(std::ranges::end(rng)){}

    JEM_nodiscard iterator begin() const noexcept {
      return begin_;
    }
    JEM_nodiscard sentinel end()   const noexcept {
      return end_;
    }

  private:
    BeginIter begin_;
    EndIter   end_;
  };

  template <typename T, typename S>
  range_view<T, S> make_range(T x, S y) {
    return range_view<T, S>(std::move(x), std::move(y));
  }
  template <typename T, typename S>
  range_view<T, S> make_range(std::pair<T, S> p) {
    return range_view<T, S>(std::move(p.first), std::move(p.second));
  }



  template <typename DerivedT,
            typename IteratorCategoryT,
            typename T,
            typename DifferenceTypeT = std::ptrdiff_t,
            typename PointerT = T *,
            typename ReferenceT = T &>
  class iterator_facade_base : public std::iterator<IteratorCategoryT, T, DifferenceTypeT, PointerT, ReferenceT> {
  protected:

    inline constexpr static bool IsRandomAccess  = std::derived_from<IteratorCategoryT, std::random_access_iterator_tag>;
    inline constexpr static bool IsBidirectional = std::derived_from<IteratorCategoryT, std::bidirectional_iterator_tag>;

    /// A proxy object for computing a reference via indirecting a copy of an
    /// iterator. This is used in APIs which need to produce a reference via
    /// indirection but for which the iterator object might be a temporary. The
    /// proxy preserves the iterator internally and exposes the indirected
    /// reference via a conversion operator.
    class reference_proxy {
      friend iterator_facade_base;

      DerivedT I;

      reference_proxy(DerivedT I) : I(std::move(I)) {}

    public:
      operator ReferenceT() const { return *I; }
    };

    using derived_type = DerivedT;

  public:

    using iterator_category = IteratorCategoryT;
    using value_type        = T;
    using difference_type   = DifferenceTypeT;
    using pointer           = PointerT;
    using reference         = ReferenceT;



    derived_type operator+(difference_type n) const requires(IsRandomAccess)  {
      static_assert(std::is_base_of<iterator_facade_base, derived_type>::value,
                    "Must pass the derived type to this template!");
      derived_type tmp = *static_cast<const derived_type *>(this);
      tmp += n;
      return tmp;
    }
    friend derived_type operator+(difference_type n, const derived_type &i) requires(IsRandomAccess) {
      return i + n;
    }
    derived_type operator-(difference_type n) const requires(IsRandomAccess) {
      derived_type tmp = *static_cast<const derived_type *>(this);
      tmp -= n;
      return tmp;
    }

    derived_type& operator++() {
      return static_cast<derived_type *>(this)->operator+=(1);
    }
    derived_type operator++(int) {
      derived_type tmp = *static_cast<derived_type *>(this);
      ++*static_cast<derived_type *>(this);
      return tmp;
    }
    derived_type &operator--()   requires(IsBidirectional) {
      static_assert(
        IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      return static_cast<derived_type *>(this)->operator-=(1);
    }
    derived_type operator--(int) requires(IsBidirectional) {
      static_assert(
        IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      derived_type tmp = *static_cast<derived_type *>(this);
      --*static_cast<derived_type *>(this);
      return tmp;
    }


    PointerT operator->() { return std::addressof(static_cast<derived_type *>(this)->operator*()); }
    PointerT operator->() const { return std::addressof(static_cast<const derived_type *>(this)->operator*()); }
    reference_proxy operator[](difference_type n) requires(IsRandomAccess) { return reference_proxy(static_cast<derived_type *>(this)->operator+(n)); }
    reference_proxy operator[](difference_type n) const requires(IsRandomAccess) { return reference_proxy(static_cast<const derived_type *>(this)->operator+(n)); }
  };

  /// CRTP base class for adapting an iterator to a different type.
  ///
  /// This class can be used through CRTP to adapt one iterator into another.
  /// Typically this is done through providing in the derived class a custom \c
  /// operator* implementation. Other methods can be overridden as well.
  template <
    typename DerivedT,
    typename WrappedIteratorT,
    typename IteratorCategoryT = typename std::iterator_traits<WrappedIteratorT>::iterator_category,
    typename T                 = std::iter_value_t<WrappedIteratorT>,
    typename DifferenceTypeT   = std::iter_difference_t<WrappedIteratorT>,
    typename PointerT          = std::conditional_t<std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<std::iter_value_t<WrappedIteratorT>>>, typename std::iterator_traits<WrappedIteratorT>::pointer, T*>,
    typename ReferenceT        = std::conditional_t<std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<std::iter_value_t<WrappedIteratorT>>>, std::iter_reference_t<WrappedIteratorT>, T&>>
  class iterator_adaptor_base : public iterator_facade_base<DerivedT, IteratorCategoryT, T, DifferenceTypeT, PointerT, ReferenceT> {
    using BaseT = typename iterator_adaptor_base::iterator_facade_base;

  protected:
    WrappedIteratorT I;

    iterator_adaptor_base() = default;

    explicit iterator_adaptor_base(WrappedIteratorT u) : I(std::move(u)) {
      static_assert(std::is_base_of<iterator_adaptor_base, DerivedT>::value,
                    "Must pass the derived type to this template!");
    }

    const WrappedIteratorT &wrapped() const { return I; }

  public:
    using difference_type = DifferenceTypeT;

    DerivedT &operator+=(difference_type n) {
      static_assert(
        BaseT::IsRandomAccess,
        "The '+=' operator is only defined for random access iterators.");
      I += n;
      return *static_cast<DerivedT *>(this);
    }
    DerivedT &operator-=(difference_type n) {
      static_assert(
        BaseT::IsRandomAccess,
        "The '-=' operator is only defined for random access iterators.");
      I -= n;
      return *static_cast<DerivedT *>(this);
    }
    using BaseT::operator-;
    difference_type operator-(const DerivedT &RHS) const {
      static_assert(
        BaseT::IsRandomAccess,
        "The '-' operator is only defined for random access iterators.");
      return I - RHS.I;
    }

    // We have to explicitly provide ++ and -- rather than letting the facade
    // forward to += because WrappedIteratorT might not support +=.
    using BaseT::operator++;
    DerivedT &operator++() {
      ++I;
      return *static_cast<DerivedT *>(this);
    }
    using BaseT::operator--;
    DerivedT &operator--() {
      static_assert(
        BaseT::IsBidirectional,
        "The decrement operator is only defined for bidirectional iterators.");
      --I;
      return *static_cast<DerivedT *>(this);
    }

    bool operator==(const DerivedT &RHS) const { return I == RHS.I; }
    bool operator<(const DerivedT &RHS) const {
      static_assert(
        BaseT::IsRandomAccess,
        "Relational operators are only defined for random access iterators.");
      return I < RHS.I;
    }

    ReferenceT operator*() const { return *I; }
  };



  namespace impl{
    
    template <typename T>
    inline constexpr static jem_size_t ptr_free_low_bits = std::countr_zero(alignof(T));
    
    class dictionary_entry_base {
      jem_u64_t keyLength;

    public:
      explicit dictionary_entry_base(jem_u64_t keyLength) : keyLength(keyLength) {}

      JEM_nodiscard jem_u64_t get_key_length() const { return keyLength; }
    };

    template <typename Val>
    class dictionary_entry_storage : public dictionary_entry_base{
      Val second;
    public:


      explicit dictionary_entry_storage(size_t keyLength)
          : dictionary_entry_base(keyLength), second() {}
      template<typename... InitTy>
      dictionary_entry_storage(size_t keyLength, InitTy &&...initVals)
          : dictionary_entry_base(keyLength),
            second(std::forward<InitTy>(initVals)...) {}
      dictionary_entry_storage(dictionary_entry_storage &e) = delete;

      const Val &get() const { return second; }
      Val &get() { return second; }

      void set(const Val &V) { second = V; }
    };

    template<>
    class dictionary_entry_storage<nothing_t> : public dictionary_entry_base {
    public:
      explicit dictionary_entry_storage(size_t keyLength, nothing_t none = nothing)
          : dictionary_entry_base(keyLength) {}
      dictionary_entry_storage(dictionary_entry_storage &entry) = delete;

      JEM_nodiscard nothing_t get() const { return nothing; }
    };


    class dictionary {

      
    protected:

      // Array of NumBuckets pointers to entries, null pointers are holes.
      // TheTable[NumBuckets] contains a sentinel value for easy iteration. Followed
      // by an array of the actual hash values as unsigned integers.
      dictionary_entry_base** TheTable      = nullptr;
      jem_u32_t               NumBuckets    = 0;
      jem_u32_t               NumItems      = 0;
      jem_u32_t               NumTombstones = 0;
      jem_u32_t               ItemSize;

    protected:
      explicit dictionary(unsigned itemSize) noexcept : ItemSize(itemSize) {}
      dictionary(dictionary &&RHS) noexcept
          : TheTable(RHS.TheTable),
            NumBuckets(RHS.NumBuckets),
            NumItems(RHS.NumItems),
            NumTombstones(RHS.NumTombstones),
            ItemSize(RHS.ItemSize) {
        RHS.TheTable = nullptr;
        RHS.NumBuckets = 0;
        RHS.NumItems = 0;
        RHS.NumTombstones = 0;
      }

      /// Allocate the table with the specified number of buckets and otherwise
      /// setup the map as empty.
      void init(jem_u32_t Size) noexcept;
      void explicit_init(jem_u32_t size) noexcept;

      jem_u32_t  rehash_table(jem_u32_t BucketNo) noexcept;

      /// lookup_bucket_for - Look up the bucket that the specified string should end
      /// up in.  If it already exists as a key in the map, the Item pointer for the
      /// specified bucket will be non-null.  Otherwise, it will be null.  In either
      /// case, the FullHashValue field of the bucket will be set to the hash value
      /// of the string.
      jem_u32_t lookup_bucket_for(std::string_view Key) noexcept;

      /// find_key - Look up the bucket that contains the specified key. If it exists
      /// in the map, return the bucket number of the key.  Otherwise return -1.
      /// This does not modify the map.
      JEM_nodiscard jem_i32_t find_key(std::string_view Key) const noexcept;

      /// remove_key - Remove the specified dictionary_entry from the table, but do not
      /// delete it.  This aborts if the value isn't in the table.
      void remove_key(dictionary_entry_base *V) noexcept;

      /// remove_key - Remove the dictionary_entry for the specified key from the
      /// table, returning it.  If the key is not in the table, this returns null.
      dictionary_entry_base *remove_key(std::string_view Key) noexcept;


      void dealloc_table() noexcept;


    public:
      inline static dictionary_entry_base* get_tombstone_val() noexcept {
        constexpr static jem_u32_t free_low_bits = ptr_free_low_bits<dictionary_entry_base*>;
        auto Val = static_cast<uintptr_t>(-1);
        Val <<= free_low_bits;
        return reinterpret_cast<dictionary_entry_base *>(Val);
      }

      JEM_nodiscard jem_u32_t  get_num_buckets() const noexcept { return NumBuckets; }
      JEM_nodiscard jem_u32_t  get_num_items() const noexcept { return NumItems; }

      JEM_nodiscard bool empty() const noexcept { return NumItems == 0; }
      JEM_nodiscard jem_u32_t  size() const noexcept { return NumItems; }

      void swap(dictionary& other) noexcept {
        std::swap(TheTable, other.TheTable);
        std::swap(NumBuckets, other.NumBuckets);
        std::swap(NumItems, other.NumItems);
        std::swap(NumTombstones, other.NumTombstones);
      }
    };

    template<typename DerivedTy, typename T>
    class dictionary_iter_base : public iterator_facade_base<DerivedTy, std::forward_iterator_tag, T> {
    protected:
      impl::dictionary_entry_base** Ptr = nullptr;

      using derived_type = DerivedTy;

    public:
      dictionary_iter_base() = default;

      explicit dictionary_iter_base(impl::dictionary_entry_base** Bucket, bool NoAdvance = false)
          : Ptr(Bucket) {
        if (!NoAdvance)
          AdvancePastEmptyBuckets();
      }

      derived_type& operator=(const derived_type& Other) {
        Ptr = Other.Ptr;
        return static_cast<derived_type &>(*this);
      }



      derived_type &operator++() {// Preincrement
        ++Ptr;
        AdvancePastEmptyBuckets();
        return static_cast<derived_type &>(*this);
      }

      derived_type operator++(int) {// Post-increment
        derived_type Tmp(Ptr);
        ++*this;
        return Tmp;
      }


      friend bool operator==(const derived_type& LHS, const derived_type &RHS) noexcept {
        return LHS.Ptr == RHS.Ptr;
      }

    private:
      void AdvancePastEmptyBuckets() {
        while (*Ptr == nullptr || *Ptr == impl::dictionary::get_tombstone_val())
          ++Ptr;
      }
    };

  }

  template<typename ValueTy>
  class dictionary_const_iterator;
  template<typename ValueTy>
  class dictionary_iterator;
  template<typename ValueTy>
  class dictionary_key_iterator;




  template<typename Val>
  class dictionary_entry final : public impl::dictionary_entry_storage<Val> {
    
    static dictionary_entry* alloc(jem_size_t keySize) noexcept {
      return static_cast<dictionary_entry*>(_aligned_malloc(sizeof(dictionary_entry) + keySize + 1, alignof(dictionary_entry)));
    }
    static void dealloc(dictionary_entry* mem) noexcept {
      _aligned_free(mem);
    }
    
  public:
    using impl::dictionary_entry_storage<Val>::dictionary_entry_storage;

    JEM_nodiscard std::string_view key() const {
      return std::string_view(get_key_data(), this->get_key_length());
    }

    /// get_key_data - Return the start of the string data that is the key for this
    /// value.  The string data is always stored immediately after the
    /// dictionary_entry object.
    JEM_nodiscard const char* get_key_data() const {
      return reinterpret_cast<const char*>(this + 1);
    }

    /// Create a dictionary_entry for the specified key construct the value using
    /// \p InitiVals.
    template <typename... Args>
    static dictionary_entry* create(std::string_view key, Args&& ...args) {
      size_t keyLength = key.length();
      

      auto *newItem = alloc(keyLength);
      
      assert(newItem && "Unhandled \"out of memory\" error");

      // Construct the value.
      new (newItem) dictionary_entry(keyLength, std::forward<Args>(args)...);

      // Copy the string information.
      auto strBuffer = const_cast<char*>(newItem->get_key_data());
      if (keyLength > 0)
        std::memcpy(strBuffer, key.data(), keyLength);
      strBuffer[keyLength] = 0;// Null terminate for convenience of clients.
      return newItem;
    }

    /// Getdictionary_entryFromKeyData - Given key data that is known to be embedded
    /// into a dictionary_entry, return the dictionary_entry itself.
    static dictionary_entry& get_dictionary_entry_from_key_data(const char* keyData) {
      auto ptr = const_cast<char *>(keyData) - sizeof(dictionary_entry<Val>);
      return *reinterpret_cast<dictionary_entry *>(ptr);
    }

    /// Destroy - Destroy this dictionary_entry, releasing memory back to the
    /// specified allocator.
    void destroy() {
      // Free memory referenced by the item.
      this->~dictionary_entry();
      dealloc(this);
    }
  };



  template <typename T>
  class dictionary : public impl::dictionary{

  public:
    using entry_type = dictionary_entry<T>;

    dictionary() noexcept : impl::dictionary(static_cast<jem_u32_t>(sizeof(entry_type))) {}

    explicit dictionary(jem_u32_t InitialSize)
        : impl::dictionary(static_cast<jem_u32_t>(sizeof(entry_type))) {
      explicit_init(InitialSize);
    }

    dictionary(std::initializer_list<std::pair<std::string_view, T>> List)
        : impl::dictionary(static_cast<jem_u32_t>(sizeof(entry_type))){
      explicit_init(List.size());
      for (const auto &P : List) {
        insert(P);
      }
    }

    dictionary(dictionary &&RHS) noexcept
        : impl::dictionary(std::move(RHS)){}

    dictionary(const dictionary &RHS)
        : impl::dictionary(static_cast<jem_u32_t>(sizeof(entry_type))){
      if (RHS.empty())
        return;

      // Allocate TheTable of the same size as RHS's TheTable, and set the
      // sentinel appropriately (and NumBuckets).
      init(RHS.NumBuckets);
      auto* HashTable    = (jem_u32_t*)(TheTable + NumBuckets + 1);
      auto* RHSHashTable = (jem_u32_t*)(RHS.TheTable + NumBuckets + 1);

      NumItems = RHS.NumItems;
      NumTombstones = RHS.NumTombstones;

      for (jem_u32_t I = 0, E = NumBuckets; I != E; ++I) {
        impl::dictionary_entry_base *Bucket = RHS.TheTable[I];
        if (!Bucket || Bucket == get_tombstone_val()) {
          TheTable[I] = Bucket;
          continue;
        }

        TheTable[I] = entry_type::create(
          static_cast<entry_type *>(Bucket)->key(),
          static_cast<entry_type *>(Bucket)->getValue());
        HashTable[I] = RHSHashTable[I];
      }
    }

    dictionary& operator=(dictionary&& RHS) noexcept {
      impl::dictionary::swap(RHS);
      return *this;
    }

    ~dictionary() {
      // Delete all the elements in the map, but don't reset the elements
      // to default values.  This is a copy of clear(), but avoids unnecessary
      // work not required in the destructor.
      if (!empty()) {
        for (unsigned I = 0, E = NumBuckets; I != E; ++I) {
          impl::dictionary_entry_base *Bucket = TheTable[I];
          if (Bucket && Bucket != get_tombstone_val()) {
            static_cast<entry_type *>(Bucket)->destroy();
          }
        }
      }
      this->dealloc_table();
    }

    using mapped_type = T;
    using value_type = dictionary_entry<T>;
    using size_type = size_t;

    using const_iterator = dictionary_const_iterator<T>;
    using iterator = dictionary_iterator<T>;

    iterator begin() noexcept { return iterator(TheTable, NumBuckets == 0); }
    iterator end() noexcept { return iterator(TheTable + NumBuckets, true); }
    const_iterator begin() const noexcept {
      return const_iterator(TheTable, !NumBuckets);
    }
    const_iterator end() const noexcept {
      return const_iterator(TheTable + NumBuckets, true);
    }

    range_view<dictionary_key_iterator<T>> keys() const noexcept {
      return { dictionary_key_iterator<T>(begin()),
              dictionary_key_iterator<T>(end()) };
    }

    iterator find(std::string_view Key) {
      int Bucket = find_key(Key);
      if (Bucket == -1)
        return end();
      return iterator(TheTable + Bucket, true);
    }

    const_iterator find(std::string_view Key) const {
      int Bucket = find_key(Key);
      if (Bucket == -1)
        return end();
      return const_iterator(TheTable + Bucket, true);
    }

    /// lookup - Return the entry for the specified key, or a default
    /// constructed value if no such entry exists.
    T lookup(std::string_view Key) const {
      const_iterator it = find(Key);
      if (it != end())
        return it->get();
      return T();
    }

    /// Lookup the ValueTy for the \p Key, or create a default constructed value
    /// if the key is not in the map.
    T& operator[](std::string_view Key) noexcept { return try_emplace(Key).first->get(); }

    /// count - Return 1 if the element is in the map, 0 otherwise.
    JEM_nodiscard size_type count(std::string_view Key) const { return find(Key) == end() ? 0 : 1; }

    template<typename InputTy>
    JEM_nodiscard size_type count(const dictionary_entry<InputTy>& entry) const {
      return count(entry.key());
    }

    /// equal - check whether both of the containers are equal.
    bool operator==(const dictionary &RHS) const {
      if (size() != RHS.size())
        return false;

      for (const auto &KeyValue : *this) {
        auto FindInRHS = RHS.find(KeyValue.key());

        if (FindInRHS == RHS.end())
          return false;

        if (KeyValue.get() != FindInRHS->get())
          return false;
      }

      return true;
    }

    /// insert - Insert the specified key/value pair into the map.  If the key
    /// already exists in the map, return false and ignore the request, otherwise
    /// insert it and return true.
    bool insert(entry_type *KeyValue) {
      unsigned BucketNo = lookup_bucket_for(KeyValue->key());
      impl::dictionary_entry_base *&Bucket = TheTable[BucketNo];
      if (Bucket && Bucket != get_tombstone_val())
        return false;// Already exists in map.

      if (Bucket == get_tombstone_val())
        --NumTombstones;
      Bucket = KeyValue;
      ++NumItems;
      VK_assert(NumItems + NumTombstones <= NumBuckets);

      rehash_table();
      return true;
    }

    /// insert - Inserts the specified key/value pair into the map if the key
    /// isn't already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    std::pair<iterator, bool> insert(std::pair<std::string_view, T> KV) {
      return try_emplace(KV.first, std::move(KV.second));
    }

    /// Inserts an element or assigns to the current element if the key already
    /// exists. The return type is the same as try_emplace.
    template<typename V>
    std::pair<iterator, bool> insert_or_assign(std::string_view Key, V &&Val) {
      auto Ret = try_emplace(Key, std::forward<V>(Val));
      if (!Ret.get())
        Ret.first->get() = std::forward<V>(Val);
      return Ret;
    }

    /// Emplace a new element for the specified key into the map if the key isn't
    /// already in the map. The bool component of the returned pair is true
    /// if and only if the insertion takes place, and the iterator component of
    /// the pair points to the element with key equivalent to the key of the pair.
    template<typename... ArgsTy>
    std::pair<iterator, bool> try_emplace(std::string_view Key, ArgsTy &&...Args) {
      unsigned BucketNo = lookup_bucket_for(Key);
      impl::dictionary_entry_base *&Bucket = TheTable[BucketNo];
      if (Bucket && Bucket != get_tombstone_val())
        return std::make_pair(iterator(TheTable + BucketNo, false),
                              false);// Already exists in map.

      if (Bucket == get_tombstone_val())
        --NumTombstones;
      Bucket = entry_type::create(Key, std::forward<ArgsTy>(Args)...);
      ++NumItems;
      assert(NumItems + NumTombstones <= NumBuckets);

      BucketNo = rehash_table(BucketNo);
      return std::make_pair(iterator(TheTable + BucketNo, false), true);
    }

    // clear - Empties out the dictionary
    void clear() {
      if (empty())
        return;

      // Zap all values, resetting the keys back to non-present (not tombstone),
      // which is safe because we're removing all elements.
      for (unsigned I = 0, E = NumBuckets; I != E; ++I) {
        impl::dictionary_entry_base *&Bucket = TheTable[I];
        if (Bucket && Bucket != get_tombstone_val()) {
          static_cast<entry_type *>(Bucket)->destroy();
        }
        Bucket = nullptr;
      }

      NumItems = 0;
      NumTombstones = 0;
    }

    /// remove - Remove the specified key/value pair from the map, but do not
    /// erase it.  This aborts if the key is not in the map.
    void remove(entry_type *KeyValue) { remove_key(KeyValue); }

    void erase(iterator I) {
      entry_type &V = *I;
      remove(&V);
      V.destroy();
    }

    bool erase(std::string_view Key) {
      iterator I = find(Key);
      if (I == end())
        return false;
      erase(I);
      return true;
    }
  };






  template<typename T>
  class dictionary_const_iterator : public impl::dictionary_iter_base<
                                      dictionary_const_iterator<T>,
                                      const dictionary_entry<T>> {
    using base = impl::dictionary_iter_base<dictionary_const_iterator<T>, const dictionary_entry<T>>;

  public:
    using base::base;

    const dictionary_entry<T>& operator*() const noexcept {
      return *static_cast<const dictionary_entry<T>*>(*this->Ptr);
    }
  };

  template<typename T>
  class dictionary_iterator : public impl::dictionary_iter_base<
                                dictionary_iterator<T>,
                                dictionary_entry<T>> {
    using base = impl::dictionary_iter_base<dictionary_iterator<T>, dictionary_entry<T>>;

  public:
    using base::base;

    dictionary_entry<T>& operator*() const noexcept {
      return *static_cast<dictionary_entry<T>*>(*this->Ptr);
    }

    operator dictionary_const_iterator<T>() const noexcept {
      return dictionary_const_iterator<T>(this->Ptr, true);
    }
  };

  template<typename T>
  class dictionary_key_iterator : public iterator_adaptor_base<
                                    dictionary_key_iterator<T>,
                                    dictionary_const_iterator<T>,
                                    std::forward_iterator_tag,
                                    std::string_view>{
    using base = iterator_adaptor_base<dictionary_key_iterator, dictionary_const_iterator<T>, std::forward_iterator_tag, std::string_view>;
  public:

    dictionary_key_iterator() = default;
    explicit dictionary_key_iterator(dictionary_const_iterator<T> iter) noexcept : base(std::move(iter)){}


    std::string_view& operator*() const noexcept {
      cache_key = this->wrapped()->key();
      return cache_key;
    }

  private:
    mutable std::string_view cache_key;
  };

}

#endif//JEMSYS_INTERNAL_DICTIONARY_HPP
