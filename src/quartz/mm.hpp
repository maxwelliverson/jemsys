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
  using native_file_handle_t = void*;
#else
  using native_file_handle_t = int;
#endif

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




    JEM_nodiscard native_file_handle_t get_native_handle() const noexcept {
      return *reinterpret_cast<const native_file_handle_t*>(storage + sizeof(void*));
    }

  private:
    alignas(StorageAlign) char storage[StorageSize] = {};
  };

  class file_mapping{
  public:

  private:
    file       mapped_file;
    void*      handle;
    jem_size_t size;
  };
  struct anonymous_file_mapping{
    native_file_handle_t handle;
    jem_size_t           size;
  };
  class mapped_region{};
  class virtual_memory_region{};

  class extensible_segment {
    friend class extensible_segment_ref;
    inline constexpr static jem_size_t MaxChunkNameLength = 256;
  public:

    extensible_segment(jem_address_t address);

  private:
    struct header_t{
      jem_u64_t              magic;
      anonymous_file_mapping nextMapping;
      anonymous_file_mapping mapping;
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
    jem_size_t chunkSize; // Multiple of alignment
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
