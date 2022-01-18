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


  class PrivateChannel;
  class LocalSpScChannel;
  class LocalSpMcChannel;
  class LocalMpScChannel;
  class LocalMpMcChannel;
  class SharedSpScChannel;
  class SharedSpMcChannel;
  class SharedMpScChannel;
  class SharedMpMcChannel;

  class SharedSpScChannelHandle;
  class SharedSpMcChannelHandle;
  class SharedMpScChannelHandle;
  class SharedMpMcChannelHandle;

  class PrivateChannelSender;
  class LocalSpScChannelSender;
  class LocalSpMcChannelSender;
  class LocalMpScChannelSender;
  class LocalMpMcChannelSender;
  class SharedSpScChannelSender;
  class SharedSpMcChannelSender;
  class SharedMpScChannelSender;
  class SharedMpMcChannelSender;

  class PrivateChannelReceiver;
  class LocalSpScChannelReceiver;
  class LocalSpMcChannelReceiver;
  class LocalMpScChannelReceiver;
  class LocalMpMcChannelReceiver;
  class SharedSpScChannelReceiver;
  class SharedSpMcChannelReceiver;
  class SharedMpScChannelReceiver;
  class SharedMpMcChannelReceiver;




  
  class LocalChannel      : public LocalObject {
  protected:

    AgtSize        slotCount;
    AgtSize        inlineBufferSize;
    std::byte*     messageSlots;

    LocalChannel(AgtStatus& status, ObjectType type, AgtContext ctx, AgtObjectId id, AgtSize slotCount, AgtSize slotSize) noexcept;
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

    PrivateChannel(AgtStatus& status, AgtContext ctx, AgtObjectId id, AgtSize slotCount, AgtSize slotSize) noexcept;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    void      releaseMessage(AgtMessage message) noexcept override;

    static AgtStatus createInstance(PrivateChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, PrivateChannelSender* sender, PrivateChannelReceiver* receiver) noexcept;
  };

  
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


    JEM_forceinline LocalChannelMessage* acquireSlot() noexcept;
    JEM_noinline AgtStatus stageOutOfLine(StagedMessage& stagedMessage, AgtTimeout timeout) noexcept;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;

    static AgtStatus createInstance(LocalSpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpScChannelSender* sender, LocalSpScChannelReceiver* receiver) noexcept;
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


    JEM_forceinline LocalChannelMessage* acquireSlot() noexcept;
    JEM_noinline AgtStatus stageOutOfLine(StagedMessage& stagedMessage, AgtTimeout timeout) noexcept;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;

    static AgtStatus createInstance(LocalSpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalSpMcChannelSender* sender, LocalSpMcChannelReceiver* receiver) noexcept;
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


    JEM_forceinline LocalChannelMessage* acquireSlot() noexcept;
    JEM_noinline AgtStatus stageOutOfLine(StagedMessage& stagedMessage, AgtTimeout timeout) noexcept;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    void releaseMessage(AgtMessage message) noexcept override;

    static AgtStatus createInstance(LocalMpScChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpScChannelSender* sender, LocalMpScChannelReceiver* receiver) noexcept;
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


    JEM_forceinline LocalChannelMessage* acquireSlot() noexcept;
    JEM_noinline AgtStatus stageOutOfLine(StagedMessage& stagedMessage, AgtTimeout timeout) noexcept;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    void      releaseMessage(AgtMessage message) noexcept override;

    static AgtStatus createInstance(LocalMpMcChannel*& outHandle, AgtContext ctx, const AgtChannelCreateInfo& createInfo, LocalMpMcChannelSender* sender, LocalMpMcChannelReceiver* receiver) noexcept;
  };

  
  class SharedSpScChannel final : public SharedObject {

    friend class SharedSpScChannelHandle;
    friend class SharedSpScChannelSender;
    friend class SharedSpScChannelReceiver;

    ReferenceCount     refCount;
    AgtSize            slotCount;
    AgtSize            slotSize;
    AgtObjectId        messageSlotsId;

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
    std::byte*         producerSlotAddr;
    std::byte*         consumerSlotAddr;
  };
  class SharedSpMcChannel final : public SharedObject {

    friend class SharedSpMcChannelHandle;
    friend class SharedSpMcChannelSender;
    friend class SharedSpMcChannelReceiver;

    ReferenceCount     refCount;
    AgtSize            slotCount;
    AgtSize            slotSize;
    AgtObjectId        messageSlotsId;
    semaphore_t        slotSemaphore;
  JEM_cache_aligned
    atomic_size_t      nextFreeSlot;
  JEM_cache_aligned
    atomic_size_t      lastQueuedSlot;
  JEM_cache_aligned
    atomic_u32_t       queuedSinceLastCheck;
    jem_u32_t          minQueuedMessages;
    jem_u32_t          maxConsumers;
    binary_semaphore_t producerSemaphore;
    semaphore_t        consumerSemaphore;

  };
  class SharedMpScChannel final : public SharedObject {

    friend class SharedMpScChannelHandle;
    friend class SharedMpScChannelSender;
    friend class SharedMpScChannelReceiver;

    ReferenceCount refCount;
    AgtSize        slotCount;
    AgtSize        slotSize;
    AgtObjectId    messageSlotsId;
    semaphore_t    slotSemaphore;
  JEM_cache_aligned
    std::atomic<AgtMessage> nextFreeSlot;
    jem_size_t              payloadOffset;
  JEM_cache_aligned
    std::atomic<AgtMessage> lastQueuedSlot;
    AgtMessage              previousReceivedMessage;
  JEM_cache_aligned
    mpsc_counter_t     queuedMessageCount;
    jem_u32_t          maxProducers;
    binary_semaphore_t consumerSemaphore;
    semaphore_t        producerSemaphore;

  };
  class SharedMpMcChannel final : public SharedObject {

    friend class SharedMpMcChannelHandle;
    friend class SharedMpMcChannelSender;
    friend class SharedMpMcChannelReceiver;

    ReferenceCount   refCount;
    AgtSize          slotCount;
    AgtSize          slotSize;
    AgtObjectId      messageSlotsId;
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
  };




  class PrivateChannelSender final : public LocalObject {

    friend class PrivateChannel;

    PrivateChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(PrivateChannelSender*& objectRef, AgtContext ctx) noexcept;

  };
  class PrivateChannelReceiver final : public LocalObject {

    friend class PrivateChannel;

    PrivateChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(PrivateChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  };

  class LocalSpScChannelSender final : public LocalObject {

    friend class LocalSpScChannel;

    LocalSpScChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalSpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  };
  class LocalSpScChannelReceiver final : public LocalObject {

    friend class LocalSpScChannel;

    LocalSpScChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalSpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  };

  class LocalSpMcChannelSender final : public LocalObject {

    friend class LocalSpMcChannel;

    LocalSpMcChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalSpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  };
  class LocalSpMcChannelReceiver final : public LocalObject {

    friend class LocalSpMcChannel;

    LocalSpMcChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalSpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  };

  class LocalMpScChannelSender final : public LocalObject {

    friend class LocalMpScChannel;

    LocalMpScChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalMpScChannelSender*& objectRef, AgtContext ctx) noexcept;
  };
  class LocalMpScChannelReceiver final : public LocalObject {

    friend class LocalMpScChannel;

    LocalMpScChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalMpScChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  };

  class LocalMpMcChannelSender final : public LocalObject {

    friend class LocalMpMcChannel;

    LocalMpMcChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalMpMcChannelSender*& objectRef, AgtContext ctx) noexcept;
  };
  class LocalMpMcChannelReceiver final : public LocalObject {

    friend class LocalMpMcChannel;

    LocalMpMcChannel* channel;

  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static AgtStatus createInstance(LocalMpMcChannelReceiver*& objectRef, AgtContext ctx) noexcept;
  };


  class SharedSpScChannelHandle final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static SharedSpScChannelHandle* createInstance(AgtContext ctx, const AgtChannelCreateInfo& createInfo) noexcept;
  };
  class SharedSpScChannelSender final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpScChannelReceiver final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };

  class SharedSpMcChannelHandle final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static SharedSpMcChannelHandle* createInstance(AgtContext ctx, const AgtChannelCreateInfo& createInfo) noexcept;
  };
  class SharedSpMcChannelSender final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedSpMcChannelReceiver final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };

  class SharedMpScChannelHandle final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static SharedMpScChannelHandle* createInstance(AgtContext ctx, const AgtChannelCreateInfo& createInfo) noexcept;
  };
  class SharedMpScChannelSender final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpScChannelReceiver final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };

  class SharedMpMcChannelHandle final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;

    static SharedMpMcChannelHandle* createInstance(AgtContext ctx, const AgtChannelCreateInfo& createInfo) noexcept;
  };
  class SharedMpMcChannelSender final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };
  class SharedMpMcChannelReceiver final : public SharedHandle {
  protected:

    AgtStatus acquire() noexcept override;
    void      release() noexcept override;

  public:
    AgtStatus stage(AgtStagedMessage& pStagedMessage, AgtTimeout timeout) noexcept override;
    void      send(AgtMessage message, AgtSendFlags flags) noexcept override;
    AgtStatus receive(AgtMessageInfo& pMessageInfo, AgtTimeout timeout) noexcept override;
    AgtStatus connect(Handle* otherHandle, ConnectAction action) noexcept override;
  };






}

#endif//JEMSYS_AGATE2_CHANNEL_HPP
