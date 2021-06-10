cmake_minimum_required(VERSION 3.20)
project(jemsys C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(jemsys.quartz src/quartz/mm.cpp src/quartz/init.c src/quartz/queue.cpp src/quartz/epoch.cpp src/quartz/deputy.cpp src/quartz/module.cpp)
target_include_directories(jemsys.quartz PUBLIC include)
add_library(jemsys.agate src/agate/mailbox.cpp)
target_include_directories(jemsys.agate PUBLIC include)
add_library(jemsys.opal src/opal/agent.cpp)
target_include_directories(jemsys.opal PUBLIC include)

add_library(jemsys library.c include/jemsys.h)
target_link_libraries(jemsys PUBLIC jemsys.quartz jemsys.agate jemsys.opal)

