//
// Created by maxwe on 2021-09-25.
//

#include <quartz.h>
#include <agate.h>

#include <thread>
#include <barrier>
// #include <format>
#include <semaphore>
#include <iostream>
#include <charconv>

using namespace std::chrono_literals;

inline void* get_thread_id() noexcept {
  return (void*)std::hash<std::thread::id>{}(std::this_thread::get_id());
}

inline std::chrono::microseconds us_since(const std::chrono::high_resolution_clock::time_point& start) noexcept {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
}

/*template <typename ...Args>
inline void log(std::string_view fmt, Args&& ...args) noexcept {
  thread_local void* threadId = get_thread_id();
  std::string str = std::format(fmt, args...);

  size_t length = str.size();
  size_t bufferLength = length + sizeof(size_t) + 1;
  auto buffer = new char[bufferLength];
  std::memcpy(buffer, &bufferLength, sizeof(size_t));
  std::memcpy(buffer + sizeof(size_t), str.data(), length + 1);
  qtz_send(threadId, 20, buffer, 0, JEM_WAIT);
  delete[] buffer;
}*/

template <size_t N>
inline void print_to_cursor(char*& cursor, const char (&string)[N]) noexcept {
  std::memcpy(cursor, string, N - 1);
  cursor += (N - 1);
}
inline void print_to_cursor(char*& cursor, char* const bufferEnd, size_t n) noexcept {
  auto&& [newCursor, result] = std::to_chars(cursor, bufferEnd, n);
  cursor = newCursor;
}
inline void print_to_cursor(char*& cursor, char* const bufferEnd, const std::chrono::microseconds& us) noexcept {
  print_to_cursor(cursor, bufferEnd, us.count());
  print_to_cursor(cursor, "us");
}
inline void end_string(char* cursor) noexcept {
  *cursor = '\0';
}

inline void execute_ops_on_threads(jem_size_t N) noexcept {
  auto startTime = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  std::barrier<>           threadBarrier{(ptrdiff_t)N};
  threads.reserve(N);

  struct payload_t{
    size_t structLength;
    std::chrono::high_resolution_clock::time_point startTime;
  } setTimePayload{
    .structLength = sizeof(payload_t),
    .startTime    = startTime
  };

  qtz_send(nullptr, 2, &setTimePayload, 1, JEM_WAIT);

  for ( size_t i = 0; i < N; ++i ) {
    threads.emplace_back([=](std::barrier<>& barrier){

      agt_set_self((agt_agent_t)get_thread_id());

      barrier.arrive_and_wait();

      jem_size_t N2 = N * N;

      for ( jem_size_t j = 0; j < N2; ++j ) {
        struct buffer_t{
          size_t bufferLength;
          char   data[72];
        } buffer;
        char* cursor = buffer.data;
        char* const bufferEnd = cursor + sizeof(buffer.data);

        print_to_cursor(cursor, "thread#");
        print_to_cursor(cursor, bufferEnd, i);
        print_to_cursor(cursor, ", msg#");
        print_to_cursor(cursor, bufferEnd, j);
        print_to_cursor(cursor, ", @");
        print_to_cursor(cursor, bufferEnd, us_since(startTime));
        buffer.bufferLength = cursor - reinterpret_cast<char*>(&buffer);
        // end_string(cursor);

        qtz_send(QTZ_THIS_PROCESS, QTZ_DEFAULT_PRIORITY, 20, &buffer, JEM_WAIT);

        // log("thread#{}, msg#{}, @{}", i, j, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime));
      }

    }, std::ref(threadBarrier));
  }

  for ( auto& thread : threads )
    thread.join();

  std::binary_semaphore isDoneProcessingRequests{false};

  struct execute_callback_request{
    size_t structLength;
    void(* callback)(void*);
    void*  userData;
  } callbackRequestBuffer{
    .structLength = sizeof(execute_callback_request),
    .callback     = [](void* sem){
      std::cout << std::endl;
      static_cast<std::binary_semaphore*>(sem)->release();
    },
    .userData     = &isDoneProcessingRequests
  };



  qtz_send(get_thread_id(), 21, &callbackRequestBuffer, 1000, JEM_WAIT);

  isDoneProcessingRequests.acquire();
}

int main() {
  qtz_init_params_t initParams{
    .kernel_version     = 0,
    .kernel_mode        = QTZ_KERNEL_INIT_CREATE_NEW,
    .kernel_access_code = "catch2-test-kernel",
    .message_slot_count = 2047,
    .process_name       = "Multithreaded Init Test Proc",
    .module_count       = 0,
    .modules            = nullptr
  };
  auto result = qtz_init(&initParams);

  execute_ops_on_threads(16);

  // std::this_thread::sleep_for(2s);

  struct kill_request{
    size_t structSize;
    unsigned long exitCode;
  } killRequest{
    .structSize = sizeof(kill_request),
    .exitCode   = 0
  };

  auto killResult = qtz_request_wait(qtz_send(nullptr, 1, &killRequest, 1, JEM_WAIT), JEM_WAIT);

  assert( killResult == QTZ_SUCCESS );
}