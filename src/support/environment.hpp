//
// Created by maxwe on 2021-10-22.
//

#ifndef JEMSYS_SUPPORT_ENVIRONMENT_HPP
#define JEMSYS_SUPPORT_ENVIRONMENT_HPP

#define JEM_SHARED_LIB_EXPORT

#include <jemsys.h>
#include <quartz.h>

#include <windows.h>
#include <cstdio>
#include <ctime>
#include <string_view>
#include <sstream>
#include <span>


namespace jem {

  template <typename T>
  concept trivially_parseable = requires (T& value, std::stringstream& sstr){
    sstr >> value;
  };




  enum class log_level{
    debug,
    info,
    warning,
    error,
    disabled
  };

  template <typename T>
  class environment_variable;

  template <typename T>
  struct environment_variable_traits {
    using type = environment_variable<T>;
    using value_type = T;
  };

  class environment_variable_base;

  class environment {

    template <typename T>
    using variable_t = typename environment_variable_traits<T>::type;

    constexpr static size_t VariablesMaxCapacity = 64;
    constexpr static size_t LogPrefixMaxLength = 64;
    constexpr static size_t LogBufferLength = /*(0x1 << 11)*/BUFSIZ;

    FILE*                       logFile;
    log_level                   logLevel;
    char                        logFormat[LogPrefixMaxLength];
    char                        logBuffer[LogBufferLength];
    size_t                      variableCount;
    size_t                      maxVariables;
    environment_variable_base** variables;
    char**                      defaultValues;
    bool                        logDateIsEnabled;
    bool                        logTimeIsEnabled;
    bool                        logLevelIsEnabled;
    bool                        logMessageIsEnabled;


    // friend class environment_variable_base;

    void push_back(environment_variable_base* var);

    static const char* get_level_string(log_level lvl) noexcept {
      switch (lvl) {
        case log_level::debug:   return "debug";
        case log_level::info:    return "info";
        case log_level::warning: return "warning";
        case log_level::error:   return "error";
        JEM_no_default;
      }
    }

    bool should_log(log_level lvl) const noexcept {
      return static_cast<std::underlying_type_t<log_level>>(logLevel) <=
             static_cast<std::underlying_type_t<log_level>>(lvl);
    }

    void write_log(log_level lvl, const char* fmt, va_list list) {


      char dateString[64] = {};
      char timeString[64] = {};
      const char* levelString = "";
      char messageBuffer[sizeof logBuffer];


      if (logDateIsEnabled || logTimeIsEnabled) {
        timespec ts;
        tm       time;
        timespec_get(&ts, TIME_UTC);
        localtime_s(&time, &ts.tv_sec);

        if (logDateIsEnabled) {
          strftime(dateString, sizeof dateString, "%D", &time);
        }
        if (logTimeIsEnabled) {
          strftime(timeString, sizeof timeString, "%T %Z", &time);
        }
      }

      if (logLevelIsEnabled)
        levelString = get_level_string(lvl);

      if (logMessageIsEnabled && vsprintf_s(messageBuffer, fmt, list)) {
        perror("jem::environment failed to parse log message");
        va_end(list);
        return;
      }
      va_end(list);

      fprintf_s(logFile, logFormat, dateString, timeString, levelString, messageBuffer);
    }

    void replace_format_substr(const char* replace, const char* with, bool& enabled) {
      const char* offset = strstr(logFormat, replace);
      enabled = offset != nullptr;
      if (enabled) {
        size_t index = offset - logFormat;
        strcpy_s(logFormat + index, LogPrefixMaxLength - index, with);
      }
    }


    void cleanup();

  public:

    environment(size_t maxVars)
        : logFile(nullptr),
          logLevel(log_level::disabled),
          logFormat(),
          logBuffer(),
          variableCount(0),
          maxVariables(maxVars),
          variables(new environment_variable_base*[maxVars]),
          defaultValues(new char*[maxVars]),
          logDateIsEnabled(false),
          logTimeIsEnabled(false),
          logLevelIsEnabled(false),
          logMessageIsEnabled(false)
    {}

    ~environment() {
      cleanup();
    }

    template <typename T>
    variable_t<T>* add(environment_variable<T>* envVar) {
      push_back(envVar);
      return static_cast<variable_t<T>*>(envVar);
    }
    template <typename T, typename ...Args>
    variable_t<T>* add(const char* name, Args&& ...args);

    template <typename T, typename ...Args>
    decltype(auto) get(const char* name, Args&& ...args) {
      auto var = add<T>(name, std::forward<Args>(args)...);
      return var->get_value();
    }

    void set_log_format(std::string_view fmt) noexcept {
      strcpy_s(logFormat, fmt.data());
      replace_format_substr("$DAT", "%1$s", logDateIsEnabled);
      replace_format_substr("$TIM", "%2$s", logTimeIsEnabled);
      replace_format_substr("$LVL", "%3$s", logLevelIsEnabled);
      replace_format_substr("$MSG", "%4$s", logMessageIsEnabled);
    }
    void set_logging_file(FILE* loggingFile) {
      if (logFile)
        fclose(logFile);
      logFile = loggingFile;
      if (logFile)
        std::setvbuf(loggingFile, logBuffer, _IOFBF, LogBufferLength);
      else
        logLevel = log_level::disabled;
    }
    void set_log_level(log_level lvl) {
      if (logFile != nullptr) {
        logLevel = lvl;
      }
    }

    void log(log_level lvl, const char* fmt, ...) {
      if (should_log(lvl)) {
        va_list list;
        va_start(list, fmt);
        write_log(lvl, fmt, list);
      }
    }

    void error(const char* fmt, ...) {
      if (should_log(log_level::error)) {
        va_list list;
        va_start(list, fmt);
        write_log(log_level::error, fmt, list);
      }
    }
    void warning(const char* fmt, ...) {
      if (should_log(log_level::warning)) {
        va_list list;
        va_start(list, fmt);
        write_log(log_level::warning, fmt, list);
      }
    }
    void info(const char* fmt, ...) {
      if (should_log(log_level::info)) {
        va_list list;
        va_start(list, fmt);
        write_log(log_level::info, fmt, list);
      }
    }
    void debug(const char* fmt, ...) {
      if (should_log(log_level::debug)) {
        va_list list;
        va_start(list, fmt);
        write_log(log_level::debug, fmt, list);
      }
    }

  };

  class environment_variable_base {

    friend class environment;

  protected:

    constexpr static size_t ErrorMsgMaxLength = (0x1 << 12);
    constexpr static unsigned DefaultMaxLength = 512;

    static char errorMessageBuffer[ErrorMsgMaxLength];

    environment*   env;
    size_t         index;
    const char*    name;
    const char*    defaultValue;
    size_t         defaultLength;
    mutable char*  buffer;
    mutable size_t valueLength;
    size_t         bufferCapacity;
    mutable bool   isDefined        = true;
    mutable bool   hasLookedUpValue = false;


    static size_t safe_strlen(const char* str) noexcept {
      if (str == nullptr)
        return 0;
      return strlen(str);
    }

    void log_value_not_found() const {
      env->info("Environment variable %s not defined", name);
    }
    void log_default_value_message() const {
      env->info("Using default value for environment variable %s ( = %s )", name, defaultValue);
    }
    void log_length_error_message() const {
      env->error("The value of the environment variable %s exceeds"
                   " the maximum allowed length ( = %u bytes )",
                   name,
                 bufferCapacity);
    }

    virtual void log_validation_error(std::string_view valueString) const { }

    virtual bool validate_value(std::string_view valueString) const {
      return true;
    }

    void doLookup() const {
      if (hasLookedUpValue)
        return;

      hasLookedUpValue = true;
      bool useDefaultValue = false;
      buffer = new char[bufferCapacity];
      DWORD length = GetEnvironmentVariableA(name, buffer, bufferCapacity);

      if (length == 0) {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
          log_value_not_found();
          useDefaultValue = true;
          isDefined = false;
        }
        *buffer = '\0';
      }
      else if (length == bufferCapacity) {
        log_length_error_message();
        useDefaultValue = true;
      }
      else if (std::string_view prospectiveValue{buffer, length}; !validate_value(prospectiveValue)) {
        log_validation_error(prospectiveValue);
        useDefaultValue = true;
      }
      else
        valueLength = length;




      if (useDefaultValue) {
        log_default_value_message();
        if (defaultValue)
          memcpy_s(buffer, bufferCapacity, defaultValue, defaultLength + 1);
        else
          doCleanup();
        valueLength = defaultLength;
      }
    }

    const char* lookup() const {
      doLookup();
      return buffer;
    }


    void doCleanup() const noexcept {
      delete[] buffer;
      buffer = nullptr;
    }

    explicit environment_variable_base(const char* name, const char* defaultValue)
        : env(nullptr),
          index(static_cast<size_t>(-1)),
          name(name),
          defaultValue(defaultValue),
          defaultLength(safe_strlen(defaultValue)),
          buffer(nullptr),
          valueLength(0),
          bufferCapacity(DefaultMaxLength)
    {}

    explicit environment_variable_base(const char* name, const char* defaultValue, unsigned maxLength)
        : env(nullptr),
          index(static_cast<size_t>(-1)),
          name(name),
          defaultValue(defaultValue),
          defaultLength(safe_strlen(defaultValue)),
          buffer(nullptr),
          valueLength(0),
          bufferCapacity(maxLength)
    {}

  public:

    virtual ~environment_variable_base() {
      delete[] buffer;
    }

    void cleanup() {
      doCleanup();
      isDefined = true;
      hasLookedUpValue = false;
    }
  };

  template <>
  class environment_variable<void> : public environment_variable_base {
  public:

    environment_variable(const char* name, const char* defaultValue)
        : environment_variable_base(name, defaultValue) { }

    bool is_defined() const {
      doLookup();
      return isDefined;
    }
  };

  template <>
  class environment_variable<const char*> : public environment_variable_base {
  public:

    environment_variable(const char* name, const char* defaultValue)
        : environment_variable_base(name, defaultValue) { }

    bool is_defined() const {
      doLookup();
      return isDefined;
    }

    const char* get_value() const {
      return lookup();
    }
  };

  template <>
  class environment_variable<std::string> : public environment_variable_base {
  public:

    environment_variable(const char* name, const char* defaultValue)
        : environment_variable_base(name, defaultValue) { }

    bool is_defined() const {
      doLookup();
      return isDefined;
    }

    std::string_view get_value() const {
      const char* value = lookup();
      return {value, valueLength};
    }
  };

  template <trivially_parseable T>
  class environment_variable<T> : public environment_variable_base {

    mutable T value;

  protected:

    virtual void convert_value(T& val, std::string_view valString) const {
      std::stringstream sstr;
      sstr << valString;
      sstr >> val;
    }

  public:

    environment_variable(const char* name, const char* defaultValue)
        : environment_variable_base(name, defaultValue) { }

    bool is_defined() const {
      doLookup();
      return isDefined;
    }

    const T& get_value() const {
      if (!hasLookedUpValue) {
        const char* valStr = lookup();
        convert_value(value, {valStr, valueLength});
      }
      return value;
    }
  };

  template <typename T>
  class environment_variable : public environment_variable_base {

    mutable T value;

  protected:

    virtual void convert_value(T& val, std::string_view valString) const = 0;

  public:

    environment_variable(const char* name, const char* defaultValue)
        : environment_variable_base(name, defaultValue) { }

    bool is_defined() const {
      doLookup();
      return isDefined;
    }

    const T& get_value() const {
      if (!hasLookedUpValue) {
        const char* strVal = lookup();
        convert_value(value, {strVal, valueLength});
      }
      return value;
    }
  };



  inline void environment::cleanup() {
    for (size_t i = 0; i < variableCount; ++i) {
      delete variables[i];
      free(defaultValues[i]);
    }
    for (auto var : std::span{variables, variableCount}) {
      delete var;
    }
    if (logFile)
      fclose(logFile);
    delete[] variables;
    delete[] defaultValues;
  }

  inline void environment::push_back(environment_variable_base* var) {
    if (variableCount == VariablesMaxCapacity) {
      error("Failed to add %s to the environment. Doing so would exceed "
            "the maximum allowed environment variables ( = %llu )",
            var->name,
            VariablesMaxCapacity);
      cleanup();
      abort();
    }
    char* newDefaultValue = nullptr;
    if (var->defaultValue != nullptr) {
      newDefaultValue = _strdup(var->defaultValue);
      var->defaultValue = newDefaultValue;
    }
    var->index = variableCount;
    var->env = this;
    variables[variableCount] = var;
    defaultValues[variableCount] = newDefaultValue;
    ++variableCount;
  }

  template <typename T, typename ...Args>
  inline environment::variable_t<T>* environment::add(const char* name, Args&& ...args) {
    return add(new variable_t<T>(name, std::forward<Args>(args)...));
  }
}




#endif//JEMSYS_SUPPORT_ENVIRONMENT_HPP
