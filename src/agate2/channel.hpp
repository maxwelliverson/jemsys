//
// Created by maxwe on 2021-12-28.
//

#ifndef JEMSYS_AGATE2_CHANNEL_HPP
#define JEMSYS_AGATE2_CHANNEL_HPP

#include "fwd.hpp"
#include "objects.hpp"
#include "message.hpp"

#include <ipc/offset_ptr.hpp>


namespace Agt {

  struct PrivateChannelMessage;
  struct LocalChannelMessage;
  struct SharedChannelMessage;
  
  class LocalChannel      : public LocalObject {
  protected:

    AgtSize        slotCount;
    AgtSize        slotSize;
    AgtSize        inlineBufferSize;
    std::byte*     messageSlots;

    LocalChannel(ObjectType type, AgtContext ctx, Id id, AgtSize slotCount, AgtSize slotSize) noexcept
        : LocalObject(type, {}, ctx, id),
          slotCount(slotCount),
          slotSize(slotSize),
          messageSlots()
    {}



  public:

    virtual void releaseMessage(AgtMessage message) noexcept = 0;
  };
  
  class SharedChannel     : public SharedObject {
  protected:
    ReferenceCount             refCount;
    AgtSize                    slotCount;
    AgtSize                    slotSize;
    ipc::offset_ptr<std::byte> messageSlots;
  public:
    virtual void releaseMessage(AgtMessage message) noexcept = 0;
  };
  
  class PrivateChannel   final : public LocalChannel {
    AgtHandle*             consumer;
    AgtHandle*             producer;
    AgtSize                availableSlotCount;
    AgtSize                queuedMessageCount;
    PrivateChannelMessage* nextFreeSlot;
    PrivateChannelMessage* prevReceivedMessage;
    PrivateChannelMessage* prevQueuedMessage;
    AgtSize                refCount;

    JEM_forceinline void*  acquireSlot(AgtTimeout timeout) noexcept;
    JEM_noinline AgtStatus stageOutOfLine(StagedMessage& stagedMessage, AgtTimeout timeout) noexcept;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void      releaseMessage(AgtMessage message) noexcept override;
  };
  /*class PrivateChannel_StrictInline final : public PrivateChannel {
    AgtSize                availableSlotCount;
    AgtSize                queuedMessageCount;
    PrivateChannelMessage* nextFreeSlot;
    PrivateChannelMessage* prevReceivedMessage;
    PrivateChannelMessage* prevQueuedMessage;
    AgtHandle*             sender;
    AgtHandle*             receiver;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class PrivateChannel_OutOfLine    final : public PrivateChannel {
    AgtSize                availableSlotCount;
    AgtSize                queuedMessageCount;
    PrivateChannelMessage* nextFreeSlot;
    PrivateChannelMessage* prevReceivedMessage;
    PrivateChannelMessage* prevQueuedMessage;
    AgtHandle*             sender;
    AgtHandle*             receiver;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class PrivateChannel_MultiFrame   final : public PrivateChannel {
    AgtSize                availableSlotCount;
    AgtSize                queuedMessageCount;
    PrivateChannelMessage* nextFreeSlot;
    PrivateChannelMessage* prevReceivedMessage;
    PrivateChannelMessage* prevQueuedMessage;
    AgtHandle*             sender;
    AgtHandle*             receiver;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class PrivateChannel_External     final : public PrivateChannel {
    AgtSize                availableSlotCount;
    AgtSize                queuedMessageCount;
    PrivateChannelMessage* nextFreeSlot;
    PrivateChannelMessage* prevReceivedMessage;
    PrivateChannelMessage* prevQueuedMessage;
    AgtHandle*             sender;
    AgtHandle*             receiver;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };*/

  
  class LocalSpScChannel final : public LocalChannel {
  JEM_cache_aligned
    semaphore_t          slotSemaphore;
    LocalChannelMessage* producerPreviousQueuedMsg;
    LocalChannelMessage* producerNextFreeSlot;

  JEM_cache_aligned
    semaphore_t          queuedMessages;
    LocalChannelMessage* consumerPreviousMsg;
  JEM_cache_aligned
    ReferenceCount       refCount;
    LocalChannelMessage* lastFreeSlot;
    AgtHandle*           consumer;
    AgtHandle*           producer;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class LocalSpMcChannel final : public LocalChannel {
  JEM_cache_aligned
    semaphore_t        slotSemaphore;
    std::atomic<LocalChannelMessage*> lastFreeSlot;
    LocalChannelMessage*              nextFreeSlot;
    LocalChannelMessage*              lastQueuedSlot;
  JEM_cache_aligned
    std::atomic<LocalChannelMessage*> previousReceivedMessage;
  JEM_cache_aligned
    semaphore_t        queuedMessages;
    ReferenceCount       refCount;
    Handle*            producer;
    jem_u32_t          maxConsumers;
    semaphore_t        consumerSemaphore;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class LocalMpScChannel final : public LocalChannel {
  JEM_cache_aligned
    std::atomic<LocalChannelMessage*> nextFreeSlot;
    semaphore_t             slotSemaphore;
  JEM_cache_aligned
    std::atomic<LocalChannelMessage*> lastQueuedSlot;
    std::atomic<LocalChannelMessage*> lastFreeSlot;
    LocalChannelMessage*              previousReceivedMessage;
  JEM_cache_aligned
    mpsc_counter_t          queuedMessageCount;
    ReferenceCount          refCount;
    Handle*                 consumer;
    jem_u32_t               maxProducers;
    semaphore_t             producerSemaphore;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;
  };
  class LocalMpMcChannel final : public LocalChannel {
  JEM_cache_aligned
    std::atomic<LocalChannelMessage*> nextFreeSlot;
    std::atomic<LocalChannelMessage*> lastQueuedSlot;
  JEM_cache_aligned
    semaphore_t                       slotSemaphore;
    std::atomic<LocalChannelMessage*> previousReceivedMessage;

  JEM_cache_aligned
    semaphore_t             queuedMessages;
    jem_u32_t               maxProducers;
    jem_u32_t               maxConsumers;
    ReferenceCount          refCount;
    semaphore_t             producerSemaphore;
    semaphore_t             consumerSemaphore;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;

    void      releaseMessage(AgtMessage message) noexcept override;
  };

  
  class SharedSpScChannel : public SharedChannel {
    std::byte*         producerSlotAddr;
    std::byte*         consumerSlotAddr;
  JEM_cache_aligned
    semaphore_t        availableSlotSema;
    binary_semaphore_t producerSemaphore;
    AgtMessage         producerPreviousMsg;
  JEM_cache_aligned
    semaphore_t        queuedMessageSema;
    binary_semaphore_t consumerSemaphore;
    AgtMessage         consumerPreviousMsg;
    AgtMessage         consumerLastFreeSlot;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlotIndex;

  public:

    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class SharedSpMcChannel : public SharedChannel {
    semaphore_t        slotSemaphore;
    JEM_cache_aligned
      atomic_size_t      nextFreeSlot;
    JEM_cache_aligned
      atomic_size_t      lastQueuedSlot;
    JEM_cache_aligned                          // 192 - alignment
      atomic_u32_t       queuedSinceLastCheck; // 196
    jem_u32_t          minQueuedMessages;    // 200
    jem_u32_t          maxConsumers;         // 220
    binary_semaphore_t producerSemaphore;    // 221
                                         // 224 - alignment
    semaphore_t        consumerSemaphore;    // 240

  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class SharedMpScChannel : public SharedChannel {
    shared_semaphore_t slotSemaphore;
    JEM_cache_aligned
      std::atomic<AgtMessage> nextFreeSlot;
    jem_size_t                 payloadOffset;
    JEM_cache_aligned
      std::atomic<AgtMessage> lastQueuedSlot;
    AgtMessage              previousReceivedMessage;
    JEM_cache_aligned
      mpsc_counter_t     queuedMessageCount;
    jem_u32_t          maxProducers;
    binary_semaphore_t consumerSemaphore;
    semaphore_t        producerSemaphore;

  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class SharedMpMcChannel : public SharedChannel {
    semaphore_t      slotSemaphore;
  JEM_cache_aligned
    atomic_size_t    nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t    lastQueuedSlot;
  JEM_cache_aligned
    atomic_u32_t     queuedSinceLastCheck;
    jem_u32_t        minQueuedMessages;
    jem_u32_t        maxProducers;         // 220
    jem_u32_t        maxConsumers;         // 224
    semaphore_t      producerSemaphore;    // 240
    semaphore_t      consumerSemaphore;    // 256

  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };




  class PrivateChannelSender final : public LocalObject {
    PrivateChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class PrivateChannelReceiver final : public LocalObject {
    PrivateChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };

  class LocalSpScChannelSender final : public LocalObject {
    LocalSpScChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpScChannelSender : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class LocalSpScChannelReceiver final : public LocalObject {
    LocalSpScChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpScChannelReceiver : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };

  class LocalSpMcChannelSender final : public LocalObject {
    LocalSpMcChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpMcChannelSender : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class LocalSpMcChannelReceiver final : public LocalObject {
    LocalSpMcChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpMcChannelReceiver : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };

  class LocalMpScChannelSender final : public LocalObject {
    LocalMpScChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpScChannelSender : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class LocalMpScChannelReceiver final : public LocalObject {
    LocalMpScChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpScChannelReceiver : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };

  class LocalMpMcChannelSender final : public LocalObject {
    LocalMpMcChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpMcChannelSender : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };
  class LocalMpMcChannelReceiver final : public LocalObject {
    LocalMpMcChannel* channel;

  protected:

    AgtStatus localAcquire() noexcept override;
    void      localRelease() noexcept override;

  public:
    AgtStatus localStage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      localSend(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus localReceive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus localConnect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpMcChannelReceiver : public SharedObject {
    AgtSize channelOffset;
  public:
    static AgtStatus sharedAcquire(SharedObject* object, AgtContext ctx) noexcept;
    static AgtSize   sharedRelease(SharedObject* object, AgtContext ctx) noexcept;
    static void      sharedDestroy(SharedObject* object, AgtContext ctx) noexcept;
    static AgtStatus sharedStage(SharedObject* object, AgtContext ctx, AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept;
    static void      sharedSend(SharedObject* object, AgtContext ctx, AgtMessage message, AgtSendFlags flags) noexcept;
    static AgtStatus sharedReceive(SharedObject* object, AgtContext ctx, AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept;
    static AgtStatus sharedConnect(SharedObject* object, AgtContext ctx, Handle* otherHandle, ConnectAction action) noexcept;
  };


}

#endif//JEMSYS_AGATE2_CHANNEL_HPP
