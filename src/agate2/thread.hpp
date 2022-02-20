//
// Created by maxwe on 2022-02-19.
//

#ifndef JEMSYS_AGATE2_INTERNAL_THREAD_HPP
#define JEMSYS_AGATE2_INTERNAL_THREAD_HPP

#include "fwd.hpp"
#include "objects.hpp"

namespace Agt {

  namespace Impl {
    struct SystemThread {
      void* handle_;
      int   id_;
    };
  }


  struct Thread : HandleHeader {
    Impl::SystemThread sysThread;
  };

  struct LocalBlockingThread : HandleHeader {
    LocalMpScChannel* channel;
  };

  struct SharedBlockingThread : HandleHeader {
    SharedMpScChannelSender* channel;
  };

  struct ThreadPool : HandleHeader {
    AgtSize             threadCount;
    Impl::SystemThread* pThreads;
  };


  AgtStatus createInstance(LocalBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept;
  AgtStatus createInstance(SharedBlockingThread*& handle, AgtContext ctx, const AgtBlockingThreadCreateInfo& createInfo) noexcept;
}

#endif//JEMSYS_AGATE2_INTERNAL_THREAD_HPP
