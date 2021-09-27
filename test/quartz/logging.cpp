//
// Created by maxwe on 2021-09-25.
//

#include <quartz/core.h>

#include <thread>
#include <barrier>
#include <format>
#include <semaphore>
#include <iostream>

using namespace std::chrono_literals;

inline void* get_thread_id() noexcept {
  return (void*)std::hash<std::thread::id>{}(std::this_thread::get_id());
}

template <typename ...Args>
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

  qtz_send(nullptr, 1, &setTimePayload, 1, JEM_WAIT);

  for ( size_t i = 0; i < N; ++i ) {
    threads.emplace_back([=](std::barrier<>& barrier){

      barrier.arrive_and_wait();

      for ( jem_size_t j = 0; j < N; ++j ) {
        log("thread#{}, msg#{}, @{}", i, j, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime));
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

  execute_ops_on_threads(4);
}