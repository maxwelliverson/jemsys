//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_MM_INTERNAL_HPP
#define JEMSYS_QUARTZ_MM_INTERNAL_HPP

#define JEM_SHARED_LIB_EXPORT
#include <quartz/core.h>

#include <memory>
#include <optional>

namespace qtz{

#if JEM_system_windows
  using native_handle_t = void*;
#else
  using native_handle_t = int;
#endif

  enum memory_access_t{
    no_access                = 0x01,
    readonly_access          = 0x02,
    readwrite_access         = 0x04,
    writecopy_access         = 0x08,
    execute_access           = 0x10,
    read_execute_access      = 0x20,
    readwrite_execute_access = 0x40,
    writecopy_execute_access = 0x80
  };
  enum process_permissions_t{
    can_terminate_process          = 0x00000001,
    can_create_thread              = 0x00000002,
    can_operate_on_process_memory  = 0x00000008,
    can_read_from_process_memory   = 0x00000010,
    can_write_to_process_memory    = 0x00000020,
    can_duplicate_handle           = 0x00000040,
    can_create_process             = 0x00000080,
    can_set_process_quota          = 0x00000100,
    can_set_process_info           = 0x00000200,
    can_query_process_info         = 0x00000400,
    can_suspend_process            = 0x00000800,
    can_query_limited_process_info = 0x00001000,
    can_delete_process             = 0x00010000,
    can_read_process_security      = 0x00020000,
    can_modify_process_security    = 0x00040000,
    can_change_process_owner       = 0x00080000,
    can_synchronize_with_process   = 0x00100000
  };

  using numa_node_t = jem_u32_t;

#define JEM_CURRENT_NUMA_NODE ((numa_node_t)-1)

  class file{
    inline constexpr static jem_size_t StorageSize  = 2 * sizeof(void*);
    inline constexpr static jem_size_t StorageAlign = alignof(void*);
  public:

    file();
    file(const file& other);
    file(file&& other) noexcept;

    file& operator=(const file& other);
    file& operator=(file&& other) noexcept;

    ~file();




    JEM_nodiscard native_handle_t get_native_handle() const noexcept {
      return *reinterpret_cast<const native_handle_t*>(storage + sizeof(void*));
    }

  private:
    alignas(StorageAlign) char storage[StorageSize] = {};
  };

  class file_mapping{
    friend class virtual_placeholder;
    friend class process;
  public:

  private:
    file       mapped_file;
    void*      handle;
    jem_size_t size;
  };
  struct anonymous_file_mapping{
    native_handle_t handle;
    jem_size_t      size;
  };
  class mapped_region{};
  class virtual_memory_region{};

  template <typename CharType>
  class environment;

  template <typename CharType, bool IsConst = false>
  class environment_variable_ref{

    template <typename>
    friend class environment;

    using env_ptr = std::conditional_t<IsConst, const environment<CharType>, environment<CharType>>*;
  public:
    using char_type = CharType;
    using environment_type = environment<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    environment_variable_ref(env_ptr env, jem_u32_t offset) noexcept
        : env(env),
          entryOffset(offset)
    {}

    environment_variable_ref& operator=(string_view_type newValue) noexcept {

    }


    friend bool operator==(environment_variable_ref a, environment_variable_ref b) noexcept {
      assert(a.env == b.env);
      return a.entryOffset == b.entryOffset;
    }

  private:
    env_ptr   env;
    jem_u32_t entryOffset;
  };

  template <typename CharType>
  class environment{

    template <typename, bool IsConst>
    friend class environment_variable_ref;

    template <bool IsConst>
    class iterator_;

    using word_type = jem_u32_t;

    inline constexpr static CharType ZeroChar = static_cast<CharType>(0x00);
    inline constexpr static CharType EqualsChar = static_cast<CharType>(0x3D);
#if JEM_system_windows
    inline constexpr static CharType SeparatorChar = static_cast<CharType>(0x3B);
#else
    inline constexpr static CharType SeparatorChar = static_cast<CharType>(0x3A);
#endif

  public:

    using char_type      = CharType;
    using string_view    = std::basic_string_view<char_type>;
    using variable_type  = environment_variable_ref<char_type>;
    using value_type     = variable_type;
    using iterator       = iterator_<false>;
    using const_iterator = iterator_<true>;
    class sentinel{
    public:
      template <bool IsConst>
      friend bool operator==(const iterator_<IsConst>& iter, sentinel) noexcept {
        return iter.offset == 0;
      }
    };
    using const_sentinel = sentinel;


    ~environment() {
      free(buffer.words);
    }




    JEM_nodiscard std::unique_ptr<char_type[]> get_normalized_buffer() const noexcept {
      auto* output = new char_type[get_required_normalized_size()];
      jem_u32_t  index = buffer.head->nextOffset;
      jem_size_t offset = 0;
      while ( index != 0 ) {
        const var_t& var = lookup_var(index);
        const jem_size_t varLength = var.entryStrLen + var.valueStrLen + 1;
        memcpy(output + offset, var.buffer, varLength);
        offset += varLength;
        output[offset++] = ZeroChar;
        index = var.nextOffset;
      }
      output[offset] = ZeroChar;
      if ( offset == 0 )
        output[1] = ZeroChar;
      return std::unique_ptr<char_type[]>(output);
    }



    JEM_nodiscard iterator        begin()       noexcept {
      return iterator(this, buffer.head->nextOffset);
    }
    JEM_nodiscard const_iterator  begin() const noexcept {
      return const_iterator(this, buffer.head->nextOffset);
    }
    JEM_nodiscard const_iterator cbegin() const noexcept {
      return this->begin();
    }

    JEM_nodiscard sentinel        end()       noexcept {
      return {};
    }
    JEM_nodiscard const_sentinel  end() const noexcept {
      return {};
    }
    JEM_nodiscard const_sentinel cend() const noexcept {
      return {};
    }

  private:

    template <bool IsConst>
    class iterator_{
      template <bool>
      friend class iterator_;
      friend class sentinel;
      using env_ptr    = std::conditional_t<IsConst, const environment, environment>*;
    public:
      using value_type = environment_variable_ref<char_type, IsConst>;
      using reference  = std::conditional_t<IsConst, const value_type, value_type>&;

      explicit iterator_(env_ptr env, jem_u32_t index) noexcept
          : env(env),
            offset(index),
            mustCache(false),
            cachedVar(env, index)
      {}
      iterator_(const iterator_<false>& other) noexcept
          : iterator_(other.env, other.offset){}

      reference operator*() const noexcept {
        if ( mustCache ) {
          cachedVar.entryOffset = offset;
          mustCache = false;
        }
        return cachedVar;
      }

      iterator_& operator++()    noexcept {
        offset = env->lookup_var(offset).nextOffset;
        mustCache = true;
        return *this;
      }
      iterator_  operator++(int) noexcept {
        iterator_ copy = *this;
        ++*this;
        return copy;
      }
      iterator_& operator--()    noexcept {
        offset = env->lookup_var(offset).prevOffset;
        mustCache = true;
        return *this;
      }
      iterator_  operator--(int) noexcept {
        iterator_ copy = *this;
        --*this;
        return copy;
      }


      friend bool operator==(const iterator_& a, const iterator_& b) noexcept {
        assert(a.env == b.env);
        return a.offset == b.offset;
      }


    private:
      env_ptr            env;
      jem_u32_t          offset;
      mutable bool       mustCache = true;
      mutable value_type cachedVar;
    };

    struct var_t {
      jem_u32_t prevOffset;
      jem_u32_t nextOffset;
      jem_u16_t entryStrLen;
      jem_u16_t valueStrLen;
      char_type buffer[];
    };



    void resort_variables() noexcept;


    JEM_nodiscard jem_size_t get_required_normalized_size() const noexcept {
      jem_size_t reqSize = 1;
      jem_u32_t  index = buffer.head->nextOffset;
      while ( index != 0 ) {
        const var_t& var = lookup_var(index);
        reqSize += (var.entryStrLen + var.valueStrLen + 2);
        index = var.nextOffset;
      }
      if ( reqSize == 1 )
        return 2;
      return reqSize;
    }


    JEM_nodiscard std::pair<jem_u32_t, bool> find_variable_index_from(jem_u32_t index, string_view entry) const noexcept {
      const var_t& initialVar = lookup_var(index);
      const auto initialResult = entry <=> string_view(get_entry(initialVar), initialVar.entryStrLen);
      if ( initialResult == std::strong_ordering::less ) {
        index = initialVar.nextOffset;
        while ( index != 0 ) {
          const var_t& var = lookup_var(index);
          const auto result = entry <=> string_view(get_entry(var), var.entryStrLen);
          if ( result == std::strong_ordering::equal )
            return { index, true };
          else if ( result == std::strong_ordering::greater )
            return { var.prevOffset, false };
          index = var.nextOffset;
        }
        return { buffer.head->prevOffset, false };
      }
      else if ( initialResult == std::strong_ordering::greater ) {
        index = initialVar.prevOffset;
        while ( index != 0 ) {
          const var_t& var = lookup_var(index);
          const auto result = entry <=> string_view(get_entry(var), var.entryStrLen);
          if ( result == std::strong_ordering::equal )
            return { index, true };
          else if ( result == std::strong_ordering::less )
            return { var.nextOffset, false };
          index = var.prevOffset;
        }
        return { buffer.head->nextOffset, false };
      }
      else {
        return { index, true };
      }
    }
    JEM_nodiscard std::pair<jem_u32_t, bool> find_variable_index(string_view entry) const noexcept {
      jem_u32_t index = buffer.head->nextOffset;

      while ( index != 0 ) {
        const var_t& var = lookup_var(index);
        const auto result = entry <=> string_view(get_entry(var), var.entryStrLen);
        if ( result == std::strong_ordering::equal )
          return { index, true };
        else if ( result == std::strong_ordering::greater )
          return { var.prevOffset, false };
        index = var.nextOffset;
      }
      return { buffer.head->prevOffset, false };
    }

    JEM_nodiscard bool check_valid_index(jem_u32_t index) const noexcept {
      return index < (bufferLength / alignof(word_type));
    }
    var_t&             lookup_var(jem_u32_t index) noexcept {
      return reinterpret_cast<var_t&>(buffer.words[index]);
    }
    const var_t&       lookup_var(jem_u32_t index) const noexcept {
      return reinterpret_cast<const var_t&>(buffer.words[index]);
    }

    void remove_variable(jem_u32_t index) noexcept {
      remove_variable(lookup_var(index));
    }
    void remove_variable(var_t& var) noexcept {
      lookup_var(var.prevOffset).nextOffset = var.nextOffset;
      lookup_var(var.nextOffset).prevOffset = var.prevOffset;
    }
    void insert_variable_after(jem_u32_t position, jem_u32_t index) noexcept {
      var_t& var = lookup_var(index);
      var_t& prev  = lookup_var(position);
      var.prevOffset = position;
      var.nextOffset = prev.nextOffset;
      lookup_var(prev.nextOffset).prevOffset = index;
      prev.nextOffset  = index;
    }
    void insert_variable_before(jem_u32_t position, jem_u32_t index) noexcept {
      var_t& var = lookup_var(index);
      var_t& next  = lookup_var(position);
      var.nextOffset = position;
      var.prevOffset = next.prevOffset;
      lookup_var(next.prevOffset).nextOffset = index;
      next.prevOffset  = index;
    }

    void move_variable_after(jem_u32_t position, jem_u32_t index) noexcept {
      remove_variable(index);
      insert_variable_after(position, index);
    }
    void move_variable_before(jem_u32_t position, jem_u32_t index) noexcept {}


    void swap_out_variable(jem_u32_t oldIndex, jem_u32_t newIndex) noexcept {
      var_t& oldVar = lookup_var(oldIndex);
      var_t& newVar = lookup_var(newIndex);
      newVar.prevOffset = oldVar.prevOffset;
      newVar.nextOffset = oldVar.nextOffset;
      lookup_var(oldVar.prevOffset).nextOffset = newIndex;
      lookup_var(oldVar.nextOffset).prevOffset = newIndex;
    }


    void reserve(jem_size_t size) noexcept {
      if ( size <= bufferCapacity )
        return;
      bufferCapacity = ((size - 1) | (alignof(word_type) - 1)) + 1;
      buffer.words = static_cast<word_type*>(realloc(buffer.words, bufferCapacity));
    }
    void grow_by(jem_size_t size) noexcept {
      bufferLength += size;
      if ( bufferCapacity < bufferLength ) {
        reserve(bufferLength * 3 / 2);
      }
    }

    JEM_nodiscard JEM_forceinline static jem_size_t required_growth_size(jem_u32_t entryLength, jem_u32_t valueLength) noexcept {
      constexpr static jem_size_t AlignMask = alignof(word_type) - 1;
      const jem_size_t charsReq = entryLength + valueLength;
      return sizeof(var_t) + (charsReq | AlignMask) + 1;
    }

    JEM_nodiscard JEM_forceinline static char_type* get_entry(const var_t& var) noexcept {
      return var.buffer;
    }
    JEM_nodiscard JEM_forceinline static char_type* get_value(const var_t& var) noexcept {
      return var.buffer + var.entryStrLen + 1;
    }

    jem_u32_t allocate_new_variable(jem_u32_t entryLength, jem_u32_t valueLength) noexcept {
      const jem_u32_t index = bufferLength / alignof(word_type);
      grow_by(required_growth_size(entryLength, valueLength));
      return index;
    }


    jem_u32_t modify_variable_value(jem_u32_t index, string_view newValue) noexcept {
      var_t& var = lookup_var(index);
      if ( newValue.length() <= var.valueStrLen ) {
        memmove(get_value(var), newValue.data(), newValue.size() * sizeof(char_type));
        var.valueStrLen = jem_u16_t(newValue.size());
        return index;
      }
      const jem_u32_t newIndex = allocate_new_variable(var.entryStrLen, newValue.size());
      var_t& newVar = lookup_var(newIndex);
      newVar.valueStrLen = jem_u16_t(newValue.size());
      newVar.entryStrLen = var.entryStrLen;
      memcpy(get_entry(newVar), get_entry(var), (var.entryStrLen + 1) * sizeof(char_type));
      memcpy(get_value(newVar), newValue.data(), newValue.size() * sizeof(char_type));
      swap_out_variable(index, newIndex);
      return newIndex;
    }
    jem_u32_t modify_variable_entry(jem_u32_t index, string_view newEntry, jem_u32_t newPosition) noexcept {
      var_t& var = lookup_var(index);
      char_type* const oldValueStr = get_value(var);

      if ( newEntry.length() <= var.entryStrLen ) {
        memmove(get_entry(var), newEntry.data(), newEntry.size() * sizeof(char_type));
        var.buffer[newEntry.size()] = EqualsChar;
        var.entryStrLen = jem_u16_t(newEntry.size());
        memmove(get_value(var), oldValueStr, var.valueStrLen);
        move_variable_after(newPosition, index);
        return index;
      }

      const jem_u32_t newIndex = allocate_new_variable(newEntry.size(), var.valueStrLen);
      var_t& newVar = lookup_var(newIndex);
      newVar.entryStrLen = jem_u16_t(newEntry.size());
      newVar.valueStrLen = var.valueStrLen;
      memcpy(get_entry(newVar), newEntry.data(), newEntry.size() * sizeof(char_type));
      newVar.buffer[newEntry.size()] = EqualsChar;
      memcpy(get_value(newVar), get_value(var), newVar.valueStrLen * sizeof(char_type));
      remove_variable(var);
      insert_variable_after(newPosition, newVar);
      return newIndex;
    }
    jem_u32_t append_variable_value(jem_u32_t index, string_view newValue) noexcept {
      var_t& var = lookup_var(index);
      const jem_u32_t newValueSize = newValue.size() + var.valueStrLen + 1;
      const jem_u32_t newIndex = allocate_new_variable(var.entryStrLen, newValueSize);
      var_t& newVar = lookup_var(newIndex);
      newVar.valueStrLen = jem_u16_t(newValueSize);
      newVar.entryStrLen = var.entryStrLen;
      memcpy(get_entry(newVar), get_entry(var), (var.entryStrLen + 1) * sizeof(char_type));
      memcpy(get_value(newVar), get_value(var), var.valueStrLen * sizeof(char_type));
      get_value(newVar)[var.valueStrLen] = SeparatorChar;
      memcpy(get_value(newVar) + var.valueStrLen + 1, newValue.data(), newValue.size() * sizeof(char_type));
      swap_out_variable(index, newIndex);
      return newIndex;
    }
    jem_u32_t insert_new_variable(jem_u32_t afterIndex, string_view entry, std::span<const string_view> values) noexcept {
      assert( !values.empty() );

      // Sum total required size for the combined value string
      auto valueStrLen = static_cast<jem_size_t>(-1);
      for ( string_view val : values )
        valueStrLen += val.size() + 1;
      const jem_u32_t newIndex = allocate_new_variable(entry.size(), valueStrLen);



      var_t& newVar = lookup_var(newIndex);
      newVar.entryStrLen = entry.size();
      newVar.valueStrLen = valueStrLen;

      // Copy entry and equals sign
      memcpy(get_entry(newVar), entry.data(), entry.size() * sizeof(word_type));
      get_entry(newVar)[entry.size()] = EqualsChar;

      // Copy values and insert separators
      char_type* valuePos = get_value(newVar);
      const string_view*       valueBegin = values.begin();
      const string_view* const valueEnd   = values.end();
      memcpy(valuePos, valueBegin->data(), valueBegin->size());
      valuePos += valueBegin->size();
      while ( ++valueBegin != valueEnd ) {
        *valuePos++ = SeparatorChar;
        memcpy(valuePos, valueBegin->data(), valueBegin->size());
        valuePos += valueBegin->size();
      }

      insert_variable_after(afterIndex, newIndex);
      return newIndex;
    }


    jem_u32_t bufferLength;
    jem_u32_t bufferCapacity;
    jem_u32_t entryCount;
    union{
      word_type* words;
      var_t*   head;
    } buffer;
  };

  class process {
  public:

    struct id{
      friend bool operator==(id a, id b) noexcept = default;
      friend std::strong_ordering operator<=>(id a, id b) noexcept = default;
    private:
      friend class process;

      explicit id(jem_u32_t value) noexcept : id_(value){}

      jem_u32_t id_;
    };


    static std::optional<process> create(const char* applicationName, std::string_view commandLineOpts, void* procSecurityDesc, void* threadSecurityDesc, bool inherit, const environment<char>& env) noexcept;
    static std::optional<process> create(const char* applicationName, std::string_view commandLineOpts, const environment<wchar_t>& env) noexcept;
    static std::optional<process> open(id processId, process_permissions_t permissions) noexcept;


    JEM_nodiscard void* reserve_memory(jem_size_t size, jem_size_t alignment = JEM_DONT_CARE) const noexcept;
    bool  release_memory(void* placeholder) const noexcept;

    JEM_nodiscard void* local_alloc(void* address, jem_size_t size, memory_access_t access = readwrite_access, numa_node_t numaNode = JEM_CURRENT_NUMA_NODE) const noexcept;
    bool  local_free(void* allocation, jem_size_t size) const noexcept;

    JEM_nodiscard void* map(void* address, const file_mapping& fileMapping, memory_access_t = readwrite_access, numa_node_t numaNode = JEM_CURRENT_NUMA_NODE) const noexcept;
    bool  unmap(void* address) const noexcept;

    bool  split(void* placeholder, jem_size_t firstSize) const  noexcept;
    bool  coalesce(void* placeholder, jem_size_t totalSize) const  noexcept;

    JEM_nodiscard bool is_valid() const noexcept {
      return handle_;
    }

  private:
    native_handle_t       handle_;
    id                    id_;
    process_permissions_t permissions_;
  };


  extern process& this_process;






  class virtual_placeholder{
  public:





    static void* reserve(jem_size_t size, jem_size_t alignment = JEM_DONT_CARE) noexcept;
    static bool  release(void* placeholder) noexcept;

    static void* alloc(void* placeholder, jem_size_t size, memory_access_t access = readwrite_access, numa_node_t numaNode = JEM_CURRENT_NUMA_NODE) noexcept;
    static bool  free(void* allocation, jem_size_t size) noexcept;

    static void* map(void* placeholder, const file_mapping& fileMapping, memory_access_t = readwrite_access, numa_node_t numaNode = JEM_CURRENT_NUMA_NODE) noexcept;
    static bool  unmap(void* placeholder) noexcept;

    static bool  split(void* placeholder, jem_size_t firstSize) noexcept;
    static bool  coalesce(void* placeholder, jem_size_t totalSize) noexcept;
  };




  class extensible_segment {
    inline constexpr static jem_size_t MagicNumber        = 0xFA81'1B0C'CAC0'182C;
    friend class extensible_segment_ref;
    inline constexpr static jem_size_t MaxChunkNameLength = 256;
  public:

    extensible_segment(jem_address_t address);

  private:
    struct header_t{
      jem_u64_t              magic;
      anonymous_file_mapping nextMapping;
      anonymous_file_mapping mapping;
      jem_size_t             chunkSize; // Multiple of alignment
      jem_size_t             alignment;
      jem_size_t             maxCapacity;
      jem_size_t             currentCapacity;
      jem_size_t             cursorPos;
      char                   name[MaxChunkNameLength];
      jem_u8_t               buffer[];
    };
    struct subheader_t{
      anonymous_file_mapping nextMapping;
      jem_u8_t               buffer[];
    };

    header_t*  header;    // Address is aligned to alignment
  };

  class extensible_segment_ref {
  public:
    extensible_segment_ref(const extensible_segment& segment) noexcept
        : initialSegmentFileMapping(segment.header->mapping),
          maximumSize(segment.header->maxCapacity)
    {}
  private:
    anonymous_file_mapping initialSegmentFileMapping;
    jem_size_t             maximumSize;
  };
}

#endif//JEMSYS_QUARTZ_MM_INTERNAL_HPP
