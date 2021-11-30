//
// Created by maxwe on 2021-06-25.
//


#include "internal.hpp"

#include <optional>
#include <variant>
#include <string_view>

namespace {

  inline std::atomic_size_t default_slot_count = 1024;


  inline agt_message immediate_success_msg{
    .parent      = AGT_NULL_HANDLE,
    .flags       = agt::signal_result_is_ready,
    .status      = AGT_SUCCESS,
  };
  inline agt_message immediate_invalid_argument_msg{
    .parent      = AGT_NULL_HANDLE,
    .flags       = agt::signal_result_is_ready,
    .status      = AGT_ERROR_INVALID_ARGUMENT,
  };
  inline agt_message immediate_incompatible_params_msg{
    .parent      = AGT_NULL_HANDLE,
    .flags       = agt::signal_result_is_ready,
    .status      = AGT_ERROR_INCOMPATIBLE_PARAMETERS,
  };
  inline agt_message immediate_duplicate_params_msg{
    .parent      = AGT_NULL_HANDLE,
    .flags       = agt::signal_result_is_ready,
    .status      = AGT_ERROR_DUPLICATE_PARAMETERS,
  };


  enum open_handle_op_t{
    AGT_OPEN_HANDLE_UNKNOWN,
    AGT_OPEN_HANDLE_COPY_HANDLE,
    AGT_OPEN_HANDLE_OPEN_BY_NAME,
    AGT_OPEN_HANDLE_CREATE_MAILBOX,
    AGT_OPEN_HANDLE_CREATE_THREAD_DEPUTY,
    AGT_OPEN_HANDLE_CREATE_THREAD_POOL_DEPUTY,
    AGT_OPEN_HANDLE_CREATE_LAZY_DEPUTY,
    AGT_OPEN_HANDLE_CREATE_PROXY_DEPUTY,
    AGT_OPEN_HANDLE_CREATE_VIRTUAL_DEPUTY,
    AGT_OPEN_HANDLE_CREATE_COLLECTIVE_DEPUTY,
    AGT_OPEN_HANDLE_MAX_VALUE
  };
  enum agt_scope_t{
    AGT_SCOPE_UNKNOWN       = 0x0,
    AGT_SCOPE_LOCAL         = 0x1,
    AGT_SCOPE_DYNAMIC       = 0x2,
    AGT_SCOPE_DYNAMIC_LOCAL = AGT_SCOPE_LOCAL | AGT_SCOPE_DYNAMIC,
    AGT_SCOPE_IPC           = 0x4,
    AGT_SCOPE_DYNAMIC_IPC   = AGT_SCOPE_IPC | AGT_SCOPE_DYNAMIC
  };

  enum class agt_set : jem_u8_t{
    success,
    ignore,
    fail
  };

  bool try_set(open_handle_op_t& oldOp, open_handle_op_t newOp) noexcept {
    constexpr static agt_set S = agt_set::success;
    constexpr static agt_set I = agt_set::ignore;
    constexpr static agt_set F = agt_set::fail;
    constexpr static agt_set OpTable[AGT_OPEN_HANDLE_MAX_VALUE][AGT_OPEN_HANDLE_MAX_VALUE] = {
      { I, S, S, S, S, S, S, S, S, S },
      { I, I, F, F, F, F, F, F, F, F },
      { I, F, I, F, F, F, F, F, F, F },
      { I, F, I, I, S, S, S, F, F, F },
      { I, F, I, I, I, F, F, F, F, F },
      { I, F, I, I, F, I, F, F, F, F },
      { I, F, I, I, F, F, I, F, F, F },
      { I, F, I, I, F, F, F, I, F, F },
      { I, F, I, I, F, F, F, F, I, F },
      { I, F, I, I, F, F, F, F, F, I }
    };

    switch (OpTable[oldOp][newOp]) {
      case agt_set::success:
        oldOp = newOp;
      case agt_set::ignore:
        return true;
      case agt_set::fail:
        return false;
    }
  }


  using deputy_params_t = std::variant<agt_create_thread_deputy_params_t*,
                                       agt_create_thread_pool_deputy_params_t*,
                                       agt_create_lazy_deputy_params_t*,
                                       agt_create_proxy_deputy_params_t*,
                                       agt_create_virtual_deputy_params_t*,
                                       agt_create_collective_deputy_params_t*>;

  struct mailbox_params_t {
    jem_u32_t   maxProducers;
    jem_u32_t   maxConsumers;
    jem_size_t  defaultMessageSize;
    jem_size_t  maxMessageSize;
    jem_size_t  slotCount;
    const char* name;
    jem_size_t  nameLength;
    agt_cleanup_callback_t cleanupCallback;
    void*                  cleanupCallbackData;
    bool        isIpc;
    bool        isDynamic;
  };


  void destroy_object(agt_object* object) noexcept;


  agt_cookie_t create_mailbox(agt_handle_t& handle, const mailbox_params_t& params) noexcept;

  agt_cookie_t lookup_handle_by_name(agt_handle_t& handle, std::string_view name, bool isExplicitlyIpc) noexcept;

  struct create_deputy_t{
    agt_handle_t& handle;
    mailbox_params_t& mailboxParams;
    agt_cookie_t operator()(agt_create_thread_deputy_params_t* params) noexcept;
    agt_cookie_t operator()(agt_create_thread_pool_deputy_params_t* params) noexcept;
    agt_cookie_t operator()(agt_create_lazy_deputy_params_t* params) noexcept;
    agt_cookie_t operator()(agt_create_proxy_deputy_params_t* params) noexcept;
    agt_cookie_t operator()(agt_create_virtual_deputy_params_t* params) noexcept;
    agt_cookie_t operator()(agt_create_collective_deputy_params_t* params) noexcept;
  };

  jem_u32_t defaultMaxConsumers(open_handle_op_t) noexcept;
  jem_u32_t defaultMaxProducers(open_handle_op_t) noexcept;
}


extern "C" {

  JEM_api agt_cookie_t JEM_stdcall agt_open_handle(agt_handle_t* pHandle, const agt_open_handle_params_t* params) JEM_noexcept {





    if ( params == nullptr || pHandle == nullptr )
      return &immediate_invalid_argument_msg;

    open_handle_op_t                      op    = AGT_OPEN_HANDLE_UNKNOWN;
    std::optional<agt_handle_t>           baseHandle;
    std::optional<jem_size_t>             defaultMsgSize;
    std::optional<jem_size_t>             maxMsgSize;
    std::optional<jem_size_t>             slotCount;
    std::optional<jem_u32_t>              maxConsumers;
    std::optional<jem_u32_t>              maxProducers;
    std::optional<std::string_view>       name;
    std::optional<agt_cleanup_callback_t> cleanupCallback;
    void*                                 cleanupCallbackData = nullptr;

    std::optional<bool>                   isDynamic;
    std::optional<bool>                   isInterprocess;

    std::optional<deputy_params_t>        deputyParams;


#define AGT_emplace(var, ...) do { if ( var.has_value()) return &immediate_duplicate_params_msg; var.emplace(__VA_ARGS__); } while(false)
#define AGT_update_op(new_op) if ( !try_set(op, new_op) ) return &immediate_incompatible_params_msg
#define AGT_param(type) (static_cast<type*>(param.pointer))
#define AGT_default_value(var, ...) if (!var.has_value()) var.emplace(__VA_ARGS__)

    for ( jem_size_t i = 0; i != params->ext_param_count; ++i ) {
      auto param = params->ext_params[i];
      switch ( (agt_ext_param_type_t)param.type ) {
        case AGT_EXT_PARAM_TYPE_BASE_HANDLE:
          AGT_update_op(AGT_OPEN_HANDLE_COPY_HANDLE);
          AGT_emplace(baseHandle, param.handle);
          break;
        case AGT_EXT_PARAM_TYPE_NAME_PARAMS: {
          AGT_update_op(AGT_OPEN_HANDLE_OPEN_BY_NAME);
          auto&& name_params = AGT_param(agt_name_params_t);
          jem_size_t name_length = name_params->name_length ?: strlen(name_params->name);
          AGT_emplace(name, name_params->name, name_length);
        }
          break;
        case AGT_EXT_PARAM_TYPE_CLEANUP_PARAMS: {
          auto&& cleanup = AGT_param(agt_cleanup_params_t);
          AGT_emplace(cleanupCallback, cleanup->callback);
          cleanupCallbackData = cleanup->user_data;
        }
        break;
        case AGT_EXT_PARAM_TYPE_MAX_CONSUMERS:
          AGT_emplace(maxConsumers, param.u32);
          break;
        case AGT_EXT_PARAM_TYPE_MAX_PRODUCERS:
          AGT_emplace(maxProducers, param.u32);
          break;
        case AGT_EXT_PARAM_TYPE_IS_DYNAMIC:
          AGT_emplace(isDynamic, param.boolean);
          break;
        case AGT_EXT_PARAM_TYPE_IS_INTERPROCESS:
          AGT_emplace(isInterprocess, param.boolean);
          break;
        case AGT_EXT_PARAM_TYPE_SLOT_COUNT:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_MAILBOX);
          AGT_emplace(slotCount, param.u64);
          break;
        case AGT_EXT_PARAM_TYPE_MAX_MESSAGE_SIZE:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_MAILBOX);
          AGT_emplace(maxMsgSize, param.u64);
          break;
        case AGT_EXT_PARAM_TYPE_DEFAULT_MESSAGE_SIZE:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_MAILBOX);
          AGT_emplace(defaultMsgSize, param.u64);
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_THREAD_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_THREAD_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_thread_deputy_params_t));
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_THREAD_POOL_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_THREAD_POOL_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_thread_pool_deputy_params_t));
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_LAZY_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_LAZY_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_lazy_deputy_params_t));
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_PROXY_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_PROXY_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_proxy_deputy_params_t));
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_VIRTUAL_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_VIRTUAL_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_virtual_deputy_params_t));
          break;
        case AGT_EXT_PARAM_TYPE_CREATE_COLLECTIVE_DEPUTY_PARAMS:
          AGT_update_op(AGT_OPEN_HANDLE_CREATE_COLLECTIVE_DEPUTY);
          AGT_emplace(deputyParams, AGT_param(agt_create_collective_deputy_params_t));
          break;

      }
    }


    if ( isDynamic == false && !defaultMsgSize.has_value() )
      return &immediate_incompatible_params_msg;



    switch ( op ) {
      case AGT_OPEN_HANDLE_UNKNOWN:
        return &immediate_invalid_argument_msg;
      case AGT_OPEN_HANDLE_COPY_HANDLE:
        if ( baseHandle.value() == AGT_NULL_HANDLE )
          return &immediate_invalid_argument_msg;

        break;
      case AGT_OPEN_HANDLE_OPEN_BY_NAME:
        return lookup_handle_by_name(*pHandle, name.value(), isInterprocess.value_or(false));
      default: break;
    }


    std::string_view finalName = name.value_or("");


    mailbox_params_t mailboxParams{
      .maxProducers        = maxProducers.value_or(defaultMaxProducers(op)),
      .maxConsumers        = maxConsumers.value_or(defaultMaxConsumers(op)),
      .defaultMessageSize  = defaultMsgSize.value_or(0),
      .maxMessageSize      = maxMsgSize.value_or(std::numeric_limits<jem_size_t>::max()),
      .slotCount           = slotCount.value_or(default_slot_count.load()),
      .name                = finalName.data(),
      .nameLength          = finalName.length(),
      .cleanupCallback     = cleanupCallback.value_or(nullptr),
      .cleanupCallbackData = cleanupCallbackData,
      .isIpc               = isInterprocess.value_or(false),
      .isDynamic           = isDynamic.value_or(false),
    };

    if ( !deputyParams.has_value() )
      return create_mailbox(*pHandle, mailboxParams);

    return std::visit(create_deputy_t{ .handle = *pHandle, .mailboxParams = mailboxParams }, deputyParams.value());
  }
  JEM_api void          JEM_stdcall agt_close_handle(agt_handle_t handle) JEM_noexcept {
    if ( handle != AGT_NULL_HANDLE ) [[likely]] {
      if ( !(agt_internal_get_handle_flags(handle) & agt::handle_is_weak_ref) ) {
        agt_object* const object = get_handle_object(handle);
        if ( object->refCount.fetch_sub(1) == 1 ) {
          destroy_object(object);
        }
      }
    }
  }
}