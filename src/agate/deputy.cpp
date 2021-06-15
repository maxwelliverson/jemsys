//
// Created by maxwe on 2021-06-12.
//

#include <agate/deputy.h>


#include "../quartz/ipc/offset_ptr.hpp"

#include <atomic>

using qtz::ipc::offset_ptr;


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


struct agt_deputy{

  virtual agt_progress_t do_send(jem_u32_t msgId, const void* message) noexcept = 0;

  JEM_forceinline void acquire_ref() noexcept {
    ref_count.fetch_add(1, std::memory_order_relaxed);
  }
  JEM_forceinline void release_ref(jem_u32_t refsReleased = 1) noexcept {
    if ( !ref_count.fetch_sub(refsReleased, std::memory_order_seq_cst) )
      this->do_destroy();
  }

protected:

  explicit agt_deputy(agt_module_t module, jem_u32_t initialRefCount = 1) noexcept
      : module(module),
        ref_count(initialRefCount)
  {}




  agt_module_t           module;

private:

  virtual void do_destroy() noexcept = 0;


  std::atomic<jem_u32_t> ref_count;
};

namespace agt::impl{
  namespace {

#if JEM_system_windows
    using native_thread_handle_t = void*;
    using native_thread_id_t     = unsigned long;
#else
    using native_thread_handle_t = int;
    using native_thread_id_t     = int;
#endif


    class thread_deputy final      : public agt_deputy{
    public:

    private:
      native_thread_handle_t threadHandle;
      native_thread_id_t     threadId;
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

      agt_progress_t do_send(jem_u32_t msgId, const void *message) noexcept override {
        ThreadpoolPer
      }

    private:

      PTP_POOL pool;
      TP_CALLBACK_ENVIRON callbackEnvironment;

      _RTL_UMS_THREAD_INFO_CLASS jhvj;
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
      agt_progress_t do_send(jem_u32_t msgId, const void *message) noexcept override {
        jem_u32_t result = pfnFilter(this, msgId, pFilterUserData) + 1;
        assert(result < realDeputyCount);
        return agt_send_to_deputy(pRealDeputies[result], msgId, message);
      }
    };
    class virtual_deputy final     : public agt_deputy{

    };
    class collective_deputy final  : public agt_deputy{

    };


    struct execution_context{
      agt_deputy_t deputy;

    };
  }
}