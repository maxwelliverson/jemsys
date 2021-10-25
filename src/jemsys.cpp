//
// Created by maxwe on 2021-10-22.
//

#define JEM_SHARED_LIB_EXPORT

#include <jemsys.h>
#include <quartz.h>


#if JEM_system_windows
# if defined(JEM_SHARED_LIB)

#include "support/environment.hpp"


/*template <typename T>
class environment_variable;

class environment_variable_base;

class environment {

  constexpr static size_t VariablesMaxCapacity = 64;
  constexpr static size_t LogPrefixMaxLength = 64;
  constexpr static size_t LogBufferLength = BUFSIZ;

  FILE*                       logFile;
  bool                        loggingEnabled;
  char                        logPrefix[LogPrefixMaxLength];
  char                        logBuffer[LogBufferLength];
  size_t                      variableCount;
  size_t                      maxVariables;
  environment_variable_base** variables;


  friend class environment_variable_base;

  void push_back(environment_variable_base* var);

public:

  template <std::derived_from<environment_variable_base> Var, typename ...Args>
  Var* add(const char* name, const char* defaultValue, Args&& ...args) {
    auto newVar = new Var(name, defaultValue, std::forward<Args>(args)...);
    push_back(newVar);
    return newVar;
  }



  void set_log_prefix(std::string_view prefix) noexcept {
    if (!prefix.empty() || prefix.size() >= LogPrefixMaxLength) {
      memcpy_s(logPrefix, LogPrefixMaxLength - 1, prefix.data(), prefix.size());
      logPrefix[prefix.size()] = '\0';
    }
    else {
      std::memset(logPrefix, 0, LogPrefixMaxLength);
    }
  }
  void set_logging_file(FILE* loggingFile) {
    if (logFile)
      fclose(logFile);
    logFile = loggingFile;
    std::setvbuf(loggingFile, logBuffer, _IOFBF, LogBufferLength);
  }
  void enable_logging(bool isEnabled = true) {
    loggingEnabled = isEnabled;
  }
};

class environment_variable_base {

  friend class environment;

protected:

  constexpr static size_t ErrorMsgMaxLength = (0x1 << 12);
  constexpr static unsigned DefaultMaxLength = 512;

  static char errorMessageBuffer[ErrorMsgMaxLength];

  environment* env;
  size_t       index;
  const char* name;
  const char* defaultValue;
  char* buffer;
  unsigned maxLength;

  void print_default_value_message() {

  }

  void print_length_error_message() {
    int result = fprintf_s(stderr,
                           "(jemsys) => The value of the environment variable %s exceeds "
                           "the maximum allowed length ( = %u bytes )\n"
                           "(jemsys) => Set %s to its default value ( = %s )\n",
                           name,
                           maxLength,
                           name,
                           defaultValue);

  }
  void print_validation_error_message(const char* fmt, ...) {
    vsprintf_s();
  }

  virtual bool validate_value(char* value) {
    return true;
  }

  const char* lookup() {
    buffer = new char[maxLength];

  }

  explicit environment_variable_base(const char* name, const char* defaultValue)
      : name(name),
        defaultValue(defaultValue),
        buffer(nullptr),
        maxLength(DefaultMaxLength)
  {}

  explicit environment_variable_base(const char* name, const char* defaultValue, unsigned maxLength)
      : name(name),
        defaultValue(defaultValue),
        buffer(nullptr),
        maxLength(maxLength)
  {}

public:

  ~environment_variable_base() {
    delete[] buffer;
  }

  void cleanup() {
    delete[] buffer;
    buffer = nullptr;
  }
};

void environment::push_back(environment_variable_base* var) {
  if (variableCount == VariablesMaxCapacity) {
    fprintf_s(stderr, "Failed to add %s to the environment. Doing so would exceed the "
                      "maximum of %llu environment variables allowed.",
              var->name,
              VariablesMaxCapacity);
    abort();
  }
  var->index = variableCount;
  var->env = this;
  variables[variableCount] = var;
  ++variableCount;
}

char environment_variable_base::errorMessageBuffer[ErrorMsgMaxLength] = {};*/

namespace {
  class log_level_environment_variable : public jem::environment_variable<jem::log_level> {

    inline constexpr static char DebugString[]    = "debug";
    inline constexpr static char InfoString[]     = "info";
    inline constexpr static char WarningString[]  = "warning";
    inline constexpr static char ErrorString[]    = "error";
    inline constexpr static char DisabledString[] = "disabled";


    void convert_value(jem::log_level& lvl, std::string_view valueString) const override {
      if (valueString == DisabledString)
        lvl = jem::log_level::disabled;
      else if (valueString == ErrorString)
        lvl = jem::log_level::error;
      else if (valueString == WarningString)
        lvl = jem::log_level::warning;
      else if (valueString == InfoString)
        lvl = jem::log_level::info;
      else {
        JEM_assert( valueString == DebugString );
        lvl = jem::log_level::debug;
      }
    }

    void log_validation_error(std::string_view valString) const override {
      env->warning(R"e(Value of %1$s ( = "%2$s") is invalid. Valid values for %1$s
                       must be one of { "%3$s", "%s", "%s", "%s", "%s" })e",
                   name,
                   valString.data(),
                   DisabledString,
                   ErrorString,
                   WarningString,
                   InfoString,
                   DebugString);
    }

    bool validate_value(std::string_view valString) const override {
      return (valString == DisabledString) ||
             (valString == ErrorString) ||
             (valString == WarningString) ||
             (valString == InfoString) ||
             (valString == DebugString);
    }


  public:
    log_level_environment_variable(const char* name)
        : jem::environment_variable<jem::log_level>(name, DisabledString) {}
  };
  class kernel_init_mode_envvar : public jem::environment_variable<qtz_kernel_init_mode_t> {

    inline constexpr static char OpenAlwaysString[]   = "OPEN_ALWAYS";
    inline constexpr static char OpenExistingString[] = "OPEN_EXISTING";
    inline constexpr static char CreateNewString[]    = "CREATE_NEW";

    void convert_value(qtz_kernel_init_mode_t &val, std::string_view valString) const override {
      if (valString == OpenAlwaysString)
        val = QTZ_KERNEL_INIT_OPEN_ALWAYS;
      else if (valString == OpenExistingString)
        val = QTZ_KERNEL_INIT_OPEN_EXISTING;
      else {
        assert( valString == CreateNewString );
        val = QTZ_KERNEL_INIT_CREATE_NEW;
      }
    }

    void log_validation_error(std::string_view valueString) const override {
      env->warning(R"e(Value of %1$s ( = "%2$s") is invalid. Valid values for %1$s
                       must be one of { "%3$s", "%s", "%s" })e",
                   name,
                   valueString.data(),
                   OpenAlwaysString,
                   OpenExistingString,
                   CreateNewString);
    }

    bool validate_value(std::string_view valueString) const override {
      return (valueString == OpenAlwaysString) ||
             (valueString == OpenExistingString) ||
             (valueString == CreateNewString);
    }

  public:

    kernel_init_mode_envvar(const char* name) : jem::environment_variable<qtz_kernel_init_mode_t>(name, "OPEN_ALWAYS") { }
  };

  const char* qtzErrorMessage(qtz_status_t status) noexcept {
    switch (status) {
      case QTZ_SUCCESS: return "QTZ_SUCCESS";
      case QTZ_NOT_READY: return "QTZ_NOT_READY";
      case QTZ_DISCARDED: return "QTZ_DISCARDED";
      case QTZ_ERROR_TIMED_OUT: return "QTZ_ERROR_TIMED_OUT";
      case QTZ_ERROR_NOT_INITIALIZED: return "QTZ_ERROR_NOT_INITIALIZED";
      case QTZ_ERROR_INTERNAL: return "QTZ_ERROR_INTERNAL";
      case QTZ_ERROR_UNKNOWN: return "QTZ_ERROR_UNKNOWN";
      case QTZ_ERROR_BAD_SIZE: return "QTZ_ERROR_BAD_SIZE";
      case QTZ_ERROR_INVALID_KERNEL_VERSION: return "QTZ_ERROR_INVALID_KERNEL_VERSION";
      case QTZ_ERROR_INVALID_ARGUMENT: return "QTZ_ERROR_INVALID_ARGUMENT";
      case QTZ_ERROR_BAD_ENCODING_IN_NAME: return "QTZ_ERROR_BAD_ENCODING_IN_NAME";
      case QTZ_ERROR_NAME_TOO_LONG: return "QTZ_ERROR_NAME_TOO_LONG";
      case QTZ_ERROR_NAME_ALREADY_IN_USE: return "QTZ_ERROR_NAME_ALREADY_IN_USE";
      case QTZ_ERROR_INSUFFICIENT_BUFFER_SIZE: return "QTZ_ERROR_INSUFFICIENT_BUFFER_SIZE";
      case QTZ_ERROR_BAD_ALLOC: return "QTZ_ERROR_BAD_ALLOC";
      case QTZ_ERROR_NOT_IMPLEMENTED: return "QTZ_ERROR_NOT_IMPLEMENTED";
      case QTZ_ERROR_UNSUPPORTED_OPERATION: return "QTZ_ERROR_UNSUPPORTED_OPERATION";
      case QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE: return "QTZ_ERROR_IPC_SUPPORT_UNAVAILABLE";
    }
  }
}

template <>
struct jem::environment_variable_traits<jem::log_level>{
  using type = log_level_environment_variable;
  using value_type = jem::log_level;
};
template <>
struct jem::environment_variable_traits<qtz_kernel_init_mode_t>{
  using type = kernel_init_mode_envvar;
  using value_type = qtz_kernel_init_mode_t;
};








JEM_api BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {

  using namespace jem;

  static environment env{16};

  // Perform actions based on the reason for calling.
  switch( fdwReason ) {
    case DLL_PROCESS_ATTACH: {
      FILE* logFile;

      fopen_s(&logFile, env.get<std::string>("JEMSYS_LOG_FILE", "./jemsys.log").data(), "a");

      env.set_logging_file(logFile);
      env.set_log_format(env.get<std::string>("JEMSYS_LOG_FORMAT", "(jemsys)[$DAT, $TIM] => $LVL :: \"$MSG\"\n"));
      env.set_log_level(env.get<log_level>("JEMSYS_LOG_LEVEL"));

      char* programName;
      _get_pgmptr(&programName);


      qtz_init_params_t initParams{
        .kernel_version     = env.get<jem_u32_t>("JEMSYS_KERNEL_VERSION", "0"),
        .kernel_mode        = env.get<qtz_kernel_init_mode_t>("JEMSYS_KERNEL_INIT_MODE"),
        .kernel_access_code = env.get<std::string>("JEMSYS_KERNEL_ACCESS_CODE", "jemsys").data(),
        .message_slot_count = env.get<size_t>("JEMSYS_MAILBOX_SIZE", "8191"),
        .process_name       = env.get<std::string>("JEMSYS_PROCESS_NAME", programName).data(),
        .module_count       = 0,
        .modules            = nullptr
      };

      if (qtz_status_t status = qtz_init(&initParams)) {
        env.error("qtz_init failed with error code \"%s\"", qtzErrorMessage(status));
        return FALSE;
      }
      return TRUE;
    }
    case DLL_THREAD_ATTACH:
      // Do thread-specific initialization.
    case DLL_THREAD_DETACH:
      // Do thread-specific cleanup.
      break;
    case DLL_PROCESS_DETACH: {
      struct kill_request {
        size_t structLength;
        DWORD  exitCode;
      } killArgs{
        .structLength = sizeof(kill_request),
        .exitCode = 0
      };
      qtz_status_t killResult = qtz_wait(qtz_send(QTZ_THIS_PROCESS, QTZ_DEFAULT_PRIORITY, 1, &killArgs, JEM_WAIT), 5 * 1000 * 1000);
      if (!killResult) {
        env.error("Quartz kill request failed with status \"%s\"", qtzErrorMessage(killResult));
      }
    }
    break;
    JEM_no_default;
  }
  return TRUE;
}



# endif
#endif

