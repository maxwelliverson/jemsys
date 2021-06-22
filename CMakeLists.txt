cmake_minimum_required(VERSION 3.19)
project(jemsys C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(jemsys.internal.ipc src/ipc.cpp)
target_include_directories(jemsys.internal.ipc PUBLIC include src)

add_library(jemsys.quartz src/quartz/mm.cpp src/quartz/init.cpp src/quartz/queue.cpp src/quartz/epoch.cpp src/quartz/deputy.cpp src/quartz/module.cpp)
target_include_directories(jemsys.quartz PUBLIC include src)
add_library(jemsys.agate src/agate/mailbox.cpp src/agate/deputy.cpp src/agate/spsc_mailbox.cpp src/agate/mpsc_mailbox.cpp src/agate/spmc_mailbox.cpp src/agate/mpmc_mailbox.cpp src/agate/message.cpp)
target_include_directories(jemsys.agate PUBLIC include src)
add_library(jemsys.opal src/opal/agent.cpp)
target_include_directories(jemsys.opal PUBLIC include src)

add_library(jemsys library.c include/jemsys.h)

target_link_libraries(jemsys.quartz PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys.agate  PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys.opal   PUBLIC jemsys.internal.ipc)
target_link_libraries(jemsys        PUBLIC jemsys.quartz jemsys.agate jemsys.opal)
