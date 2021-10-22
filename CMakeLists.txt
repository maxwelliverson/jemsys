cmake_minimum_required(VERSION 3.19)
project(jemsys C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)


add_library(asan INTERFACE)
target_compile_options(asan INTERFACE -fsanitize=address)
target_link_directories(asan INTERFACE "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/lib/clang/12.0.0/lib/windows")
#target_link_libraries(asan INTERFACE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
#target_link_options(asan INTERFACE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_library(jemsys.internal #[[src/ipc.cpp]] src/dictionary.cpp src/vector.cpp)
target_include_directories(jemsys.internal PUBLIC include src)
add_library(jemsys.internal.ipc src/ipc.cpp)
target_include_directories(jemsys.internal.ipc PUBLIC include src)

add_library(jemsys.quartz #[[src/quartz/mm.cpp]] src/quartz/init.cpp src/quartz/mailbox.cpp src/quartz/self.cpp)
target_include_directories(jemsys.quartz PUBLIC include src)
add_library(jemsys.old.agate src/agate/mailbox.cpp src/agate/deputy.cpp #[[src/agate/spsc_mailbox.cpp src/agate/mpsc_mailbox.cpp src/agate/spmc_mailbox.cpp src/agate/mpmc_mailbox.cpp]] src/agate/message.cpp src/agate/dispatch.cpp src/agate/dispatch/mpsc_mailbox/local.cpp src/agate/handle.cpp src/agate/dispatch/mpsc_mailbox/ipc.cpp src/agate/dispatch/mpsc_mailbox/dynamic_local.cpp src/agate/dispatch/mpsc_mailbox/dynamic_ipc.cpp src/agate/dispatch/private_mailbox/local.cpp src/agate/dispatch/private_mailbox/dynamic_local.cpp)
target_include_directories(jemsys.old.agate PUBLIC include src)
add_library(jemsys.agate src/agate/agent.cpp src/agate/spsc_mailbox.cpp src/agate/mpsc_mailbox.cpp src/agate/spmc_mailbox.cpp src/agate/mpmc_mailbox.cpp src/agate/vtable.cpp src/agate/agate.cpp src/agate/private_mailbox.cpp)
target_include_directories(jemsys.agate PUBLIC include src)
add_library(jemsys.opal src/opal/agent.cpp)
target_include_directories(jemsys.opal PUBLIC include src)
add_library(jemsys.silica src/silica.cpp)
target_include_directories(jemsys.silica PUBLIC include src)

add_executable(jemsys.quartz.kernel src/quartz/kernel.cpp)
target_include_directories(jemsys.quartz.kernel PUBLIC include src)

add_executable(jemsys.quartz.prioq.TEST test/quartz/priority_queue_test.cpp)
target_include_directories(jemsys.quartz.prioq.TEST PUBLIC include src)

add_library(jemsys library.c include/jemsys.h)

add_executable(TEST.quartz.logging test/quartz/logging.cpp)
target_include_directories(TEST.quartz.logging PUBLIC include src)

add_executable(TEST.agate.mailbox test/agate/solo_mailbox.cpp)

target_include_directories(TEST.agate.mailbox PUBLIC include src)

add_executable(TEST.jemsys test/catch.cpp #[[test/quartz/init_test.cpp]] test/silica/module_builder.cpp test/silica/attributes.cpp test/agate/mailbox.cpp)
target_include_directories(TEST.jemsys PUBLIC include src)



#target_link_libraries(jemsys.old.agate PUBLIC jemsys.internal)

target_link_libraries(jemsys.quartz PUBLIC jemsys.internal)
target_link_libraries(jemsys.old.agate PUBLIC jemsys.internal)
target_link_libraries(jemsys.agate  PUBLIC jemsys.internal jemsys.quartz)
target_link_libraries(jemsys.opal   PUBLIC jemsys.internal)
target_link_libraries(TEST.quartz.logging PUBLIC jemsys.internal jemsys.quartz)
target_link_libraries(TEST.agate.mailbox PUBLIC jemsys.agate jemsys.quartz asan)

target_link_libraries(TEST.jemsys   PUBLIC jemsys.internal jemsys.quartz jemsys.agate jemsys.silica)
