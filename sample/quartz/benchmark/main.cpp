//
// Created by maxwe on 2021-10-21.
//

#include "quartz.h"
#include "agate.h"

#include <thread>
#include <barrier>
// #include <format>
#include <semaphore>
#include <iostream>
#include <charconv>

std::chrono::high_resolution_clock::time_point g_procStartTime;
std::chrono::high_resolution_clock::time_point g_sendStartTime;
std::chrono::high_resolution_clock::time_point g_sendEndTime;
std::chrono::high_resolution_clock::time_point g_procEndTime;

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

inline void execute_ops_on_threads(jem_size_t N, jem_size_t messagesPerThread) noexcept {
  auto startTime = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  std::barrier             entryBarrier{(ptrdiff_t)N, []() noexcept {
                              struct execute_callback_request{
                                size_t structLength;
                                void(* callback)(void*);
                                void*  userData;
                              } callbackRequestBuffer{
                                .structLength = sizeof(execute_callback_request),
                                .callback     = [](void*){
                                  g_procStartTime = std::chrono::high_resolution_clock::now();
                                },
                                .userData     = nullptr
                              };

                              qtz_discard(qtz_send(QTZ_THIS_PROCESS, QTZ_DEFAULT_PRIORITY, 21, &callbackRequestBuffer, JEM_WAIT));

                              g_sendStartTime = std::chrono::high_resolution_clock::now();
                            }};
  std::barrier             finishBarrier{(ptrdiff_t)N, []() noexcept { g_sendEndTime = std::chrono::high_resolution_clock::now(); }};
  threads.reserve(N);

  struct payload_t{
    size_t structLength;
    std::chrono::high_resolution_clock::time_point startTime;
  } setTimePayload{
    .structLength = sizeof(payload_t),
    .startTime    = startTime
  };

  qtz_send(QTZ_THIS_PROCESS, 1, 2, &setTimePayload, JEM_WAIT);

  for ( size_t i = 0; i < N; ++i ) {
    threads.emplace_back([&]() mutable {

      agt_set_self((agt_agent_t)get_thread_id());

      jem_size_t messageCount = messagesPerThread;

      entryBarrier.arrive_and_wait();



      for ( jem_size_t j = 0; j < messageCount; ++j ) {
        /*struct buffer_t{
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
*/
        qtz_send(QTZ_THIS_PROCESS, QTZ_DEFAULT_PRIORITY, 0, nullptr, JEM_WAIT);
      }

      finishBarrier.arrive_and_wait();

    });
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
      g_procEndTime = std::chrono::high_resolution_clock::now();
      static_cast<std::binary_semaphore*>(sem)->release();
    },
    .userData     = &isDoneProcessingRequests
  };



  qtz_discard(qtz_send(QTZ_THIS_PROCESS, 1000, 21, &callbackRequestBuffer, JEM_WAIT));

  isDoneProcessingRequests.acquire();

  auto totalMessages = N * messagesPerThread;
  auto totalSendTime = g_sendEndTime - g_sendStartTime;
  auto totalProcTime = g_procEndTime - g_procStartTime;
  auto totalTime = g_procEndTime - g_sendStartTime;
  auto totalSendTimePerMessage = totalSendTime / (float)totalMessages;
  auto totalProcTimePerMessage = totalProcTime / (float)(totalMessages + 2);

  std::cout << "totalMessages: " << totalMessages << "\n";
  std::cout << "totalSendTime: " << totalSendTime << "\n";
  std::cout << "totalProcTime: " << totalProcTime << "\n";
  std::cout << "totalTime:     " << totalTime << "\n\n";
  std::cout << "totalSendTimePerMessage: " << totalSendTimePerMessage << "\n";
  std::cout << "totalProcTimePerMessage: " << totalProcTimePerMessage << std::endl;
}

int main() {
  qtz_init_params_t initParams{
    .kernel_version     = 0,
    .kernel_mode        = QTZ_KERNEL_INIT_CREATE_NEW,
    .kernel_access_code = "catch2-test-kernel",
    .message_slot_count = 16000,
    .process_name       = "Multithreaded Init Test Proc",
    .module_count       = 0,
    .modules            = nullptr
  };
  auto result = qtz_init(&initParams);

  execute_ops_on_threads(6, 1000);

  struct kill_request{
    size_t structSize;
    unsigned long exitCode;
  } killRequest{
    .structSize = sizeof(kill_request),
    .exitCode   = 0
  };

  auto killResult = qtz_wait(qtz_send(QTZ_THIS_PROCESS, 1, 1, &killRequest, JEM_WAIT), JEM_WAIT);

  assert( killResult == QTZ_SUCCESS );
}