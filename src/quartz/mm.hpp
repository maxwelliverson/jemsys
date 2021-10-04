//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_MM_INTERNAL_HPP
#define JEMSYS_QUARTZ_MM_INTERNAL_HPP

#define JEM_SHARED_LIB_EXPORT
#include <quartz.h>

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


    static std::optional<process> create(const char* applicationName, std::string_view commandLineOpts, void* procSecurityDesc, void* threadSecurityDesc, bool inherit/*, const environment<char>& env*/) noexcept;
    static std::optional<process> create(const char* applicationName, std::string_view commandLineOpts/*, const environment<wchar_t>& env*/) noexcept;
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


}

#endif//JEMSYS_QUARTZ_MM_INTERNAL_HPP
