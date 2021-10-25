//
// Created by maxwe on 2021-07-03.
//

#include "kernel.hpp"


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#include <unordered_map>
#include <charconv>
#include <mutex>
#include <memory>
#include <optional>
#include <cstdlib>
#include <cstring>

namespace {
  
  using process_id_t = int;

  using timepoint_t = std::chrono::high_resolution_clock::time_point;


  enum visibility_t{
    public_visibility,
    protected_visibility,
    private_visibility
  };


  class local_handle {

  protected:
    int processId;

    explicit local_handle(int id) noexcept : processId(id){ }

  public:

    virtual std::string_view get_name() const noexcept = 0;
    virtual bool is_ipc_capable() const noexcept = 0;
    virtual bool is_visible_from(const local_handle& other) const noexcept = 0;


  };

  struct deputy_info {
    std::string  name;

    visibility_t visibility;
  };
  struct thread_info {

  };

  struct local_address {

  };


  using handle_map_t = std::unordered_map<uintptr_t, std::unique_ptr<local_handle>>;

  struct process_info {
    std::string  name;

    process_id_t   id;
    handle_map_t handles;
    uintptr_t    useCount;


  };

  struct kernel_statistics {
    timepoint_t creationTime;

  };

  struct kernel_mailbox {
    std::mutex writeMtx;
  };

  struct kernel_state {
    timepoint_t                           creationTime;
    process_id_t                            creatingProcessId;
    std::unordered_map<int, process_info> processes;

    void*                                 fileMappingHandle;
    void*                                 address;
    size_t                                totalSize;
  };

  struct init_kernel_args {
    process_id_t        srcProcessId;
    uintptr_t         inboxAddress;
    int*              result;
    std::atomic_flag* isReady;
    uintptr_t*        kernelInboxAddress;

    std::optional<size_t> sharedAddressSpaceSize;


    jem_size_t        srcProcNameLength;
    char              srcProcName[];
  };


  struct address_space {
    void*  handle;
    void*  address;
    size_t totalSize;

  };


  bool decode_params(init_kernel_args*& args, int argc, char** argv) noexcept {

    // [0]:   string   executablePath
    // [1]:   string   processName
    // [2]:   int      processId
    // [3]:   address  result
    // [4]:   address  isReady
    // [5]:   address  kernelInboxAddress
    // [6]:   address  procInboxAddress

    if ( argc < 6 )
      return false;

    const size_t procNameLength = strlen(argv[1]);

    args = static_cast<init_kernel_args*>(malloc(sizeof(init_kernel_args) + procNameLength + 1));

    args->srcProcNameLength = procNameLength;
    std::memcpy(args->srcProcName, argv[1], procNameLength + 1 );

    auto [ procIdPtr, procIdErr ]               = std::from_chars(argv[2], argv[2] + std::strlen(argv[2]), args->srcProcessId);
    auto [ resultPtr, resultErr ]               = std::from_chars(argv[3], argv[3] + std::strlen(argv[3]), reinterpret_cast<uintptr_t&>(args->result));
    auto [ isReadyPtr, isReadyErr ]             = std::from_chars(argv[4], argv[4] + std::strlen(argv[4]), reinterpret_cast<uintptr_t&>(args->isReady));
    auto [ kernelAddressPtr, kernelAddressErr ] = std::from_chars(argv[5], argv[5] + std::strlen(argv[5]), reinterpret_cast<uintptr_t&>(args->kernelInboxAddress));
    auto [ inboxAddressPtr, inboxAddressErr ]   = std::from_chars(argv[6], argv[6] + std::strlen(argv[6]), reinterpret_cast<uintptr_t&>(args->inboxAddress));



    constexpr static std::errc success{};


    if ( procIdErr != success )
      return false;
    if ( resultErr != success )
      return false;
    if ( isReadyErr != success )
      return false;
    if ( kernelAddressErr != success )
      return false;
    if ( inboxAddressErr != success )
      return false;
    return true;
  }

  bool init_address_space(init_kernel_args* args, address_space& addrSpace) noexcept {



    if ( !args->sharedAddressSpaceSize.has_value() ) {

    }



    SECURITY_ATTRIBUTES secAttr{
      .nLength              = sizeof(SECURITY_ATTRIBUTES),
      .lpSecurityDescriptor = nullptr,
      .bInheritHandle       = true
    };

    /*addrSpace.handle = CreateFileMapping2(INVALID_HANDLE_VALUE,
                                          &secAttr,
                                          FILE_MAP_WRITE,
                                          PAGE_READWRITE,
                                          0,
                                          );*/

    return false;
  }


}





int main(int argc, char** argv) {

  using clock = std::chrono::high_resolution_clock;

  init_kernel_args* args;
  address_space     kernel;
  kernel_state      state;

  if ( !decode_params(args, argc, argv) ) {

    if ( args ) {
      if ( args->result ) {
        *args->result = 0;
      }
      if ( args->isReady ) {
        args->isReady->test_and_set();
      }
    }

    free(args);

    return 1;
  }

  assert( args != nullptr );


  state.creationTime      = clock::now();
  state.creatingProcessId = args->srcProcessId;

  if ( !init_address_space(args, kernel) ) {

  }

  // ... Initialize Kernel State


  free(args);


  // ... Event loop

  return 0;
}
