cmake_minimum_required(VERSION 3.19)
project(jemsys C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(jemsys.internal.ipc src/ipc.cpp)
target_include_directories(jemsys.internal.ipc PUBLIC include src)

add_library(jemsys.quartz src/quartz/mm.cpp src/quartz/init.cpp src/quartz/module.cpp src/quartz/mailbox.cpp)
target_include_directories(jemsys.quartz PUBLIC include src)
add_library(jemsys.agate src/agate/mailbox.cpp src/agate/deputy.cpp src/agate/spsc_mailbox.cpp src/agate/mpsc_mailbox.cpp src/agate/spmc_mailbox.cpp src/agate/mpmc_mailbox.cpp src/agate/message.cpp src/agate/dispatch.cpp src/agate/dispatch/mpsc_mailbox/local.cpp src/agate/handle.cpp src/agate/dispatch/mpsc_mailbox/ipc.cpp src/agate/dispatch/mpsc_mailbox/dynamic_local.cpp src/agate/dispatch/mpsc_mailbox/dynamic_ipc.cpp src/agate/dispatch/private_mailbox/local.cpp src/agate/dispatch/private_mailbox/dynamic_local.cpp)
target_include_directories(jemsys.agate PUBLIC include src)
add_library(jemsys.opal src/opal/agent.cpp)
target_include_directories(jemsys.opal PUBLIC include src)
add_library(jemsys.silica src/silica.cpp)
target_include_directories(jemsys.silica PUBLIC include src)

add_executable(jemsys.quartz.kernel src/quartz/kernel.cpp)
target_include_directories(jemsys.quartz.kernel PUBLIC include src)

add_executable(jemsys.quartz.prioq.TEST src/quartz/priority_queue_test.cpp)
target_include_directories(jemsys.quartz.prioq.TEST PUBLIC include src)

add_library(jemsys library.c include/jemsys.h)

target_link_libraries(jemsys.quartz PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys.agate  PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys.opal   PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys        PUBLIC jemsys.quartz jemsys.agate jemsys.opal)
