//
// Created by maxwell on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_INTERNAL_H
#define JEMSYS_QUARTZ_INTERNAL_H

#define JEM_SHARED_LIB_EXPORT

#include <quartz.h>


#include <atomic>
#include <semaphore>
#include <span>
#include <array>
#include <ranges>
#include <mutex>
#include <shared_mutex>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <vector>
#include <charconv>


#include "atomicutils.hpp"
#include "dictionary.hpp"
#include "handles.hpp"





/*using atomic_u32_t  = std::atomic_uint32_t;
using atomic_u64_t  = std::atomic_uint64_t;
using atomic_size_t = std::atomic_size_t;
using atomic_flag_t = std::atomic_flag;

using jem_semaphore_t        = std::counting_semaphore<>;
using jem_binary_semaphore_t = std::binary_semaphore;*/

typedef jem_u64_t jem_bitmap_field_t;


#define JEM_BITS_PER_BITMAP_FIELD (sizeof(jem_bitmap_field_t) * 8)
#define JEM_VIRTUAL_REGION_SIZE (JEM_VIRTUAL_PAGE_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_VIRTUAL_SEGMENT_SIZE (JEM_VIRTUAL_REGION_SIZE * JEM_BITS_PER_BITMAP_FIELD)
#define JEM_GLOBAL_VIRTUAL_MEMORY_MAX (JEM_VIRTUAL_SEGMENT_SIZE * JEM_BITS_PER_BITMAP_FIELD) // 16GB


#define QTZ_REQUEST_SIZE 512
#define QTZ_REQUEST_PAYLOAD_SIZE QTZ_REQUEST_SIZE - JEM_CACHE_LINE

#define QTZ_NAME_MAX_LENGTH (2*JEM_CACHE_LINE)

#if JEM_system_windows
using qtz_exit_code_t = unsigned long;
using qtz_pid_t = unsigned long;
using qtz_tid_t = unsigned long;
using qtz_handle_t = void*;
#else
using qtz_exit_code_t = int;
using qtz_pid_t       = int;
using qtz_tid_t       = int;
using qtz_handle_t    = int;
#endif



/*typedef struct jem_global_message{
  jem_u32_t nextIndex;

} jem_global_message, * jem_global_message_t;*/


namespace qtz{




  struct handle_descriptor;

  using handle_info_t = typename jem::dictionary<handle_descriptor>::entry_type*;

  // TODO: Expand utility of handle_descriptor
  // TODO: Create handle ownership model?
  // TODO: Create handle lifetime model

  /*
   * Every handle has an owner
   * There exists a root handle
   * All handles created without explicit owner are owned by the root handle
   * A handle X can "require" a handle Y
   * A handle X can "borrow" a handle Y
   *
   * A handle that enters an invalid state will notify every handle that has
   * borrowed or required it.
   *
   *
   * */
  struct handle_descriptor{
    void* address;

    bool  isShared;

  };


  /*class memory_placeholder{
    void*  m_address;
    size_t m_size;
  public:
    memory_placeholder();
    memory_placeholder(size_t size) noexcept;
    memory_placeholder(void* addr, size_t size) noexcept : m_address(addr), m_size(size) { }
    ~memory_placeholder();

    inline size_t size() const noexcept {
      return m_size;
    }

    inline bool is_valid() const noexcept {
      return m_address != nullptr;
    }
    inline explicit operator bool() const noexcept {
      return is_valid();
    }

    memory_placeholder split() noexcept;
    memory_placeholder split(size_t size) noexcept;

    bool               coalesce() noexcept;
    bool               coalesce(memory_placeholder& other) noexcept;

    void*              commit() noexcept;
    void*              map(qtz_handle_t handle) noexcept;

    static memory_placeholder release(void* addr, size_t size) noexcept;
    static memory_placeholder unmap(void* addr, size_t size) noexcept;
  };

  class virtual_allocator{

    void*      addressSpaceInitial;
    jem_size_t addressSpaceSize;

  protected:
    inline void*      addr() const noexcept {
      return addressSpaceInitial;
    }
    inline jem_size_t size() const noexcept {
      return addressSpaceSize;
    }

  public:

    virtual ~virtual_allocator();

    virtual void* reserve(jem_size_t size) noexcept = 0;
    virtual void* reserve(jem_size_t size, jem_size_t alignment, jem_size_t offset) noexcept = 0;



    virtual void  release(void* address, jem_size_t size) noexcept = 0;

  };*/
}




enum {
  QTZ_MESSAGE_IS_READY      = 0x1,
  QTZ_MESSAGE_IS_DISCARDED  = 0x2,
  QTZ_MESSAGE_NOT_PRESERVED = 0x4,
  QTZ_MESSAGE_IS_FINALIZED  = 0x8,
  QTZ_MESSAGE_ALL_FLAGS     = 0xF,
  QTZ_MESSAGE_PRESERVE      = (QTZ_MESSAGE_ALL_FLAGS ^ QTZ_MESSAGE_NOT_PRESERVED)
};

typedef enum {
  GLOBAL_MESSAGE_KIND_NOOP                         = 0,
  GLOBAL_MESSAGE_KIND_KILL                         = 1,
  GLOBAL_MESSAGE_KIND_2                            = 2,
  GLOBAL_MESSAGE_KIND_ALLOCATE_MAILBOX             = 3,
  GLOBAL_MESSAGE_KIND_FREE_MAILBOX                 = 4,
  GLOBAL_MESSAGE_KIND_3                            = 5,
  GLOBAL_MESSAGE_KIND_4                            = 6,
  GLOBAL_MESSAGE_KIND_5                            = 7,
  GLOBAL_MESSAGE_KIND_6                            = 8,
  GLOBAL_MESSAGE_KIND_7                            = 9,
  GLOBAL_MESSAGE_KIND_8                            = 10,
  GLOBAL_MESSAGE_KIND_9                            = 11,
  GLOBAL_MESSAGE_KIND_10                           = 12,
  GLOBAL_MESSAGE_KIND_11                           = 13,
  GLOBAL_MESSAGE_KIND_REGISTER_OBJECT              = 14,
  GLOBAL_MESSAGE_KIND_UNREGISTER_OBJECT            = 15,
  GLOBAL_MESSAGE_KIND_LINK_OBJECTS                 = 16,
  GLOBAL_MESSAGE_KIND_UNLINK_OBJECTS               = 17,
  GLOBAL_MESSAGE_KIND_OPEN_OBJECT_HANDLE           = 18,
  GLOBAL_MESSAGE_KIND_DESTROY_OBJECT               = 19,
  GLOBAL_MESSAGE_KIND_LOG_MESSAGE                  = 20,
  GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK             = 21,
  GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK_WITH_BUFFER = 22
} qtz_message_kind_t;
typedef enum {
  QTZ_ACTION_DISCARD,
  QTZ_ACTION_DEFERRED,
  QTZ_ACTION_NOTIFY_LISTENER
} qtz_message_action_t;

namespace qtz {
  struct kill_request {
    size_t          structLength;
    qtz_exit_code_t exitCode;
  };
  struct pause_request {
    size_t structSize;
  };
  struct alloc_mailbox_request{
    size_t     structSize;
    void**     handle;
    jem_size_t slotSize;
    jem_size_t slotCount;
    bool       isShared;
    char       name[QTZ_NAME_MAX_LENGTH];
  };
  struct free_mailbox_request{
    size_t     structSize;
    void*      handle;
    void*      slotsAddress;
    jem_size_t slotSize;
    jem_size_t slotCount;
    char       name[QTZ_NAME_MAX_LENGTH];
  };
  struct log_message_request {
    size_t    structLength;
    // jem_u64_t timestamp;
    char      message[];
  };
  struct execute_callback_request{
    size_t structLength;
    void(* callback)(void*);
    void*  userData;
  };
  struct execute_callback_with_buffer_request{
    size_t structLength;
    void(*callback)(void*);
    char  userData[];
  };

  class async_buffered_output_log {
    struct buffer{
      buffer*       next;
      size_t        bytesToWrite;
      char*         data;
    };
    /*struct shared_data{
      std::atomic<buffer*> poolHead;
      mpsc_counter_t       queuedBuffers;
      semaphore_t          bufferTokens;
      binary_semaphore_t   isKill;

      buffer*              currentBuffer;
    };*/

    size_t               bufferSize;
    size_t               totalBufferCount;
    buffer*              bufferArray;
    std::counting_semaphore<> poolTokens;
    mpsc_counter_t       queuedBuffers;
    buffer*              currentBuffer;
    std::atomic<buffer*> bufferListHead;
    buffer*              queueTail;
    std::jthread         asyncThread;
    char*                currentCursor;
    char*                currentBufferEnd;
    // shared_data*         shared;

    inline static constexpr size_t DefaultBufferSize  = 256;
    inline static constexpr size_t DefaultBufferCount = 4;

    void push_to_queue() noexcept {

      currentBuffer->bytesToWrite = currentCursor - currentBuffer->data;

      buffer* oldHead   = acquire_from_pool();
      buffer* oldBuffer = std::exchange(currentBuffer, oldHead);
      currentCursor     = oldHead->data;
      currentBufferEnd  = currentCursor + bufferSize;

      queueTail->next = oldBuffer;
      queueTail = oldBuffer;
      queuedBuffers.increase(1);
    }

    void pop_from_queue(buffer*& prev) noexcept {
      queuedBuffers.decrease(1);
      auto result = prev->next;
      release_to_pool(prev);
      prev = result;
    }
    bool try_pop_from_queue(buffer*& prev) noexcept {
      if (!queuedBuffers.try_decrease(1))
        return false;
      auto result = prev->next;
      release_to_pool(prev);
      prev = result;
      return true;
    }
    bool try_pop_from_queue_for(buffer*& prev, jem_u64_t us_timeout) noexcept {
      if (!queuedBuffers.try_decrease_for(1, us_timeout))
        return false;
      auto result = prev->next;
      release_to_pool(prev);
      prev = result;
      return true;
    }

    buffer* acquire_from_pool() noexcept {
      poolTokens.acquire();

      buffer* oldHead = bufferListHead.load();
      assert( oldHead != nullptr );


      buffer* nextBuffer;
      do {
        nextBuffer = oldHead->next;
      } while( !bufferListHead.compare_exchange_weak(oldHead, nextBuffer) );

      return oldHead;
    }
    void    release_to_pool(buffer* buf) noexcept {
      buffer* currListHead = bufferListHead.load();

      do {
        buf->next = currListHead;
      } while( !bufferListHead.compare_exchange_weak(currListHead, buf) );

      poolTokens.release();
    }




    /*void thread_proc() noexcept {
      DWORD handleCount     = (DWORD)totalBufferCount;
      DWORD bufSize         = (DWORD)bufferSize;
      size_t bufCount       = totalBufferCount;
      HANDLE cout           = GetStdHandle(STD_OUTPUT_HANDLE);
      buffer* bufArray      = bufferArray;
      const HANDLE* handles = eventHandles;
      binary_semaphore_t* killSwitch = this->isKill;
      bool isPostMortem     = false;
      std::vector<size_t> signalledIndices;

      signalledIndices.reserve(bufCount);

      while ( !isPostMortem ) {

        auto waitResult = WaitForMultipleObjects(handleCount, handles, false, 100);
        if ( waitResult == WAIT_TIMEOUT ) {
          if ( killSwitch->try_acquire() ) {
            isPostMortem = true;
            waitResult = WaitForMultipleObjects(handleCount, handles, false, 0);
            if ( waitResult == WAIT_TIMEOUT )
              continue;
          }
          else
            continue;
        }
        signalledIndices.push_back(waitResult - WAIT_OBJECT_0);
        for (;;) {
          auto result = WaitForMultipleObjects(handleCount, handles, false, 0);
          if ( result == WAIT_TIMEOUT )
            break;
          signalledIndices.push_back(result - WAIT_OBJECT_0);
        }

        for ( size_t index : signalledIndices ) {
          buffer* signalledBuffer = bufArray + index;
          DWORD   totalBytes      = signalledBuffer->bytesToWrite;
          DWORD   numberOfBytesWritten = 0;
          while (!WriteFile(cout, signalledBuffer->data + numberOfBytesWritten, totalBytes, &numberOfBytesWritten, nullptr)) {
            totalBytes -= numberOfBytesWritten;
          }
          push_clean_buffer(signalledBuffer);
        }

        signalledIndices.clear();

      }

      delete killSwitch;

      for ( buffer& buf : std::span{ bufArray, bufCount}) {
        delete[] buf.data;
        CloseHandle(buf.eventHandle);
      }
      delete[] bufArray;
      delete[] handles;
    }*/

    static void print_buffer(void* handle, buffer* buf) noexcept {
      unsigned long numberOfBytesWritten = 0;
      unsigned long totalBytes = buf->bytesToWrite;
      while (!WriteFile(handle, buf->data + numberOfBytesWritten, totalBytes, &numberOfBytesWritten, nullptr)) {
        totalBytes -= numberOfBytesWritten;
      }
    }

    static void thread_proc(std::stop_token self, async_buffered_output_log& log, buffer* buffers) noexcept {
      buffer* buf          = buffers;
      HANDLE  StdOut       = GetStdHandle(STD_OUTPUT_HANDLE);
      bool    isPostMortem = false;


      while ( !isPostMortem ) {
        if ( !log.try_pop_from_queue_for(buf, 100'000) ) {
          if ( self.stop_requested() )
            isPostMortem = true;
          continue;
        }
        print_buffer(StdOut, buf);
      }
      while (log.try_pop_from_queue(buf)) {
        print_buffer(StdOut, buf);
      }

      for ( buffer& b : std::span{ log.bufferArray, log.totalBufferCount })
        delete[] b.data;
      delete[] log.bufferArray;
    }

    void initialize() noexcept {

      bufferArray      = new buffer[totalBufferCount];

      buffer* lastBuffer = nullptr;

      for ( size_t i = totalBufferCount - 1; i > 0; --i ) {

        buffer* thisBuf = bufferArray + i;

        thisBuf->next = lastBuffer;
        thisBuf->data = new char[bufferSize];
        thisBuf->bytesToWrite = bufferSize;
        lastBuffer = thisBuf;
      }

      currentBuffer    = lastBuffer;
      currentCursor    = currentBuffer->data;
      currentBufferEnd = currentCursor + bufferSize;

      bufferListHead.store(lastBuffer->next);


      bufferArray->data = new char[bufferSize];
      queueTail = bufferArray;

      asyncThread = std::jthread{
        &async_buffered_output_log::thread_proc,
        std::ref(*this),
        bufferArray
      };
    }

    static const void* get_size_next_line(const void* bytes, size_t& maxSize) noexcept {
      const void* pos = memchr(bytes, '\n', maxSize);
      if ( pos != nullptr ) {
        maxSize = (const char*)pos - (const char*)bytes;
        return (const char*)pos + 1;
      }
      return (const char*)bytes + maxSize;
    }

    template <typename T>
    void write_number(T value) noexcept {
      auto&& [newCursor, errorCode] = std::to_chars(currentCursor, currentBufferEnd, value);
      if ( errorCode == std::errc::value_too_large ) [[unlikely]] {
        assert( newCursor == currentBufferEnd );
        push_to_queue();
        write_number(value); // tail call
      }
      else
        currentCursor = newCursor;

      assert( currentCursor <= currentBufferEnd );
    }
    void write_bytes(const void* bytes, size_t byteCount) noexcept {

      /*size_t remainingAvailableSize = currentBufferEnd - currentCursor;

      const void* newLinePos

      size_t remainingSize = byteCount;
      size_t sizeToWrite = remainingAvailableSize;
      const char* bytePos = (const char*)get_size_next_line(bytes, sizeToWrite);
      std::memcpy(currentCursor, bytes, sizeToWrite);
      remainingSize -= sizeToWrite;
      while (remainingSize > 0) {
        push_to_queue();
        sizeToWrite = std::min(remainingSize, bufferSize);
        const char* nextPos = (const char*)get_size_next_line(bytePos, sizeToWrite);
        std::memcpy(currentCursor, bytePos, sizeToWrite);
        bytePos = nextPos;
        remainingSize -= sizeToWrite;
        currentCursor += sizeToWrite;
      }
      *//*if ( byteCount <= remainingAvailableSize ) {
        const char* bytePos = (const char*)get_size_next_line(bytes, sizeToWrite);
        std::memcpy(currentCursor, bytes, byteCount);
        currentCursor += byteCount;
      }
      else {

      }*//*
      assert( currentCursor <= currentBufferEnd );*/

      std::memcpy(currentCursor, bytes, byteCount);
      currentCursor += byteCount;
      assert( currentCursor <= currentBufferEnd );
    }
    void write_nt_string(const char* string) noexcept {
      write_bytes(string, std::strlen(string));
      assert( currentCursor <= currentBufferEnd );
      /*int r = strcpy_s(currentCursor, currentBufferEnd - currentCursor, string);
      if ( r != 0 ) {
        size_t remainingSize = strlen(string);
        const char* stringPos = string;
        do {
          cycle_next_buffer();
          size_t sizeToWrite = std::min(remainingSize, bufferSize);
          remainingSize -= sizeToWrite;
          std::memcpy(currentCursor, stringPos, sizeToWrite);
          stringPos += sizeToWrite;
        } while( remainingSize > 0 );
      }*/
    }
    void write_char(char c) noexcept {
      if ( c == '\n' ) {
        push_to_queue();
        return;
      }

      if ( currentCursor == currentBufferEnd ) [[unlikely]]
        push_to_queue();
      *currentCursor++ = c;
      assert( currentCursor <= currentBufferEnd );
    }

    template <typename T>
    void write_hex_number(T value) noexcept {
      auto&& [newCursor, errorCode] = std::to_chars(currentCursor, currentBufferEnd, value, 16);
      if ( errorCode == std::errc::value_too_large ) [[unlikely]] {
        assert( newCursor == currentBufferEnd );
        push_to_queue();
        write_hex_number(value); // tail call
      }
      else
        currentCursor = newCursor;

      assert( currentCursor <= currentBufferEnd );
    }

  public:

    async_buffered_output_log() noexcept
        : async_buffered_output_log(DefaultBufferSize, DefaultBufferCount){}
    async_buffered_output_log(size_t bufSize, size_t bufCount) noexcept
        : bufferSize(bufSize),
          totalBufferCount(bufCount),
          bufferArray(nullptr),
          poolTokens((ptrdiff_t)bufCount - 2),
          currentBuffer(nullptr),
          currentCursor(nullptr),
          currentBufferEnd(nullptr)
    {
      assert(bufCount > 2);
      initialize();
    }

    ~async_buffered_output_log() {
      push_to_queue();
    }


    async_buffered_output_log& write(std::chrono::microseconds us) noexcept {
      write_number(us.count());
      return this->write("us");
    }
    /*async_buffered_output_log& write(std::span<std::byte> bytes) noexcept {
      write_bytes(bytes.data(), bytes.size());
      return *this;
    }
    async_buffered_output_log& write(std::span<const std::byte> bytes) noexcept {
      write_bytes(bytes.data(), bytes.size());
      return *this;
    }*/
    async_buffered_output_log& write(std::string_view sv) noexcept {
      write_bytes(sv.data(), sv.size());
      return *this;
    }
    async_buffered_output_log& write(char c) noexcept {
      write_char(c);
      return *this;
    }
    template <size_t N>
    async_buffered_output_log& write(const char (&str)[N]) noexcept {
      write_bytes(str, N - 1);
      return *this;
    }
    template <std::integral T>
    async_buffered_output_log& write(T value) noexcept {
      write_number(value);
      return *this;
    }
    template <std::floating_point T>
    async_buffered_output_log& write(T value) noexcept {
      write_number(value);
      return *this;
    }
    /*async_buffered_output_log& write(const char* string) noexcept {
      write_nt_string(string);
      return *this;
    }*/
    template <std::integral T>
    async_buffered_output_log& write_hex(T value) noexcept {
      write_hex_number(value);
      return *this;
    }


    void flush() noexcept {
      push_to_queue();
    }
    void newline() noexcept {
      *currentCursor++ = '\n';
      size_t remainingSpace = currentBufferEnd - currentCursor;
      if ( remainingSpace >= bufferSize / 4 )
        push_to_queue();
      /*if ( remainingSpace >= bufferSize / 4 )
        *currentCursor++ = '\n';
      else {
        *currentCursor++ = 'X';
        push_to_queue();
      }*/
    }
  };

  class big_buffered_log {
    char*              buffer;
    size_t             bufferSize;
    size_t             charsWritten;
    size_t             flushLimit;
    char*              cursor;
    char* volatile     printCursor;
    std::atomic<char*> printBufferEnd;

    std::counting_semaphore<> chunksReady{(ptrdiff_t)0};

    std::jthread asyncThread;


    static void thread_proc(std::stop_token parent, big_buffered_log& log) noexcept {
      bool shouldStop = false;
      char* localPrintCursor = log.printCursor;
      HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
      while( !shouldStop ) {
        log.printBufferEnd.wait(localPrintCursor);
        // localPrintCursor = log.printCursor;
        unsigned long numberOfBytesWritten = 0;
        char* printBufEnd = log.printBufferEnd.load();
        if ( parent.stop_requested() )
          shouldStop = true;
        if ( printBufEnd == nullptr )
          continue;
        if ( printBufEnd < localPrintCursor ) {
          localPrintCursor -= log.bufferSize;
        }
        unsigned long totalBytes = printBufEnd - localPrintCursor;
        if ( totalBytes != 0 ) {
          WriteFile(StdOut, localPrintCursor, totalBytes - 1, &numberOfBytesWritten, nullptr);
          localPrintCursor += numberOfBytesWritten;
          if ( numberOfBytesWritten == totalBytes - 1 )
            ++localPrintCursor;
        }
        log.printCursor = localPrintCursor;
      }

      BOOL unmapResultOne = UnmapViewOfFile(log.buffer);
      BOOL unmapResultTwo = UnmapViewOfFile(log.buffer + log.bufferSize);
      assert( unmapResultOne == TRUE );
      assert( unmapResultTwo == TRUE );
    }

    void raw_write_char(char c) noexcept {
      *cursor++ = c;
      ++charsWritten;
    }
    void raw_write_string(const char* str, size_t n) noexcept {
      std::memcpy(cursor, str, n);
      charsWritten += n;
      cursor += n;
    }
    template <std::integral I>
    void raw_write_integer(I i, int base = 10) noexcept {
      char* bufferEnd = printCursor;
      if ( bufferEnd <= cursor )
        bufferEnd += bufferSize;
      auto [newCursor, err] = std::to_chars(cursor, bufferEnd, i, base);
      assert( err != std::errc::value_too_large );
      if ( err == std::errc{} ) {
        charsWritten += (newCursor - cursor);
        cursor = newCursor;
      }
    }

    void normalize() noexcept {
      if ( charsWritten >= flushLimit )
        flush();
      if ( cursor >= buffer + bufferSize )
        cursor -= bufferSize;
    }

  public:
    big_buffered_log(size_t size) {
      assert( size != 0 );
      bufferSize = ((JEM_VIRTUAL_PAGE_SIZE - 1) | (size - 1)) + 1;
      void* placeholderOne = VirtualAlloc2(nullptr, nullptr, bufferSize * 2, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
      void* placeholderTwo = (char*)placeholderOne + bufferSize;
      auto splitResult = VirtualFree(placeholderOne, bufferSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
      assert(splitResult == TRUE);
      auto memHandle = CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, (DWORD)bufferSize, nullptr);
      assert(memHandle != nullptr);
      auto pOne = MapViewOfFile3(memHandle, nullptr, placeholderOne, 0, bufferSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
      auto pTwo = MapViewOfFile3(memHandle, nullptr, placeholderTwo, 0, bufferSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
      assert(pOne == placeholderOne);
      assert(pTwo == placeholderTwo);
      buffer = (char*)pOne;
      cursor = buffer;
      printCursor = cursor;
      printBufferEnd.store(cursor);
      charsWritten = 0;
      flushLimit = (bufferSize / 4);
      asyncThread = std::jthread{&big_buffered_log::thread_proc, std::ref(*this)};
    }
    ~big_buffered_log() {
      if ( charsWritten > 0 )
        flush();
      else {
        printBufferEnd.store(nullptr);
        printBufferEnd.notify_one();
      }
    }

    big_buffered_log& write(std::string_view sv) noexcept {
      if ( !sv.empty() ) {
        raw_write_string(sv.data(), sv.size());
        normalize();
      }
      return *this;
    }
    template <size_t N>
    big_buffered_log& write(const char (&string_literal)[N]) noexcept {
      size_t n = N;
      if ( string_literal[N - 1] == '\0' )
        n -= 1;
      raw_write_string(string_literal, n);
      normalize();
      return *this;
    }
    template <std::integral I>
    big_buffered_log& write(I i) noexcept {
      raw_write_integer(i);
      normalize();
      return *this;
    }

    big_buffered_log& write(const std::chrono::microseconds& us) noexcept {
      constexpr static decltype(auto) UsString = "us";
      raw_write_integer(us.count());
      raw_write_string(UsString, sizeof(UsString) - 1);
      normalize();
      return *this;
    }

    template <std::integral I>
    big_buffered_log& write_hex(I i) noexcept {
      raw_write_integer(i, 16);
      normalize();
      return *this;
    }


    void flush() noexcept {
      raw_write_char('\n');
      charsWritten = 0;
      printBufferEnd.store(cursor);
      printBufferEnd.notify_one();
    }

    void newline() noexcept {
      flush();
    }
  };

  static_assert(sizeof(async_buffered_output_log) > sizeof(big_buffered_log));
}



struct qtz_request {

  inline constexpr static jem_u32_t DiscardedDeathFlags = QTZ_MESSAGE_NOT_PRESERVED | QTZ_MESSAGE_IS_DISCARDED;
  inline constexpr static jem_u32_t FinalizedDeathFlags = QTZ_MESSAGE_NOT_PRESERVED | QTZ_MESSAGE_IS_FINALIZED;
  inline constexpr static jem_u32_t FinalizedAndDiscardedFlags = DiscardedDeathFlags ^ FinalizedDeathFlags;

  JEM_cache_aligned
  jem_size_t           nextSlot;
  jem_size_t           thisSlot;

  qtz_message_kind_t   messageKind;
  jem_u32_t            queuePriority;

  atomic_flags32_t     flags = 0;
  qtz_message_action_t action;
  qtz_status_t         status;

  void*                senderObject;

  bool                 readyToDie;
  bool                 fromForeignProcess;

  // 27 free bytes


  JEM_cache_aligned
  unsigned char        payload[QTZ_REQUEST_PAYLOAD_SIZE];



  void init(jem_size_t slot) noexcept {
    nextSlot = slot + 1;
    thisSlot = slot;
    flags.set(QTZ_MESSAGE_NOT_PRESERVED);
    fromForeignProcess = false;
    readyToDie         = false;
  }


  template <typename P>
  inline P* payload_as() const noexcept {
    return (P*)payload;
  }


  bool is_discarded() const noexcept {
    return flags.test(QTZ_MESSAGE_IS_DISCARDED);
  }
  bool is_ready_to_die() const noexcept {
    return readyToDie;
  }

  void finalize() noexcept {
    readyToDie = (flags.fetch_and_set(QTZ_MESSAGE_IS_FINALIZED) & DiscardedDeathFlags) == DiscardedDeathFlags;
  }
  void discard() noexcept {
    readyToDie = (flags.fetch_and_set(QTZ_MESSAGE_IS_DISCARDED) & FinalizedDeathFlags) == FinalizedDeathFlags;
  }
  void finalize_and_return() noexcept {
    jem_u32_t oldFlags = flags.fetch_and_set(QTZ_MESSAGE_IS_READY | QTZ_MESSAGE_IS_FINALIZED);
    readyToDie = (oldFlags & DiscardedDeathFlags) == DiscardedDeathFlags;
    if ( !(oldFlags & QTZ_MESSAGE_IS_DISCARDED) )
      flags.notify_all();
  }
  void lock() noexcept {
    flags.reset(QTZ_MESSAGE_NOT_PRESERVED);
  }
  void unlock() noexcept {
    jem_u32_t oldFlags = flags.fetch_and_set(QTZ_MESSAGE_NOT_PRESERVED);
    readyToDie = (oldFlags & FinalizedAndDiscardedFlags) == FinalizedAndDiscardedFlags;
  }

  void reset() noexcept {
    flags.reset(QTZ_MESSAGE_NOT_PRESERVED);
    readyToDie = false;
    fromForeignProcess = false;
    messageKind = (qtz_message_kind_t)-1;
  }

  struct qtz_mailbox* parent_mailbox() const noexcept;
};

class request_priority_queue{

  jem_size_t     indexMask;
  jem_size_t     queueHeadIndex;
  jem_size_t     queueSize;
  qtz_request_t* entryArray;

  JEM_forceinline static jem_size_t parent(jem_size_t entry) noexcept {
    return entry >> 1;
  }
  JEM_forceinline static jem_size_t left(jem_size_t entry) noexcept {
    return entry << 1;
  }
  JEM_forceinline static jem_size_t right(jem_size_t entry) noexcept {
    return (entry << 1) + 1;
  }

  JEM_forceinline qtz_request_t& lookup(jem_size_t i) const noexcept {
    return const_cast<qtz_request_t&>(entryArray[indexMask & (queueHeadIndex + i)]);
  }

  JEM_forceinline bool is_less_than(jem_size_t a, jem_size_t b) const noexcept {
    return lookup(a)->queuePriority < lookup(b)->queuePriority;
  }
  JEM_forceinline void exchange(jem_size_t a, jem_size_t b) noexcept {
    std::swap(lookup(a), lookup(b));
  }


  inline void heapify(jem_size_t i) noexcept {
    jem_size_t l = left(i);
    jem_size_t r = right(i);
    jem_size_t smallest;

    if ( l > queueSize )
      return;

    qtz_request_t& i_ = lookup(i);
    qtz_request_t& l_ = lookup(l);

    if ( r > queueSize ) {
      if ( l_->queuePriority < i_->queuePriority )
        std::swap(l_, i_);
      return;
    }

    qtz_request_t& r_ = lookup(r);

    qtz_request_t& s_ = l_->queuePriority < r_->queuePriority ? l_ : r_;


    if ( s_->queuePriority < i_->queuePriority ) {
      jem_size_t s = l_->queuePriority < r_->queuePriority ? l : r;
      std::swap(s_, i_);
      heapify(s);
    }


    /*smallest = (l <= queueSize && is_less_than(l, i)) ? l : i;
    if ( r <= queueSize && is_less_than(r, smallest))
      smallest = r;

    if ( smallest != i ) {
      heapify(smallest);
    }*/
  }

  inline void build_heap() noexcept {
    for ( jem_size_t i = parent(queueSize); i > 0; --i )
      heapify(i);
  }

  inline void append_n(const qtz_request_t* requests, jem_size_t request_count) noexcept {

    /**
     *     ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
     *    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
     *    |   |   |   |   |   |   |   |   |   | 2 | 3 | 5 | 7 | 4 |   |   |
     *    |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
     *      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     *
     *  - queueHeadIndex = 8
     *  - queueSize      = 5
     *  - indexMask      = 0xF
     *
     *
     *     ___ ___ ___ ___ ___
     *    |   |   |   |   |   |
     *    | 7 | 1 | 2 | 6 | 1 |
     *    |___|___|___|___|___|
     *
     *  - request_count = 5
     *
     *  ::nextEndIndex       => 18
     *  ::actualNextEndIndex => 2
     *
     *
     *
     *     ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
     *    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
     *    | 2 | 6 | 1 |   |   |   |   |   |   | 2 | 3 | 5 | 7 | 4 | 7 | 1 |
     *    |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
     *      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
     *
     *
     *  - queueHeadIndex = 8
     *  - queueSize      = 10
     *  - indexMask      = 0xF
     *
     * */

    size_t nextEndIndex = queueHeadIndex + queueSize + request_count;
    size_t actualNextEndIndex = nextEndIndex & indexMask;

    if ( actualNextEndIndex == nextEndIndex || actualNextEndIndex == 0 ) [[likely]] {
      std::memcpy(&lookup(queueSize + 1), requests, request_count * sizeof(void*));
    }
    else {
      size_t distanceUntilEnd = nextEndIndex - actualNextEndIndex;
      std::memcpy(&lookup(queueSize + 1), requests, distanceUntilEnd * request_count);
      std::memcpy(entryArray, requests + distanceUntilEnd, actualNextEndIndex * sizeof(void*));
    }
    queueSize += request_count;
  }

  inline void rebalance_after_pop() noexcept {


    /*
     * Example:
     *
     *                                     A
     *                                   ⟋   ⟍
     *                                 ⟋       ⟍
     *                               ⟋           ⟍
     *                             ⟋               ⟍
     *                           ⟋                   ⟍
     *                         ⟋                       ⟍
     *                       ⟋                           ⟍
     *                     B                                C
     *                   ⟋   ⟍                           ⟋   ⟍
     *                 ⟋       ⟍                       ⟋       ⟍
     *               ⟋           ⟍                   ⟋           ⟍
     *             D               E               F                G
     *            / \             / \             / \              / \
     *           /   \           /   \           /   \            /   \
     *          /     \         /     \         /     \          /     \
     *         H       I       J       K       L       M        N       O
     *        / \     / \     / \     / \     / \     /
     *       /   \   /   \   /   \   /   \   /   \   /
     *      P     Q R     S T     U V     W X     Y Z
     *
     *
     *      Order:
     *
     *      N, G
     *      P, H, D, B, A
     *      R, I
     *      T, J, E
     *      V, K
     *      X, L, F, C
     *      Z, M
     *
     * */



    jem_size_t i = parent(queueSize);
    i |= 0x1;
    ++i;

    assert( i % 2 == 0);
    assert( i * 2 > queueSize);

    for ( ; i <= queueSize; i += 2) {

      // for each even indexed node in the second half of the array,
      // ie. for every left leaf node,

      size_t j = i;
      do {

        // for every parent node until one is a right child (odd indexed)

        size_t node  = j;
        qtz_request_t* p = &lookup(parent(node));
        qtz_request_t* c;

        do {

          // bubble sort lol

          c = &lookup(node);

          if ( (*p)->queuePriority < (*c)->queuePriority )
            break;

          std::swap(*p, *c);

          p = c;
          node *= 2;

        } while (node <= i);

        j = parent(j);

      } while ( !(j & 0x1) );

    }
  }
  inline void rebalance_after_insert() noexcept {

    qtz_request_t* node = &lookup(queueSize);

    for ( jem_size_t i = parent(queueSize); i > 0; i = parent(i)) {
      qtz_request_t* papi = &lookup(i);
      if ( (*node)->queuePriority >= (*papi)->queuePriority )
        break;
      std::swap(*node, *papi);
      node = papi;
    }

  }

  bool is_valid_heap_subtree(jem_size_t i) const noexcept {

    if ( i * 2 > queueSize )
      return true;


    jem_size_t    l = left(i);
    jem_size_t    r = right(i);
    qtz_request_t node = lookup(i);
    qtz_request_t l_node = lookup(l);
    qtz_request_t r_node = lookup(r);

    bool result = node->queuePriority <= l_node->queuePriority;

    if ( l == queueSize )
      return result;

    return result &&
           node->queuePriority <= r_node->queuePriority &&
           is_valid_heap_subtree(l) &&
           is_valid_heap_subtree(r);
  }

public:

  explicit request_priority_queue(jem_size_t maxSize) noexcept {
    jem_size_t actualArraySize = std::bit_ceil(maxSize);
    indexMask = actualArraySize - 1;
    queueHeadIndex = 0;
    queueSize = 0;
    entryArray = new qtz_request_t[actualArraySize];
  }

  ~request_priority_queue() {
    delete[] entryArray;
  }



  qtz_request_t peek() const noexcept {
    assert( not empty() );
    return lookup(1);
  }
  qtz_request_t pop() noexcept {
    auto req = peek();
    queueHeadIndex = (queueHeadIndex + 1) & indexMask;
    queueSize -= 1;
    rebalance_after_pop();
    return req;
  }

  void insert(qtz_request_t request) noexcept {
    lookup(++queueSize) = request;
    rebalance_after_insert();
  }
  void insert_n(const qtz_request_t* requests, jem_size_t request_count) noexcept {
    switch (request_count) {
      case 0:
        return;
      case 1:
        insert(*requests);
        return;
      default:
        append_n(requests, request_count);
        build_heap();
    }
  }

  jem_size_t size() const noexcept {
    return queueSize;
  }
  bool empty() const noexcept {
    return !queueSize;
  }

  bool is_valid_heap() const noexcept {
    return is_valid_heap_subtree(1);
  }
};

struct alignas(QTZ_REQUEST_SIZE) qtz_mailbox {

  using time_point = std::chrono::high_resolution_clock::time_point;
  using duration   = std::chrono::high_resolution_clock::duration;

  // Cacheline 0 - Semaphore for claiming slots [writer]
  JEM_cache_aligned

  semaphore_t slotSemaphore;


  // 48 free bytes


  // Cacheline 1 - Index of a free slot [writer]
  JEM_cache_aligned

  atomic_size_t nextFreeSlot;

  // 56 free bytes


  // Cacheline 2 - End of the queue [writer]
  JEM_cache_aligned

  atomic_size_t lastQueuedSlot;

  // 56 free bytes


  // Cacheline 3 - Queued message counter [writer,reader]
  JEM_cache_aligned

  // atomic_u32_t queuedSinceLastCheck;
  mpsc_counter_t queuedMessages;
  // jem_u8_t       uuid[16];
  jem_u32_t      totalSlotCount;
  jem_u32_t        defaultPriority;
  qtz_exit_code_t  exitCode;
  std::chrono::high_resolution_clock::time_point startTime;
  qtz_request_t    killRequest;
  std::atomic_flag shouldCloseMailbox;
  qtz_request_t    previousRequest;

  // 40 free bytes


  // Cacheline 4 - Mailbox state [reader]
  JEM_cache_aligned


  // qtz::async_buffered_output_log log;
  qtz::big_buffered_log          log;
  jem::fixed_size_pool           mailboxPool;

  // jem_u32_t        maxCurrentDeferred;
  // jem_u32_t        releasedSinceLastCheck;




  // 20 free bytes


  // Cacheline 5 - Memory Management [reader]
  // JEM_cache_aligned


  // request_priority_queue messagePriorityQueue;

  // jem::dictionary<qtz::handle_descriptor> namedHandles;






  // 48 free bytes


  JEM_cache_aligned

  char fileMappingName[JEM_CACHE_LINE];


  // Message Slots
  alignas(QTZ_REQUEST_SIZE) qtz_request messageSlots[];




  qtz_mailbox(jem_size_t slotCount, jem_size_t mailboxSize, const char fileMappingName[JEM_CACHE_LINE])
      : slotSemaphore(static_cast<jem_ptrdiff_t>(slotCount - 1)),
        nextFreeSlot(0),
        lastQueuedSlot(slotCount - 1),
        totalSlotCount(static_cast<jem_u32_t>(slotCount)),
        queuedMessages(),
        // uuid(),
        previousRequest(get_request(slotCount - 1)),
        // maxCurrentDeferred(0),
        // releasedSinceLastCheck(0),
        shouldCloseMailbox(),
        exitCode(0),
        mailboxPool(4 * JEM_CACHE_LINE, 255),
        // messagePriorityQueue(slotCount),
        // namedHandles(),
        defaultPriority(100),
        // log(512, 8),
        log(1),
        fileMappingName()
  {
    std::memcpy(this->fileMappingName, std::assume_aligned<JEM_CACHE_LINE>(fileMappingName), JEM_CACHE_LINE);

    for ( jem_size_t i = 0; i < slotCount; ++i )
      messageSlots[i].init(i);
  }

private:
  inline qtz_request_t     get_request(jem_size_t slot) const noexcept {
    assert(slot < totalSlotCount);
    return const_cast<qtz_request_t>(messageSlots + slot);
  }
  inline static jem_size_t get_slot(qtz_request_t request) noexcept {
    return request->thisSlot;
  }

  inline qtz_request_t get_free_request_slot() noexcept {

    jem_size_t thisSlot = nextFreeSlot.load(std::memory_order_acquire);
    jem_size_t nextSlot;
    qtz_request_t message;
    do {
      message  = get_request(thisSlot);
      nextSlot = message->nextSlot;
      // assert(atomic_load(&message->isFree, relaxed));
    } while ( !nextFreeSlot.compare_exchange_weak(thisSlot, nextSlot) );

    return message;
  }
  inline qtz_request_t get_next_queued_request() const noexcept {
    return get_request(previousRequest->nextSlot);
  }
  inline void          release_request_slot(qtz_request_t message) noexcept {
    const jem_size_t thisSlot = get_slot(message);
    jem_size_t newNextSlot = nextFreeSlot.load(std::memory_order_acquire);
    do {
      message->nextSlot = newNextSlot;
    } while( !nextFreeSlot.compare_exchange_weak(newNextSlot, thisSlot) );
    slotSemaphore.release();
  }
  inline void          replace_previous(qtz_request_t message) noexcept {
    message->lock();
    previousRequest->unlock();
    if ( previousRequest->is_ready_to_die() ) {
      previousRequest->reset();
      release_request_slot(previousRequest);
    }
    previousRequest = message;
  }

public:




  inline qtz_request_t acquire_free_slot() noexcept {
    slotSemaphore.acquire();
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot() noexcept {
    if ( !slotSemaphore.try_acquire() )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot_for(jem_u64_t timeout_us) noexcept {
    if ( !slotSemaphore.try_acquire_for(timeout_us) )
      return nullptr;
    return get_free_request_slot();
  }
  inline qtz_request_t try_acquire_free_slot_until(deadline_t deadline) noexcept {
    if ( !slotSemaphore.try_acquire_until(deadline) )
      return nullptr;
    return get_free_request_slot();
  }


  inline qtz_request_t acquire_next_queued_request() noexcept {

    queuedMessages.decrease(1);
    auto nextRequest = get_next_queued_request();
    replace_previous(nextRequest);
    return nextRequest;


    /*if ( messagePriorityQueue.empty() ) {


      qtz_request_t message = get_next_queued_request();

      auto minQueued = queuedSinceLastCheck.exchange(0);
      assert(minQueued != 0);
      if ( minQueued == 1 ) {
        replace_previous(message);
        return message;
      }
      else {
        messagePriorityQueue.insert(message);
        for (auto n = minQueued - 1; n > 0; --n) {
          message = get_request(message->nextSlot);
          messagePriorityQueue.insert(message);
        }
        replace_previous(message);
      }
    }
    else {

      auto minQueued = queuedSinceLastCheck.exchange(0);

      if ( minQueued > 0 ) {
        qtz_request_t message = get_next_queued_request();
        messagePriorityQueue.insert(message);
        for (auto n = minQueued - 1; n > 0; --n) {
          message = get_request(message->nextSlot);
          messagePriorityQueue.insert(message);
        }
        replace_previous(message);
      }
    }


    return messagePriorityQueue.pop();*/
  }


  inline void          enqueue_request(qtz_request_t message) noexcept {
    const jem_size_t slot = get_slot(message);
    jem_size_t prevLastQueuedSlot = lastQueuedSlot.exchange(slot);
    /*jem_size_t prevLastQueuedSlot = lastQueuedSlot.load(std::memory_order_acquire);
    do {
      //message->nextSlot = lastQueuedSlot;
    } while ( !lastQueuedSlot.compare_exchange_weak(prevLastQueuedSlot, slot) );*/
    get_request(prevLastQueuedSlot)->nextSlot = slot;
    queuedMessages.increase(1);
    /*queuedSinceLastCheck.fetch_add(1, std::memory_order_release);
    queuedSinceLastCheck.notify_one();*/
  }
  inline void          discard_request(qtz_request_t message) noexcept {
    message->discard();
    if ( message->is_ready_to_die() ) {
      message->reset();
      release_request_slot(message);
    }
  }

  inline bool          should_close() const noexcept {
    return shouldCloseMailbox.test();
  }




  void process_next_message() noexcept {

    using PFN_request_proc = qtz_message_action_t(qtz_mailbox::*)(qtz_request_t) noexcept;

    constexpr static PFN_request_proc global_dispatch_table[] = {
      &qtz_mailbox::proc_noop,
      &qtz_mailbox::proc_kill,
      &qtz_mailbox::proc_placeholder2,
      &qtz_mailbox::proc_alloc_mailbox,
      &qtz_mailbox::proc_free_mailbox,
      &qtz_mailbox::proc_placeholder3,
      &qtz_mailbox::proc_placeholder4,
      &qtz_mailbox::proc_placeholder5,
      &qtz_mailbox::proc_placeholder6,
      &qtz_mailbox::proc_placeholder7,
      &qtz_mailbox::proc_placeholder8,
      &qtz_mailbox::proc_placeholder9,
      &qtz_mailbox::proc_placeholder10,
      &qtz_mailbox::proc_placeholder11,
      &qtz_mailbox::proc_register_object,
      &qtz_mailbox::proc_unregister_object,
      &qtz_mailbox::proc_link_objects,
      &qtz_mailbox::proc_unlink_objects,
      &qtz_mailbox::proc_open_object_handle,
      &qtz_mailbox::proc_destroy_object,
      &qtz_mailbox::proc_log_message,
      &qtz_mailbox::proc_execute_callback,
      &qtz_mailbox::proc_execute_callback_with_buffer
    };

    auto message = acquire_next_queued_request();

    /*if ( message->is_discarded() )
      return;*/

    switch ( (this->*(global_dispatch_table[message->messageKind]))(message) ) {
      case QTZ_ACTION_DISCARD:
        // FIXME: Messages are not being properly discarded :(
        message->discard();
        break;
      case QTZ_ACTION_NOTIFY_LISTENER:
        message->finalize_and_return();
        break;
      case QTZ_ACTION_DEFERRED:
        message->finalize();
        break;
        JEM_no_default;
    }
  }


  /*
   * GLOBAL_MESSAGE_KIND_NOOP,
GLOBAL_MESSAGE_KIND_1,
GLOBAL_MESSAGE_KIND_2,
GLOBAL_MESSAGE_KIND_ALLOCATE_MAILBOX,
GLOBAL_MESSAGE_KIND_FREE_MAILBOX,
GLOBAL_MESSAGE_KIND_3,
GLOBAL_MESSAGE_KIND_4,
GLOBAL_MESSAGE_KIND_5,
GLOBAL_MESSAGE_KIND_6,
GLOBAL_MESSAGE_KIND_7,
GLOBAL_MESSAGE_KIND_8,
GLOBAL_MESSAGE_KIND_9,
GLOBAL_MESSAGE_KIND_10,
GLOBAL_MESSAGE_KIND_11,
GLOBAL_MESSAGE_KIND_REGISTER_OBJECT,
GLOBAL_MESSAGE_KIND_UNREGISTER_OBJECT,
GLOBAL_MESSAGE_KIND_LINK_OBJECTS,
GLOBAL_MESSAGE_KIND_UNLINK_OBJECTS,
GLOBAL_MESSAGE_KIND_OPEN_OBJECT_HANDLE,
GLOBAL_MESSAGE_KIND_DESTROY_OBJECT,
GLOBAL_MESSAGE_KIND_LOG_MESSAGE,
GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK,
GLOBAL_MESSAGE_KIND_EXECUTE_CALLBACK_WITH_BUFFER
   *
   * */


  qtz_message_action_t proc_noop(qtz_request_t request) noexcept;
  qtz_message_action_t proc_kill(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder2(qtz_request_t request) noexcept;
  qtz_message_action_t proc_alloc_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_free_mailbox(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder3(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder4(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder5(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder6(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder7(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder8(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder9(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder10(qtz_request_t request) noexcept;
  qtz_message_action_t proc_placeholder11(qtz_request_t request) noexcept;
  qtz_message_action_t proc_register_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_unregister_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_link_objects(qtz_request_t request) noexcept;
  qtz_message_action_t proc_unlink_objects(qtz_request_t request) noexcept;
  qtz_message_action_t proc_open_object_handle(qtz_request_t request) noexcept;
  qtz_message_action_t proc_destroy_object(qtz_request_t request) noexcept;
  qtz_message_action_t proc_log_message(qtz_request_t request) noexcept;
  qtz_message_action_t proc_execute_callback(qtz_request_t request) noexcept;
  qtz_message_action_t proc_execute_callback_with_buffer(qtz_request_t request) noexcept;

};

inline struct qtz_mailbox* qtz_request::parent_mailbox() const noexcept {
  return const_cast<qtz_mailbox*>(reinterpret_cast<const qtz_mailbox*>(this - (thisSlot + 1)));
}


#if defined(_WIN32)
#define QTZ_NULL_HANDLE nullptr
#else
#define QTZ_NULL_HANDLE 0
#endif

extern qtz_mailbox* g_qtzGlobalMailbox;
extern void*        g_qtzProcessAddressSpace;
extern size_t       g_qtzProcessAddressSpaceSize;


extern qtz_handle_t g_qtzProcessInboxFileMapping;
extern size_t       g_qtzProcessInboxFileMappingBytes;

extern qtz_handle_t g_qtzMailboxThreadHandle;
extern qtz_tid_t    g_qtzMailboxThreadId;


extern qtz_handle_t g_qtzKernelCreationMutex;
extern qtz_handle_t g_qtzKernelBlockMappingHandle;

extern qtz_handle_t g_qtzKernelProcessHandle;
extern qtz_pid_t    g_qtzKernelProcessId;




extern "C" qtz_exit_code_t JEM_stdcall qtz_mailbox_main_thread_proc(void *params);


static_assert(sizeof(qtz_mailbox) == (8 * JEM_CACHE_LINE));



#endif//JEMSYS_QUARTZ_INTERNAL_H
