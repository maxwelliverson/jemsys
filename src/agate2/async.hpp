//
// Created by maxwe on 2021-11-28.
//

#ifndef JEMSYS_AGATE2_ASYNC_HPP
#define JEMSYS_AGATE2_ASYNC_HPP

#include "fwd.hpp"
#include "wrapper.hpp"

namespace Agt {

  enum class AsyncType {
    eUnbound,
    eDirect,
    eMessage,
    eSignal
  };
  enum class AsyncState {
    eEmpty,
    eBound,
    eReady
  };
  enum class AsyncDataState {
    eUnbound,
    eNotReady,
    eReady,
    eReadyAndRecyclable
  };

  enum class AsyncFlags : AgtUInt32 {
    eUnbound  = 0x0,
    eBound    = 0x1,
    eReady    = 0x2,
    eReusable = 0x4
  };

  JEM_forceinline constexpr AsyncFlags operator~(AsyncFlags a) noexcept {
    return static_cast<AsyncFlags>(~static_cast<AgtUInt32>(a));
  }
  JEM_forceinline constexpr AsyncFlags operator&(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) & static_cast<AgtUInt32>(b));
  }
  JEM_forceinline constexpr AsyncFlags operator|(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) | static_cast<AgtUInt32>(b));
  }
  JEM_forceinline constexpr AsyncFlags operator^(AsyncFlags a, AsyncFlags b) noexcept {
    return static_cast<AsyncFlags>(static_cast<AgtUInt32>(a) ^ static_cast<AgtUInt32>(b));
  }


  inline constexpr static AsyncFlags eAsyncUnbound          = AsyncFlags::eUnbound;
  inline constexpr static AsyncFlags eAsyncBound            = AsyncFlags::eBound;
  inline constexpr static AsyncFlags eAsyncNotReady         = AsyncFlags::eBound;
  inline constexpr static AsyncFlags eAsyncReady            = AsyncFlags::eBound | AsyncFlags::eReady;
  inline constexpr static AsyncFlags eAsyncReadyAndReusable = AsyncFlags::eReady | AsyncFlags::eReusable;


  class AsyncData : public Wrapper<AsyncData, AgtAsyncData> {
  public:

    AsyncType      getType() const noexcept;

    void           reset(AgtUInt32 expectedCount, Message message) const noexcept;
    void           reset(Signal signal) const noexcept;

    void           attach(Signal signal) const noexcept;

    bool           tryAttach(Signal signal) const noexcept;

    void           detachSignal() const noexcept;

    void           arrive() const noexcept;

    AgtUInt32      getMaxExpectedCount() const noexcept;

    AsyncFlags     wait(AgtUInt32 expectedCount, AgtTimeout timeout) const noexcept;


    static AsyncData create(Context ctx) noexcept;

  };

  AsyncType asyncDataGetType(const AgtAsyncData_st* asyncData) noexcept;

  void      asyncDataReset(AgtAsyncData data, AgtUInt32 expectedCount, AgtMessage message) noexcept;
  void      asyncDataReset(AgtAsyncData data, AgtSignal signal) noexcept;

  void      asyncDataAttach(AgtAsyncData data, AgtSignal signal) noexcept;
  bool      asyncDataTryAttach(AgtAsyncData data, AgtSignal signal) noexcept;

  void      asyncDataDetachSignal(AgtAsyncData data) noexcept;

  void      asyncDataAttach(AgtAsyncData data) noexcept;

  AgtUInt32 asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept;

  AsyncFlags asyncDataWait(AgtAsyncData data, AgtUInt32 expectedCount, AgtTimeout timeout) noexcept;

  AgtAsyncData createAsyncData(AgtContext context) noexcept;


  class Async : public Wrapper<Async, AgtAsync> {
  public:

    Context   getContext() const noexcept;

    AsyncData getData() const noexcept;
    // void      setData(AsyncData data) const noexcept;

    void      copyTo(Async other) const noexcept;
    void      clear() const noexcept;
    void      destroy() const noexcept;

    AgtStatus wait(AgtTimeout timeout) const noexcept;


    static Async create(Context ctx) noexcept;

  };




  AgtContext   asyncGetContext(const AgtAsync_st* async) noexcept;
  AgtAsyncData asyncGetData(const AgtAsync_st* async) noexcept;

  void         asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync_st* toAsync) noexcept;
  void         asyncClear(AgtAsync async) noexcept;
  void         asyncDestroy(AgtAsync async) noexcept;

  AgtStatus    asyncWait(AgtAsync async, AgtTimeout timeout) noexcept;

  AgtAsync     createAsync(AgtContext context) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
