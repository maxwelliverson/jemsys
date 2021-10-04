//
// Created by maxwe on 2021-06-01.
//

#include "internal.hpp"


// #include <quartz/init.h>
#include "kernel.hpp"


#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>

#include <rpc.h>


#pragma comment(lib, "mincore")
#pragma comment(lib, "Rpcrt4")
#else
#include <pthread.h>
#endif


#if !defined(_WIN32)
using DWORD = unsigned;
#endif




using namespace qtz;


qtz_mailbox* g_qtzGlobalMailbox = nullptr;

void*        g_qtzProcessAddressSpace = nullptr;
size_t       g_qtzProcessAddressSpaceSize = 0;


qtz_handle_t g_qtzProcessInboxFileMapping = QTZ_NULL_HANDLE;
size_t       g_qtzProcessInboxFileMappingBytes = 0;

qtz_handle_t g_qtzMailboxThreadHandle = QTZ_NULL_HANDLE;
qtz_tid_t    g_qtzMailboxThreadId = 0;

qtz_handle_t g_qtzKernelCreationMutex = QTZ_NULL_HANDLE;
qtz_handle_t g_qtzKernelBlockMappingHandle = QTZ_NULL_HANDLE;

qtz_handle_t g_qtzKernelProcessHandle = QTZ_NULL_HANDLE;
qtz_pid_t    g_qtzKernelProcessId = 0;

kernel_control_block* qtz::g_kernelBlock = nullptr;

namespace {

  inline constexpr jem_size_t KernelMemoryBlockSize = (0x1ULL << 26);
  inline constexpr jem_size_t KernelExecutableBufferSize = 32;
  inline constexpr jem_size_t AddressSpaceSize = (0x1ULL << 36);
  inline constexpr jem_size_t MailboxStackSize = 0;
  inline constexpr DWORD      MailboxThreadFlags = 0;

  std::atomic_bool   g_processIsInitialized{false};
  binary_semaphore_t g_isInitializing{1};

  bool     g_addressSpaceIsInitialized{false};
  bool     g_mailboxIsInitialized{false};
  bool     g_mailboxThreadIsInitialized{false};
  bool     g_kernelIsInitialized{false};

  /**
   *
   * 256GiB
   *
   *  64KiB - Small Slab
   *   4MiB - Medium Slab
   * 256MiB - Large Slab
   *  16GiB - Huge Slab
   *
   *
   *
   * |  4MiB  |  4MiB  |   256GiB
   * |  4MiB  |  4MiB  |
   * |________|________|________________________________|________ ... ________|
   *
   * */


#if defined(_WIN32)
  JEM_forceinline DWORD high_bits_of(jem_u64_t x) noexcept {
    return static_cast<DWORD>((0xFFFF'FFFF'0000'0000ULL & x) >> 32);
  }
  JEM_forceinline DWORD low_bits_of(jem_u64_t x) noexcept {
    return static_cast<DWORD>(0x0000'0000'FFFF'FFFFULL & x);
  }




  struct security_descriptor {
    SECURITY_ATTRIBUTES attributes;
    void*               data;
  };





  inline const char* allocKernelBlockName(const char* accessCode) noexcept;
  inline void        freeKernelBlockName(const char* nameString) noexcept;

  inline const char* getDefaultProcessName() noexcept;
  inline const char* getDefaultKernelId() noexcept;


  inline void getKernelExecutableName(char exeBuffer[KernelExecutableBufferSize], jem_u32_t kernelVersion) noexcept;




  inline void initKernelProcSACL(security_descriptor& descriptor) noexcept {
    descriptor.attributes.bInheritHandle = false;
    descriptor.attributes.lpSecurityDescriptor = nullptr;
    descriptor.attributes.nLength = sizeof(descriptor.attributes);
  }
  inline void initKernelBlockSACL(security_descriptor& descriptor) noexcept {
    descriptor.attributes.bInheritHandle = false;
    descriptor.attributes.lpSecurityDescriptor = nullptr;
    descriptor.attributes.nLength = sizeof(descriptor.attributes);
  }
  inline void initKernelThreadSACL(security_descriptor& descriptor) noexcept {
    descriptor.attributes.bInheritHandle = false;
    descriptor.attributes.lpSecurityDescriptor = nullptr;
    descriptor.attributes.nLength = sizeof(descriptor.attributes);
  }
  inline void initMailboxThreadSACL(security_descriptor& descriptor) noexcept {
    descriptor.attributes.bInheritHandle = false;
    descriptor.attributes.lpSecurityDescriptor = nullptr;
    descriptor.attributes.nLength = sizeof(descriptor.attributes);
  }
  inline void destroyKernelProcSACL(const security_descriptor& descriptor) noexcept { }
  inline void destroyKernelThreadSACL(const security_descriptor& descriptor) noexcept { }
  inline void destroyKernelBlockSACL(const security_descriptor& descriptor) noexcept { }
  inline void destroyMailboxThreadSACL(const security_descriptor& descriptor) noexcept { }







  inline jem_status_t createKernelProcess(const qtz::init_kernel_args& args) noexcept {

    PROCESS_INFORMATION procInfo;
    security_descriptor kProcSecurity{};
    security_descriptor kThreadSecurity{};
    jem_status_t status = JEM_SUCCESS;
    char* argBuffer = qtz::encode_params(args);

    char exeNameBuffer[KernelExecutableBufferSize];



    initKernelProcSACL(kProcSecurity);
    initKernelThreadSACL(kThreadSecurity);

    getKernelExecutableName(exeNameBuffer, args.kernelVersion);


    if ( !CreateProcess(exeNameBuffer,
                        argBuffer,
                        &kProcSecurity.attributes,
                        &kThreadSecurity.attributes,
                        false,
                        DETACHED_PROCESS,
                        nullptr,
                        nullptr,
                        nullptr,
                        &procInfo) ) {
      status = JEM_ERROR_UNKNOWN;
      g_kernelBlock->state.store(kernel_process_launch_failed);
      g_kernelBlock->state.notify_one();
    }

    free(argBuffer);

    destroyKernelThreadSACL(kThreadSecurity);
    destroyKernelProcSACL(kProcSecurity);

    return status;
  }




  inline qtz_status_t initAddressSpace(const qtz_init_params_t& params) noexcept {

    MEM_ADDRESS_REQUIREMENTS addressRequirements{
      .LowestStartingAddress = nullptr,
      .HighestEndingAddress = nullptr,
      .Alignment = AddressSpaceSize
    };

    MEM_EXTENDED_PARAMETER extendedParameter{
      .Type = MemExtendedParameterAddressRequirements,
      .Pointer = &addressRequirements
    };

    g_qtzProcessAddressSpace = VirtualAlloc2(
      GetCurrentProcess(),
      nullptr,
      AddressSpaceSize,
      MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
      PAGE_NOACCESS,
      &extendedParameter,
      1);
    g_qtzProcessAddressSpaceSize = AddressSpaceSize;

    if ( g_qtzProcessAddressSpace == nullptr )
      return QTZ_ERROR_BAD_ALLOC;
    return QTZ_SUCCESS;
  }
  inline qtz_status_t initMailbox(const qtz_init_params_t& params) noexcept {

    const jem_size_t messageSlotCount = std::bit_ceil(params.message_slot_count);
    char             fileMappingName[JEM_CACHE_LINE]{};
    char* nameCursor   = fileMappingName;
    char* const nameSentinel = fileMappingName + sizeof(fileMappingName);
    security_descriptor threadSecDesc{};

    UUID     mailboxID;
    RPC_CSTR mailboxNameIDString;


    constexpr static char NamePrefix[] = "qtz-mailbox:{";

    std::memcpy(fileMappingName, NamePrefix, sizeof(NamePrefix) - 1);
    nameCursor += sizeof(NamePrefix) - 1;

    UuidCreate(&mailboxID);
    UuidToString(&mailboxID, &mailboxNameIDString);

    const size_t idStringLength = strlen((const char*)mailboxNameIDString);

    std::memcpy(nameCursor, (const char*)mailboxNameIDString, idStringLength);
    nameCursor += idStringLength;
    *nameCursor = '}';
    ++nameCursor;
    *nameCursor = '\0';

    RpcStringFree(&mailboxNameIDString);

    jem_size_t totalFileMappingSize = messageSlotCount * QTZ_REQUEST_SIZE;

    qtz_handle_t fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high_bits_of(totalFileMappingSize), low_bits_of(totalFileMappingSize), fileMappingName);

    if ( !fileMapping ) {
      return QTZ_ERROR_BAD_ALLOC;
    }

    void* mailboxMemory = MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, totalFileMappingSize);
    // void* mailboxMemory = MapViewOfFile3(fileMapping, GetCurrentProcess(), nullptr, 0, totalFileMappingSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, nullptr, 0);

    if ( !mailboxMemory ) {
      CloseHandle(fileMapping);
      return QTZ_ERROR_BAD_ALLOC;
    }


    g_qtzGlobalMailbox = new (mailboxMemory) qtz_mailbox(messageSlotCount - 1, totalFileMappingSize, fileMappingName);
    onexit([]{



      return 0;
    });

    initMailboxThreadSACL(threadSecDesc);


    g_qtzMailboxThreadHandle = CreateThread(&threadSecDesc.attributes, MailboxStackSize, qtz_mailbox_main_thread_proc, nullptr, MailboxThreadFlags, &g_qtzMailboxThreadId);

    if ( !g_qtzMailboxThreadHandle ) {
      auto error = GetLastError();
      // TODO: Handle thread creation errors and return appropriate status
      return QTZ_ERROR_UNKNOWN;
    }

    destroyMailboxThreadSACL(threadSecDesc);

    return QTZ_SUCCESS;
  }
  inline jem_status_t initKernel(const qtz_init_params_t& params) noexcept {

    /**
     * Kernel creation/access is performed via a control block.
     *
     * During initialization, the calling process attempts to open/create the control block.
     * If a process creates a new control block, it must then create the kernel process.
     * This process ensures that kernels with unique IDs will have unique process instances
     * across the system, even when many processes attempt to initialize the same kernel all
     * at once.
     * */

    constexpr static jem_u32_t KernelBlockAccessRights = STANDARD_RIGHTS_REQUIRED | FILE_MAP_WRITE | FILE_MAP_READ;


    jem_status_t status      = JEM_SUCCESS;
    const char*  processName = params.process_name;
    const char*  kernelId    = params.kernel_access_code;

    if ( !processName || (*processName == '\0'))
      processName = getDefaultProcessName();

    if ( !kernelId || (*kernelId == '\0'))
      kernelId = getDefaultKernelId();

    qtz::init_kernel_args kernelArgs{
      .processName             = processName,
      .processInboxFileMapping = g_qtzProcessInboxFileMapping,
      .processInboxBytes       = g_qtzProcessInboxFileMappingBytes,
      .srcProcessId            = static_cast<qtz_pid_t>(GetCurrentProcessId()),
      .kernelVersion           = params.kernel_version,
      .kernelIdentifier        = kernelId,
      .kernelBlockName         = allocKernelBlockName(kernelId),
      .kernelBlockSize         = KernelMemoryBlockSize
    };

    security_descriptor kBlockSecurity{};

    initKernelBlockSACL(kBlockSecurity);


    switch ( params.kernel_mode ) {
      case QTZ_KERNEL_INIT_OPEN_EXISTING:
        g_qtzKernelBlockMappingHandle = OpenFileMapping(KernelBlockAccessRights, FALSE, kernelArgs.kernelBlockName);
        if ( !g_qtzKernelBlockMappingHandle)
          status = JEM_ERROR_DOES_NOT_EXIST;
        break;
      case QTZ_KERNEL_INIT_CREATE_NEW:
      case QTZ_KERNEL_INIT_OPEN_ALWAYS:
        g_qtzKernelBlockMappingHandle = CreateFileMapping(
          INVALID_HANDLE_VALUE,
          &kBlockSecurity.attributes,
          PAGE_READWRITE,
          high_bits_of(kernelArgs.kernelBlockSize),
          low_bits_of(kernelArgs.kernelBlockSize),
          kernelArgs.kernelBlockName
        );

        if ( !g_qtzKernelBlockMappingHandle)
          status = JEM_ERROR_UNKNOWN;
        else if ( GetLastError() == ERROR_ALREADY_EXISTS ) {

          // kernel block has already been created by some other process.
          // If the init mode was CREATE_NEW, initialization fails.
          // If the init mode was OPEN_ALWAYS (the default), kernel initialization
          // is effectively completed.

          if ( params.kernel_mode == QTZ_KERNEL_INIT_CREATE_NEW ) {
            CloseHandle(g_qtzKernelBlockMappingHandle);
            g_qtzKernelBlockMappingHandle = nullptr;
            status = JEM_ERROR_ALREADY_EXISTS;
          }
        }
        else {

          // Created a new kernel block; kernel process needs to be started.

          status = createKernelProcess(kernelArgs);

        }
        break;
      default:
        status = JEM_ERROR_INVALID_ARGUMENT;
    }





    destroyKernelBlockSACL(kBlockSecurity);
    freeKernelBlockName(kernelArgs.kernelBlockName);

    return status;
  }
#else

  inline qtz_status_t initAddressSpace(const qtz_init_params_t& params) noexcept {

  }
  inline qtz_status_t initMailbox(const qtz_init_params_t& params) noexcept {

  }


#endif


  inline void destroyKernel() {

  }
  inline void destroyMailbox() {
    /*
     * Send message to kernel indicating that this process is disconnecting...
     * */

    g_qtzGlobalMailbox->shouldCloseMailbox.test_and_set();

    // Dump log?
  }
}






extern "C" {
  JEM_api qtz_status_t  JEM_stdcall qtz_init(const qtz_init_params_t* params) {

    qtz_status_t status = QTZ_SUCCESS;

    if ( g_processIsInitialized.load() )
      return status;

    if ( !g_isInitializing.try_acquire_for(1000 * 500) )
      return QTZ_ERROR_UNKNOWN;

    if ( g_processIsInitialized.load() )
      goto exit;


    if ( params == nullptr ) [[unlikely]]
      return QTZ_ERROR_INVALID_ARGUMENT;



    // Initialize address space

    // ...

    if ( !g_addressSpaceIsInitialized ) {
      if ( (status = initAddressSpace(*params)) != QTZ_SUCCESS )
        goto exit;
      g_addressSpaceIsInitialized = true;
    }

    // Initialize process mailbox

    if ( !g_mailboxIsInitialized ) {
      if ( (status = initMailbox(*params)) != QTZ_SUCCESS )
        goto exit;
      g_mailboxIsInitialized = true;
    }

    // Initialize IPC kernel

    /*if ( !g_kernelIsInitialized ) {
      if ( (status = initKernel(*params)) != JEM_SUCCESS )
        goto exit;
      g_kernelIsInitialized = true;
    }


    if ( g_addressSpaceIsInitialized && g_mailboxIsInitialized && g_kernelIsInitialized )
      g_processIsInitialized.test_and_set();*/

    if ( g_addressSpaceIsInitialized && g_mailboxIsInitialized )
      g_processIsInitialized.store(true);



    // atexit(destroyKernel);
    atexit(destroyMailbox);

    exit:
      g_isInitializing.release();
      return status;
  }
}