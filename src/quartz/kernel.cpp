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


#pragma warning(push)
#pragma warning(disable:4624)
#include <llvm/Support/CommandLine.h>
#pragma warning(pop)


#define JEMSYS_KERNEL_VERSION ((JEMSYS_KERNEL_VERSION_MAJOR << 22) | (JEMSYS_KERNEL_VERSION_MINOR << 12) | JEMSYS_KERNEL_VERSION_PATCH)

using namespace llvm::cl;


template <typename DataType>
class llvm::cl::basic_parser<DataType*> : public parser<uintptr_t> {
public:
  using parser_data_type = DataType*;
  using OptVal = OptionValue<DataType*>;

  basic_parser(Option &O) : parser<uintptr_t>(O) {}
};

template <typename DataType>
class llvm::cl::parser<DataType*> : public basic_parser<DataType*> {
  using StringRef = llvm::StringRef;
public:
  parser(Option& O) : basic_parser<DataType*>(O) {}



  // parse - Return true on error.
  bool parse(Option &O, StringRef ArgName, StringRef Arg, DataType* &Val) {
    return parser<uintptr_t>::parse(O, ArgName, Arg, reinterpret_cast<uintptr_t&>(Val));
    // return this->parse(O, ArgName, Arg, reinterpret_cast<uintptr_t&>(Val));
  }

  // getValueName - Overload in subclass to provide a better default value.
  StringRef getValueName() const override { return "address"; }

  void printOptionDiff(const Option &O, DataType* V, OptionValue<DataType*> Default, size_t GlobalWidth) const {
    parser<uintptr_t>::printOptionDiff(O, (uintptr_t)V, *(OptionValue<uintptr_t>*)&Default, GlobalWidth);
  }
};

class version_parser : public parser<unsigned> {
  using StringRef = llvm::StringRef;
public:
  version_parser(Option& O) : parser<unsigned>(O){}

  // parse - Return true on error.
  bool parse(Option &O, StringRef ArgName, StringRef Arg, unsigned &Val) {

    using std::tie;

    unsigned long major = 0;
    unsigned long minor = 0;
    unsigned long patch = 0;

    if (Arg.front() == 'v')
      Arg = Arg.drop_front();

    const char* strPos = Arg.data();
    char* dotPos;


    StringRef tok;
    StringRef remaining = Arg;

    tie(tok, remaining) = getToken(remaining, ".");
    if (tok.empty() || !tok.getAsInteger(10, major))
      return true;
    tie(tok, remaining) = getToken(remaining, ".");
    if (!tok.empty()) {
      if (!tok.getAsInteger(10, minor))
        return true;
      tie(tok, remaining) = getToken(remaining, ".");
      if (!tok.empty()) {
        if (!remaining.empty() || !tok.getAsInteger(10, patch))
          return true;
      }
    }
    Val = ((major << 22) | (minor << 12) | patch);
    return false;
  }

  // getValueName - Overload in subclass to provide a better default value.
  StringRef getValueName() const override { return "version"; }

  // void printOptionDiff(const Option &O, unsigned V, OptVal Default, size_t GlobalWidth) const;
};

namespace {

  inline constexpr jem_size_t DefaultSharedAddressSpaceSize = 1024 * 1024 * 1024 * 16ULL;
  
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


  struct local_entity_info {

  };
  struct agent_info {
    std::string  name;

    visibility_t visibility;
  };
  struct mailbox_info {
    std::string  name;

    visibility_t visibility;
  };

  struct thread_info {

  };

  struct local_address {

  };


  using handle_map_t = std::unordered_map<uintptr_t, std::unique_ptr<local_handle>>;

  struct process_info {
    char                       name[JEM_CACHE_LINE*2];
    jem_global_id_t            jemId;
    qtz_pid_t                  osId;
    handle_map_t               handles;
    uintptr_t                  useCount;

    jem_ptrdiff_t              mailboxAddrOffset;
    qtz_mailbox_t              mailbox;



    qtz::kernel_control_block* localKcb;
  };

  struct kernel_statistics {
    timepoint_t creationTime;
    jem_size_t  messagesProcessed;
    jem_u32_t   processesAttached;
    jem_u32_t   processesDetached;
    jem_size_t  sharedAgentsAttached;
    jem_size_t  sharedAgentsDetached;
  };

  struct kernel_mailbox {
    std::mutex writeMtx;
  };

  struct kernel_state {
    qtz::kernel_control_block*                        kcb;
    timepoint_t                                       creationTime;
    process_id_t                                      creatingProcessId;
    std::unordered_map<jem_global_id_t, process_info> processes;

    kernel_statistics                                 stats;

    void*                                             fileMappingHandle;
    void*                                             address;
    size_t                                            totalSize;

    qtz_exit_code_t                                   exitCode;
  };

  struct init_kernel_args {
    process_id_t          srcProcessId;
    uintptr_t             inboxAddress;
    int*                  result;
    std::atomic_flag*     isReady;
    uintptr_t*            kernelInboxAddress;

    std::optional<size_t> sharedAddressSpaceSize;


    jem_size_t            srcProcNameLength;
    char                  srcProcName[];
  };


  struct address_space {
    void*  handle;
    void*  address;
    size_t totalSize;

  };

  opt<unsigned, false, version_parser> o_minVersion{"min", Optional, ValueRequired, init(0), desc("kernel version must be at least this")};

  opt<unsigned, false, version_parser> o_maxVersion{"max", Optional, ValueRequired, init(std::numeric_limits<unsigned>::max()), desc("kernel version cannot be any greater than this")};

  opt<size_t>         o_sharedAddressSpaceSize{"s", Optional, ValueRequired, init(DefaultSharedAddressSpaceSize), desc("size of the shared address space")};



  // opt<std::string>    o_srcProcessInboxName{"mailbox", Required, desc("Mailbox identifier of the calling process")};

  // opt<std::string>    o_srcProcessName{Positional, Required, desc("proc-name")};

  opt<std::string>    o_kernelAccessCode{Positional, Required, desc("<kernel-id>")};

  // opt<qtz_handle_t>   o_kernelMemoryBlockHandle{Positional, Required, desc("<kernel-handle>")};

  /*opt<qtz_handle_t>   o_procInboxOsHandle{Positional, Required,
                                value_desc("proc-inbox-handle"),
                                desc("OS handle of the source process' inbox memory block")};
  opt<void*>          o_procInboxAddr{Positional, Required,
                                value_desc("proc-inbox-addr"),
                                desc("address (in the source process) of the source process' inbox")};
  opt<int*>           o_resultAddr{Positional, Required,
                                value_desc("result-addr"),
                                desc("address (in the source process) where the result of kernel initialization should be stored")};
  opt<atomic_flag_t*> o_isReadyAddr{Positional, Required,
                                value_desc("is-ready-addr"),
                                desc("address (in the source process) of the flag used to indicate kernel initialization has completed")};
  opt<void**>         o_kernelInboxAddr{Positional, Required,
                                value_desc("kernel-inbox-addr"),
                                desc("address (in the source process) where the kernel's inbox address will be stored")};
*/





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


  bool init_kernel(kernel_state& state) noexcept {
    using clock = std::chrono::high_resolution_clock;

    state.creationTime = clock::now();
    // state.creatingProcessId = o_srcProcessId;
    state.exitCode = 0;
    return true;
  }

  void kernel_event_loop(kernel_state& state) {

  }

}



#define JEMSYS_KERNEL_VERSION_MAJOR 0
#define JEMSYS_KERNEL_VERSION_MINOR 0
#define JEMSYS_KERNEL_VERSION_PATCH 0


int main(int argc, char** argv) {
  SetVersionPrinter([](llvm::raw_ostream& os){
    os << "v" << JEMSYS_KERNEL_VERSION_MAJOR <<
          "." << JEMSYS_KERNEL_VERSION_MINOR <<
          "." << JEMSYS_KERNEL_VERSION_PATCH;
  });
  ParseCommandLineOptions(argc, argv, "A interprocess communication mediator for use by Jemsys libraries. "
                                      "This program should never be invoked directly.");

  kernel_state      state;

  if (init_kernel(state)) {
    kernel_event_loop(state);
  }

  return (int)state.exitCode;
}
