//
// Created by maxwe on 2021-06-12.
//

#include <agate/deputy.h>
#include <agate/message.h>

#include "ipc/segment.hpp"
#include "ipc/mpsc_mailbox.hpp"

#include <atomic>

using ipc::offset_ptr;


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <intrin.h>
#include <immintrin.h>


namespace agt::impl{

#if JEM_system_windows
  using native_thread_handle_t = void*;
  using native_thread_id_t     = unsigned long;
#else
  using native_thread_handle_t = int;
  using native_thread_id_t     = int;
#endif

  namespace {
    enum promise_control_code_t{
      ctrl_user_message,
      ctrl_cancel_operation,
      ctrl_kill_deputy,
      ctrl_map_deputy,
      ctrl_disconnect_deputy
    };


    inline thread_local agt_deputy_t bound_deputy = nullptr;
  }
}

extern "C" {

struct agt_deputy {

  virtual agt_promise_t do_send(jem_u32_t msgId, const void* message) noexcept = 0;

  JEM_forceinline void acquire_ref() noexcept {
    ref_count.fetch_add(1, std::memory_order_relaxed);
  }
  JEM_forceinline void release_ref(jem_u32_t refsReleased = 1) noexcept {
    if (!ref_count.fetch_sub(refsReleased, std::memory_order_seq_cst))
      this->do_destroy();
  }

protected:
  explicit agt_deputy(agt_module_t module, jem_u32_t initialRefCount = 1) noexcept
      : module(module),
        ref_count(initialRefCount) {}


  agt_module_t module;
  ipc::segment segment;

private:
  virtual void do_destroy() noexcept = 0;


  std::atomic<jem_u32_t> ref_count;
};

struct agt_promise {
  alignas(16) agt_deputy_t          sender;
  jem_u32_t                         msgId;
  agt::impl::promise_control_code_t ctrl;
  //std::byte    payload[];
};

}

namespace agt::impl{
  namespace {


    using PFN_send_to_deputy         = agt_promise_t(JEM_stdcall*)(agt_deputy_t deputy, jem_u32_t msgId, const void* msgData);
    using PFN_send_to_foreign_deputy = agt_promise_t(JEM_stdcall*)(agt_deputy_t deputy, jem_u32_t msgId, const void* msgData);


    using deputy_mailbox_t = ipc::mpsc_mailbox<agt_promise>;
    using deputy_slot_t    = deputy_mailbox_t::slot_type;


    inline static ipc::memory_desc lookup_message_memory_desc(agt_deputy_t thisDeputy, jem_u32_t ) noexcept;

    void send_user_message(deputy_slot_t* slot, jem_u32_t msgId, jem_size_t msgSize, const void* msgData) noexcept {
      slot->msgId  = msgId;
      slot->ctrl   = ctrl_user_message;
      slot->sender = bound_deputy;
      std::memcpy(((char*)slot) + sizeof(deputy_slot_t), msgData, msgSize);
    }
    void send_kill_deputy_signal(deputy_slot_t* slot) noexcept {
      slot->sender = bound_deputy;
      slot->ctrl = ctrl_kill_deputy;
    }
    void send_cancel_operation_signal(deputy_slot_t* slot, agt_promise_t promise) noexcept {

    }
    void send_disconnect_deputy_signal(deputy_slot_t* slot, agt_deputy_t deputy);


    inline ipc::memory_desc getPromiseMemoryReq(ipc::memory_desc msgDesc, jem_size_t& payloadOffset) noexcept {
      ipc::memory_desc promiseDesc = ipc::default_memory_requirements<agt_promise>;
      promiseDesc.alignment = std::max(promiseDesc.alignment, msgDesc.alignment);
      align_size(promiseDesc);
      payloadOffset = promiseDesc.size;
      promiseDesc.size += msgDesc.size;
      align_size(promiseDesc);
      return promiseDesc;
    }


    class thread_deputy final      : public agt_deputy{
      static DWORD JEM_stdcall threadProc(void* params) noexcept {
        auto* This = static_cast<thread_deputy*>(params);




        return This->exitCode;
      }
    public:

      thread_deputy(agt_module_t module, jem_size_t slotCount, ipc::memory_desc msgDesc) noexcept
          : agt_deputy(module),
            threadHandle(nullptr),
            threadId(),
            exitCode(0),
            inbox(slotCount, msgDesc){
        SECURITY_ATTRIBUTES secAttr{
          .nLength              = sizeof(SECURITY_ATTRIBUTES),
          .lpSecurityDescriptor = nullptr,
          .bInheritHandle       = true
        };
        threadHandle = CreateThread(&secAttr, 64 * 1024 * 4, threadProc, this, 0, (LPDWORD)&threadId);
      }

      agt_promise_t do_send(jem_u32_t msgId, const void *message) noexcept override {
        slot_t* slot = inbox.begin_write();
        send_user_message(slot, msgId, msgSize, message);
        inbox.end_write(slot);
        return slot;
      }
      void do_destroy() noexcept override {

      }

    private:

      using mailbox_t = ipc::mpsc_mailbox<agt_promise>;
      using slot_t    = mailbox_t::slot_type;

      agt_type_descriptor_t lookupMessageType(jem_u32_t msgId) noexcept;


      agt_message_proc_t             pfnMessageProc;
      void*                          pUserData;
      jem_size_t                     msgSize;
      jem_size_t                     payloadOffset;
      native_thread_handle_t         threadHandle;
      native_thread_id_t             threadId;
      DWORD                          exitCode;
      ipc::mpsc_mailbox<agt_promise> inbox;
    };
    class thread_pool_deputy final : public agt_deputy{
    public:

      explicit thread_pool_deputy(agt_module_t module, jem_u32_t threadCount) noexcept
          : agt_deputy(module),
            pool(CreateThreadpool(nullptr)),
            callbackEnvironment()
      {
        InitializeThreadpoolEnvironment(&callbackEnvironment);
        SetThreadpoolThreadMinimum(pool, threadCount);

      }

      agt_promise_t do_send(jem_u32_t msgId, const void *message) noexcept override {
        ThreadpoolPer
      }

      void do_destroy() noexcept override {
        CloseThreadpool(pool);
        DestroyThreadpoolEnvironment(&callbackEnvironment);
      }

    private:

      PTP_POOL pool;
      TP_CALLBACK_ENVIRON callbackEnvironment;

      UMS_THREAD_INFO_CLASS khbj;
    };
    class lazy_deputy final        : public agt_deputy{

    };
    class proxy_deputy final       : public agt_deputy{
      agt_message_filter_t pfnFilter;
      void*                pFilterUserData;
      jem_size_t           realDeputyCount;
      const agt_deputy_t*  pRealDeputies;
    public:
      agt_promise_t do_send(jem_u32_t msgId, const void *message) noexcept override {
        jem_u32_t result = pfnFilter(this, msgId, pFilterUserData) + 1;
        assert(result < realDeputyCount);
        return agt_send_to_deputy(pRealDeputies[result], msgId, message);
      }
    };
    class virtual_deputy final     : public agt_deputy{

    };

    class collective_deputy;
    class collective_marshal;

    class collective_deputy final  : public agt_deputy{

      collective_marshal* marshal;
      agt_module_t        module;
      agt_message_proc_t  pfnMessageProc;
      void*               pUserData;

    public:

      void do_destroy() noexcept override {

      }
      agt_promise_t do_send(jem_u32_t msgId, const void *message) noexcept override {

      }
    };

    class collective_marshal final {
      collective_deputy*     deputies;
      agt_deputy_t*          deputyList;
      native_thread_handle_t threadHandle;
      native_thread_id_t     threadId;
    };



    struct execution_context {
      agt_deputy_t deputy;
      /*CONTEXT      context;
      char*        inst_ptr;
      char*        frame_ptr;
*/
    };


    thread_deputy* make_thread_deputy(const agt_deputy_params_t* params) noexcept {

    }

  }
}

JEM_api agt_request_t JEM_stdcall agt_open_deputy(agt_deputy_t* pDeputy, const agt_deputy_params_t* pParams) {
  switch () {

  }
}

JEM_api void          JEM_stdcall agt_close_deputy(agt_deputy_t deputy) {
  deputy->release_ref();
}



JEM_api agt_promise_t JEM_stdcall agt_send_to_deputy(agt_deputy_t deputy, jem_u32_t msgId, const void* buffer) {
  return deputy->do_send(msgId, buffer);
}

JEM_api JEM_noreturn void JEM_stdcall agt_convert_thread_to_deputy(const agt_deputy_params_t* pParams) {}



