cmake_minimum_required(VERSION 3.19)
project(jemsys C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)


add_library(asan INTERFACE)
if (CMAKE_ASAN_ENABLED)
    set(ASAN_LIBRARY_DIRECTORY "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/lib/clang/12.0.0/lib/windows")
    target_compile_options(asan INTERFACE -fsanitize=address /Od)
    target_link_directories(asan INTERFACE "${ASAN_LIBRARY_DIRECTORY}")
endif()


function(add_jemsys_library target_)
    set(optionalArgs PRIVATE)
    set(singleValueArgs)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(JEMSYS_LIB "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    if(JEMSYS_LIB_PRIVATE)
        list(TRANSFORM JEMSYS_LIB_SOURCES PREPEND src/)
        add_library(${target_} STATIC ${JEMSYS_LIB_SOURCES})
        target_include_directories(${target_} PRIVATE include src)
        if(DEFINED JEMSYS_LIB_DEPENDENCIES)
            target_link_libraries(${target_} PRIVATE ${JEMSYS_LIB_DEPENDENCIES})
        endif()
    else()
        list(APPEND JEMSYS_LIBRARIES ${target_})
        set(JEMSYS_LIBRARIES ${JEMSYS_LIBRARIES} PARENT_SCOPE)
        set(JEMSYS_LIB_SHARED_NAME ${target_}_shared)
        set(JEMSYS_LIB_STATIC_NAME ${target_}_static)
        list(TRANSFORM JEMSYS_LIB_SOURCES PREPEND src/${target_}/)
        add_library(${JEMSYS_LIB_SHARED_NAME} SHARED ${JEMSYS_LIB_SOURCES})
        add_library(${JEMSYS_LIB_STATIC_NAME} STATIC ${JEMSYS_LIB_SOURCES})

        if(DEFINED JEMSYS_LIB_DEPENDENCIES)
            foreach(dep IN LISTS JEMSYS_LIB_DEPENDENCIES)
                list(FIND JEMSYS_LIBRARIES ${dep} IS_JEMSYS_LIB)
                if(IS_JEMSYS_LIB GREATER -1)
                    list(APPEND JEMSYS_LIB_STATIC_DEPENDENCIES ${dep}_static)
                    list(APPEND JEMSYS_LIB_SHARED_DEPENDENCIES ${dep}_shared)
                else()
                    list(APPEND JEMSYS_LIB_STATIC_DEPENDENCIES ${dep})
                    list(APPEND JEMSYS_LIB_SHARED_DEPENDENCIES ${dep})
                endif()
            endforeach()

            target_link_libraries(${JEMSYS_LIB_STATIC_NAME} PRIVATE ${JEMSYS_LIB_STATIC_DEPENDENCIES})
            target_link_libraries(${JEMSYS_LIB_SHARED_NAME} PRIVATE ${JEMSYS_LIB_SHARED_DEPENDENCIES})
        endif()

        set_target_properties(${JEMSYS_LIB_SHARED_NAME}
                PROPERTIES
                OUTPUT_NAME ${target_})
        target_include_directories(${JEMSYS_LIB_SHARED_NAME} PRIVATE include src)
        target_include_directories(${JEMSYS_LIB_STATIC_NAME} PRIVATE include src)
        target_compile_definitions(${JEMSYS_LIB_SHARED_NAME} PRIVATE JEM_SHARED_LIB JEM_SHARED_LIB_EXPORT)

    endif()
endfunction()


function(add_jemsys_executable target_)
    set(optionalArgs TEST)
    set(singleValueArgs LINK)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(JEMSYS_EXE "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    if(JEMSYS_EXE_TEST)
        set(JEMSYS_EXE_TARGET test.${target_})
        list(TRANSFORM JEMSYS_EXE_SOURCES PREPEND test/)
    else()
        set(JEMSYS_EXE_TARGET ${target_})
        list(TRANSFORM JEMSYS_EXE_SOURCES PREPEND src/)
    endif()
    add_executable(${JEMSYS_EXE_TARGET} ${JEMSYS_EXE_SOURCES})
    target_include_directories(${JEMSYS_EXE_TARGET} PUBLIC include src)
    if(NOT DEFINED JEMSYS_EXE_LINK)
        set(JEMSYS_EXE_LINK static)
    endif()
    if(DEFINED JEMSYS_EXE_DEPENDENCIES)
        foreach(dep IN LISTS JEMSYS_EXE_DEPENDENCIES)
            list(FIND JEMSYS_LIBRARIES ${dep} IS_JEMSYS_LIB)
            if(IS_JEMSYS_LIB GREATER -1)
                list(APPEND JEMSYS_EXE_REAL_DEPENDENCIES ${dep}_${JEMSYS_EXE_LINK})
            else()
                list(APPEND JEMSYS_EXE_REAL_DEPENDENCIES ${dep})
            endif()
        endforeach()

        target_link_libraries(${JEMSYS_EXE_TARGET} PRIVATE ${JEMSYS_EXE_REAL_DEPENDENCIES})
    endif()
    if(JEMSYS_EXE_TEST)
        set_target_properties(${JEMSYS_EXE_TARGET} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()
endfunction()







add_jemsys_library(
        support
            PRIVATE
            SOURCES
                dictionary.cpp
                vector.cpp
)

add_jemsys_library(
        ipc
            PRIVATE
            SOURCES
                ipc.cpp
)

add_jemsys_library(
        quartz
            SOURCES
                init.cpp
                mailbox.cpp
                self.cpp
            DEPENDENCIES
                support
)

add_jemsys_library(
        agate
            SOURCES
                agate.cpp
                vtable.cpp
                agent.cpp
                spsc_mailbox.cpp
                mpsc_mailbox.cpp
                spmc_mailbox.cpp
                mpmc_mailbox.cpp
                private_mailbox.cpp
            DEPENDENCIES
                support
                quartz
)

add_jemsys_library(
        opal
            SOURCES
                agent.cpp
)


add_jemsys_library(
        silica
            SOURCES
                silica.cpp
            DEPENDENCIES
                support
)



add_library(jemsys SHARED src/jemsys.cpp)
target_link_libraries(jemsys PUBLIC agate_shared quartz_shared #[[opal_shared]] silica_shared)
target_include_directories(jemsys PRIVATE include src)
target_compile_definitions(jemsys PRIVATE JEM_SHARED_LIB)



add_jemsys_executable(
        quartz_kernel
            SOURCES
                quartz/kernel.cpp
)

add_jemsys_executable(
        priority_queue
            TEST
            SOURCES
                quartz/priority_queue_test.cpp
)

add_jemsys_executable(
        quartz_logging
            TEST
            SOURCES
                quartz/logging.cpp
            DEPENDENCIES
                support
                quartz
                agate
)

add_jemsys_executable(
        agate_mailbox
            TEST
            SOURCES
                agate/solo_mailbox.cpp
            DEPENDENCIES
                agate
)

add_jemsys_executable(
        all
            TEST
            SOURCES
                catch.cpp
                silica/attributes.cpp
                silica/module_builder.cpp
                agate/mailbox.cpp
                quartz/init.cpp
            DEPENDENCIES
                quartz
                agate
                silica
)

add_jemsys_executable(
        quartz_benchmark
            TEST
            SOURCES
                quartz/speed_test.cpp
            DEPENDENCIES
                quartz
)