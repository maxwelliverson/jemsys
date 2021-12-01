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




  /*class AsyncData : public Wrapper<AsyncData, AgtAsyncData> {
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

  };*/



  bool         asyncDataAttach(AgtAsyncData asyncData, AgtContext ctx, AgtUInt32& key) noexcept;
  void         asyncDataArrive(AgtAsyncData asyncData, AgtContext ctx, AgtUInt32 key) noexcept;
  // AsyncFlags   asyncDataWait(AgtAsyncData data, AgtUInt32 expectedCount, AgtTimeout timeout) noexcept;

  // AgtAsyncData createAsyncData(AgtContext context) noexcept;



  AgtUInt32    asyncDataGetEpoch(const AgtAsyncData_st* asyncData) noexcept;

  AsyncType    asyncDataGetType(const AgtAsyncData_st* asyncData) noexcept;

  // void         asyncDataReset(AgtAsyncData data, AgtUInt32 expectedCount, AgtMessage message) noexcept;
  // void         asyncDataReset(AgtAsyncData data, AgtSignal signal) noexcept;

  void         asyncDataAttach(AgtAsyncData data, AgtSignal signal) noexcept;
  bool         asyncDataTryAttach(AgtAsyncData data, AgtSignal signal) noexcept;

  void         asyncDataDetachSignal(AgtAsyncData data) noexcept;

  AgtUInt32    asyncDataGetMaxExpectedCount(const AgtAsyncData_st* data) noexcept;






  AgtContext   asyncGetContext(const AgtAsync_st* async) noexcept;
  AgtAsyncData asyncGetData(const AgtAsync_st* async) noexcept;

  void         asyncCopyTo(const AgtAsync_st* fromAsync, AgtAsync toAsync) noexcept;
  void         asyncClear(AgtAsync async) noexcept;
  void         asyncDestroy(AgtAsync async) noexcept;

  AgtAsyncData asyncAttach(AgtAsync async, AgtSignal) noexcept;

  void         asyncReset(AgtAsync async, AgtUInt32 targetExpectedCount, AgtUInt32 maxExpectedCount) noexcept;

  AgtStatus    asyncWait(AgtAsync async, AgtTimeout timeout) noexcept;

  AgtAsync     createAsync(AgtContext context) noexcept;

}

#endif//JEMSYS_AGATE2_ASYNC_HPP
