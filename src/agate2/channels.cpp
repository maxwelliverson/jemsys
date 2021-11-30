//
// Created by maxwe on 2021-11-03.
//

#include "internal.hpp"

namespace agt{

  struct channel {
    atomic_u32_t refCount;
    channel_kind kind;
    jem_size_t slotCount;
    jem_size_t slotSize;
  };

  struct local_channel : channel{
    std::byte* messageSlots;
  };

  struct shared_channel : channel {
    ipc::offset_ptr<std::byte> messageSlots;
  };

  struct private_mailbox : local_channel {
    jem_size_t availableSlotCount;
    jem_size_t queuedMessageCount;
    AgtMessage nextFreeSlot;
    AgtMessage prevReceivedMessage;
    AgtMessage prevQueuedMessage;
    bool       isSenderClaimed;
    bool       isReceiverClaimed;
  };

  struct local_spsc_channel : local_channel {
    binary_semaphore_t consumerSemaphore;
    binary_semaphore_t producerSemaphore;
    JEM_cache_aligned
      semaphore_t        slotSemaphore;
    AgtMessage         producerPreviousQueuedMsg;
    AgtMessage         producerNextFreeSlot;

    JEM_cache_aligned
      semaphore_t        queuedMessages;
    AgtMessage         consumerPreviousMsg;
    JEM_cache_aligned
      std::atomic<AgtMessage> lastFreeSlot;
  };
  struct local_mpsc_channel : local_channel {
    semaphore_t             slotSemaphore;
    JEM_cache_aligned
      std::atomic<AgtMessage> nextFreeSlot;

    JEM_cache_aligned
      std::atomic<AgtMessage> lastQueuedSlot;
    std::atomic<AgtMessage> lastFreeSlot;
    AgtMessage              previousReceivedMessage;
    JEM_cache_aligned
      mpsc_counter_t          queuedMessageCount;
    jem_u32_t               maxProducers;
    binary_semaphore_t      consumerSemaphore;
    semaphore_t             producerSemaphore;
  };
  struct local_spmc_channel : local_channel {
    semaphore_t        slotSemaphore;
    JEM_cache_aligned
      std::atomic<AgtMessage> lastFreeSlot;
    AgtMessage              nextFreeSlot;
    AgtMessage              lastQueuedSlot;
    JEM_cache_aligned
      std::atomic<AgtMessage> previousReceivedMessage;
    JEM_cache_aligned
      semaphore_t        queuedMessages;
    jem_u32_t          maxConsumers;
    binary_semaphore_t producerSemaphore;
    semaphore_t        consumerSemaphore;
  };
  struct local_mpmc_channel : local_channel {
    semaphore_t             slotSemaphore;
    JEM_cache_aligned
      std::atomic<AgtMessage> nextFreeSlot;
    std::atomic<AgtMessage> lastQueuedSlot;
    JEM_cache_aligned
      std::atomic<AgtMessage> previousReceivedMessage;
    JEM_cache_aligned
      semaphore_t             queuedMessages;
    jem_u32_t               maxProducers;
    jem_u32_t               maxConsumers;
    semaphore_t             producerSemaphore;
    semaphore_t             consumerSemaphore;
  };

  struct shared_spsc_channel : shared_channel {
    std::byte*         producerSlotAddr;
    std::byte*         consumerSlotAddr;
    JEM_cache_aligned
      semaphore_t        availableSlotSema;    // [1]: 8;  (0,   8)
    binary_semaphore_t producerSemaphore;    // [1]: 1;  (8,   9)
                                         // [1]: 7;  (9,  16) - alignment
    AgtMessage      producerPreviousMsg;  // [1]: 8;  (16, 24)
    JEM_cache_aligned
      semaphore_t        queuedMessageSema;    // [2]: 8;  (0,   8)
    binary_semaphore_t consumerSemaphore;    // [2]: 1;  (8,   9)
                                         // [2]: 7;  (9,  16) - alignment
    AgtMessage      consumerPreviousMsg;  // [2]: 8;  (16, 24)
    AgtMessage      consumerLastFreeSlot; // [2]: 8;  (24, 32)
    JEM_cache_aligned
      atomic_size_t      nextFreeSlotIndex;         // [3]: 8;  (0,   8)
    JEM_cache_aligned
      std::byte          slots[];
  };
  struct shared_mpsc_channel : shared_channel {
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
    JEM_cache_aligned
      std::byte          slots[];
  };
  struct shared_spmc_channel : shared_channel {
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
    JEM_cache_aligned
      std::byte          slots[];
  };
  struct shared_mpmc_channel : shared_channel {
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
    JEM_cache_aligned
      std::byte          slots[];
  };



  struct JEM_cache_aligned channel_vtable {
    AgtMessage (JEM_stdcall* pfnStage)(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void (      JEM_stdcall* pfnSend)(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage (JEM_stdcall* pfnRead)(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus ( JEM_stdcall* pfnConnect)(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void (      JEM_stdcall* pfnReturnMessage)(channel* channel, AgtMessage message) JEM_noexcept;
  };




  extern const channel_vtable vtable_table[];

  namespace impl{

    AgtMessage JEM_stdcall private_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall private_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall private_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall private_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall private_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall local_spsc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_spsc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall local_spsc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall local_spsc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_spsc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall local_mpsc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_mpsc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall local_mpsc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall local_mpsc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_mpsc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall local_spmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_spmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall local_spmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall local_spmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_spmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall local_mpmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_mpmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall local_mpmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall local_mpmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall local_mpmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall shared_spsc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_spsc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall shared_spsc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall shared_spsc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_spsc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall shared_mpsc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_mpsc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall shared_mpsc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall shared_mpsc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_mpsc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall shared_spmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_spmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall shared_spmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall shared_spmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_spmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

    AgtMessage JEM_stdcall shared_mpmc_stage(channel* channel, AgtSize messageSize, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_mpmc_send(channel* channel, AgtMessage message,  AgtSendFlags flags) JEM_noexcept;
    AgtMessage JEM_stdcall shared_mpmc_read(channel* channel, AgtTimeout timeoutUs) JEM_noexcept;
    AgtStatus  JEM_stdcall shared_mpmc_connect(channel* channel, agt_connect_action_t action, AgtTimeout timeoutUs) JEM_noexcept;
    void       JEM_stdcall shared_mpmc_return_message(channel* channel, AgtMessage message) JEM_noexcept;

  }

}

namespace {


  inline void cleanup_message(AgtMessage message) noexcept {
    message->signal.isReady.reset();
    message->signal.openHandles.store(1, std::memory_order_relaxed);
    if ( !(message->signal.flags.fetch_and_clear() & agt::message_fast_cleanup) ) {
      std::memset(message->payload, 0, message->payloadSize);
      message->sender = nullptr;
      message->id     = 0;
      message->signal.status = AGT_ERROR_STATUS_NOT_SET;
    }
  }
  inline void destroy_message(agt::channel* channel, AgtMessage message) noexcept {
    cleanup_message(message);
    agt::vtable_table[mailbox->kind].pfn_return_message(mailbox, message);
  }

  template <typename Mailbox>
  void return_message(Mailbox* mailbox, agt_cookie_t message) noexcept;

  template <> JEM_forceinline void return_message(agt::local_mpsc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::local_mpsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_spsc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::local_spsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_mpmc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::local_mpmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::local_spmc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::local_spmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_mpsc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::shared_mpsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_spsc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::shared_spsc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_mpmc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::shared_mpmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::shared_spmc_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::shared_spmc_return_message(mailbox, message);
  }
  template <> JEM_forceinline void return_message(agt::private_mailbox* mailbox, agt_cookie_t message) noexcept {
    agt::impl::private_return_message(mailbox, message);
  }

  inline void place_hold(AgtMessage message) noexcept {
    message->signal.flags.set(agt::message_in_use);
  }
  template <typename Mailbox>
  inline void free_hold(Mailbox* mailbox, AgtMessage message) noexcept {
    if ( message->signal.flags.fetch_and_reset(agt::message_in_use) & agt::message_is_condemned ) {
      cleanup_message(message);
      return_message(mailbox, message);
    }
  }

  inline void condemn(agt_mailbox_t mailbox, AgtMessage message) noexcept {
    if ( !(message->signal.flags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(mailbox, message);
  }
  inline void condemn(AgtMessage message) noexcept {
    if ( !(message->signal.flags.fetch_and_set(agt::message_is_condemned) & agt::message_in_use) )
      destroy_message(to_mailbox(message), message);
  }
  inline void condemn(agt_signal_t signal) noexcept {
    condemn(to_message(signal));
  }
}


extern "C" {

JEM_api AgtStatus     JEM_stdcall agtCreateChannel(const AgtChannelCreateInfo* cpCreateInfo, AgtSender* pSender, AgtReceiver* pReceiver) JEM_noexcept {

}

JEM_api AgtStatus     JEM_stdcall agtStageChannel(AgtSender sender, AgtStagedMessage* pStagedMessage, AgtSize messageSize, AgtTimeout usTimeout) JEM_noexcept {
  switch (sender->kind) {
    case AGT_OBJECT_KIND_RECEIVER:
      return AGT_ERROR_INVALID_HANDLE_TYPE;
    case AGT_OBJECT_KIND_SENDER: {
      auto channel = (agt::channel*)sender->object;
      if (messageSize > (channel->slotSize - sizeof(AgtMessage_st)))
        return AGT_ERROR_MESSAGE_TOO_LARGE;
      auto message = agt::vtable_table[channel->kind].pfnStage(channel, messageSize, usTimeout);
      if (!message)
        return AGT_TIMED_OUT;
      pStagedMessage->cookie = message;
      pStagedMessage->payload = message->payload;
      return AGT_SUCCESS;
    }
    case AGT_OBJECT_KIND_AGENT:
      return AGT_ERROR_NOT_YET_IMPLEMENTED;
    JEM_no_default;
  }
}
JEM_api AgtStatus     JEM_stdcall agtReceive(AgtReceiver receiver, AgtMessageInfo* pMessageInfo, AgtTimeout usTimeout) JEM_noexcept {

}

JEM_api void          JEM_stdcall agtSend(const AgtStagedMessage* cpStagedMessage, AgtAgent sender, AgtAsync asyncHandle, AgtSendFlags flags) JEM_noexcept {
  auto message = (AgtMessage)cpStagedMessage->payload;
  auto channel = to_channel(message);
  message->id = cpStagedMessage->id;
  message->sender = sender;
  // message->messageFlags.set(flags);
  if (asyncHandle != nullptr) {
    message->asyncHandle = asyncHandle->data;

  }
  agt::vtable_table[channel->kind].pfnSend(channel, message, flags);
}
JEM_api void          JEM_stdcall agtReturn(AgtMessage message, AgtStatus status) JEM_noexcept {

}


}