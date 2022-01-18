cmake_minimum_required(VERSION 3.19)
project(jemsys C CXX #[[CUDA]])

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CUDA_STANDARD 17)

#set(LLVM_DIR $ENV{LLVM_CMAKE_DIR})
#set(LLVM_DIR "C:/Users/maxwe/git/vcpkg/installed/x64-windows/share/llvm")
#find_package(LLVM REQUIRED)
#find_library(LLVM_SUPPORT_LIB LLVMSupport.lib REQUIRED)

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
    list(TRANSFORM JEMSYS_LIB_SOURCES PREPEND src/${target_}/)
    if(JEMSYS_LIB_PRIVATE)
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
        target_compile_definitions(${JEMSYS_LIB_SHARED_NAME} PUBLIC JEM_SHARED_LIB PRIVATE JEM_SHARED_LIB_EXPORT)
    endif()
endfunction()


function(add_jemsys_executable target_)
    set(optionalArgs TEST SAMPLE)
    set(singleValueArgs LINK)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(JEMSYS_EXE "${optionalArgs}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    if(JEMSYS_EXE_TEST)
        if(target_ MATCHES "catch")
            set(JEMSYS_EXE_TARGET ${target_})
            set(JEMSYS_EXE_SOURCE_PREFIX "test")
        else()
            set(JEMSYS_EXE_TARGET test.${target_})
            set(JEMSYS_EXE_SOURCE_PREFIX "${JEMSYS_EXE_TARGET}")
        endif()
    elseif(JEMSYS_EXE_SAMPLE)
        set(JEMSYS_EXE_TARGET sample.${target_})
        set(JEMSYS_EXE_SOURCE_PREFIX "${JEMSYS_EXE_TARGET}")
    else()
        set(JEMSYS_EXE_TARGET ${target_})
        set(JEMSYS_EXE_SOURCE_PREFIX "src.${JEMSYS_EXE_TARGET}")
    endif()
    # message("JEMSYS_EXE_SOURCE_PREFIX: ${JEMSYS_EXE_SOURCE_PREFIX}")
    string(REGEX REPLACE "\\." "/" JEMSYS_EXE_SOURCE_PREFIX ${JEMSYS_EXE_SOURCE_PREFIX})
    list(TRANSFORM JEMSYS_EXE_SOURCES PREPEND "${JEMSYS_EXE_SOURCE_PREFIX}/")
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
                object_manager.cpp
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
                message.cpp
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
        agate2
            SOURCES
                library.cpp
                channels.cpp
                objects.cpp
                context.cpp
                message.cpp
                async.cpp
                atomic.cpp
                signal.cpp
                vtable.cpp
            DEPENDENCIES
                support
)

add_jemsys_library(
        silica2
            SOURCES
                interface.cpp
            DEPENDENCIES
                support
)

add_jemsys_library(
        quartz2
            SOURCES
                init.cpp
            DEPENDENCIES
                support
)

add_jemsys_library(
        opal
            SOURCES
                agent.cpp
                coroutines.cpp
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



#[[add_jemsys_executable(
        quartz_kernel
            SOURCES
                quartz/kernel.cpp
            DEPENDENCIES
                LLVMSupport
)]]

add_jemsys_executable(
        catch
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
        quartz.priority_queue
            SAMPLE
            SOURCES
                main.cpp
            DEPENDENCIES
                quartz
)

add_jemsys_executable(
        agate.mailbox
            SAMPLE
            SOURCES
                main.cpp
            DEPENDENCIES
                agate
)



add_jemsys_executable(
        quartz.benchmark
            SAMPLE
            SOURCES
                main.cpp
            DEPENDENCIES
                quartz
)

add_jemsys_executable(
        agate2.server
            SAMPLE
            SOURCES
                main.cpp
            DEPENDENCIES
                agate2
)




#[[
add_library(rng_kernels sample/rng/rng_lib.cu)
target_include_directories(rng_kernels PRIVATE include src)

add_jemsys_executable(
        rng
            SAMPLE
            SOURCES
                rng/main.cpp
                rng/rng_lib.cpp
            DEPENDENCIES
                agate
                rng_kernels
)]]
