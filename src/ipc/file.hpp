//
// Created by Maxwell on 2021-06-14.
//

#ifndef JEMSYS_IPC_FILE_INTERNAL_HPP
#define JEMSYS_IPC_FILE_INTERNAL_HPP

#include <jemsys.h>

namespace ipc{
#if JEM_system_windows
  using native_handle_t = void*;
#else
  using native_handle_t = int;
#endif

  using numa_node_t = jem_u32_t;

#define JEM_CURRENT_NUMA_NODE ((numa_node_t)-1)


  /*class file{
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
  };*/

  struct file{
    native_handle_t handle;
  };

  struct file_mapping{
    file            mappedFile;
    native_handle_t handle;
    jem_size_t      size;
  };
  struct anonymous_mapping{
    native_handle_t handle;
    jem_size_t      size;
  };
}

#endif//JEMSYS_IPC_FILE_INTERNAL_HPP
