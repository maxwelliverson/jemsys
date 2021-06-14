//
// Created by maxwe on 2021-06-12.
//

#include <agate/deputy.h>



#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


struct agt_deputy{

  agt_deputy() = default;




protected:


private:
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

    private:

      _RTL_UMS_THREAD_INFO_CLASS jhvj;
      UMS_THREAD_INFO_CLASS khbj;
      TP_CALLBACK_ENVIRON jdbf;
      PTP_CALLBACK_INSTANCE instance;
    };
    class lazy_deputy final        : public agt_deputy{

    };
    class proxy_deputy final       : public agt_deputy{

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