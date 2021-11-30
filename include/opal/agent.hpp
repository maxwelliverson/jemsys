//
// Created by maxwe on 2021-10-25.
//

#ifndef JEMSYS_OPAL_AGENT_HPP
#define JEMSYS_OPAL_AGENT_HPP

#include <jemsys.h>

#include <memory>
#include <chrono>

namespace opal {

  enum class status {
    success,
    not_ready,
    deferred,
    cancelled,
    timed_out,
    error_unknown,
    error_status_not_set,
    error_unknown_message_type,
    error_invalid_flags,
    error_invalid_message,
    error_invalid_signal,
    error_cannot_discard,
    error_message_too_large,
    error_insufficient_slots,
    error_bad_size,
    error_invalid_argument,
    error_bad_encoding_in_name,
    error_name_too_long,
    error_name_already_in_use,
    error_bad_alloc,
    error_mailbox_is_full,
    error_mailbox_is_empty,
    error_too_many_senders,
    error_too_many_receivers,
    error_already_received,
    error_initialization_failed,
    error_not_yet_implemented
  };


  class agent;

  class cookie;


  class slot final {
    cookie*         m_cookie;
    jem_global_id_t m_id;
    void*           m_payload;

    friend class mailbox;

  public:
    JEM_forceinline void  set_message_id(jem_u64_t msgId) noexcept {
      m_id = msgId;
    }
    JEM_forceinline void* get_payload() const noexcept {
      return m_payload;
    }

    JEM_forceinline explicit operator bool() const noexcept {
      return m_cookie != nullptr;
    }
  };

  class signal final {
    struct sig_t;

    sig_t* m_sig;
  public:
    signal() noexcept : m_sig(nullptr){}
    signal(const signal& other) noexcept;
    signal(signal&& other) noexcept;

    ~signal() {
      if (m_sig)
        discard();
    }


    status wait(jem_u64_t timeout_us) noexcept;
    void   discard() noexcept;
  };

  class message final {

    cookie*         m_cookie;
    jem_size_t      m_size;
    jem_global_id_t m_id;
    void*           m_payload;

    friend class mailbox;
    friend class agent;

  public:

    JEM_forceinline jem_size_t size() const noexcept {
      return m_size;
    }
    JEM_forceinline jem_u64_t  id() const noexcept {
      return m_id;
    }
    JEM_forceinline void*      payload() const noexcept {
      return m_payload;
    }

    status get_sender(agent*& sender, void* asyncHandle = nullptr) const noexcept;

    JEM_forceinline void       reply(status status) noexcept;

    JEM_forceinline explicit operator bool() const noexcept {
      return m_cookie;
    }
  };

  class mailbox final {

    enum class action{
      attach_sender,
      detach_sender,
      attach_receiver,
      detach_receiver
    };

    class impl;

    impl* m_impl;


    status try_connect_for(action action, jem_u64_t timeout_us) noexcept;

    status try_receive_for(message& msg, jem_u64_t timeout_us) noexcept;
    status try_acquire_slot_for(slot& slot, jem_size_t slotSize, jem_u64_t timeout_us) noexcept;

  public:

    mailbox() noexcept : m_impl(nullptr){ }
    mailbox(const mailbox& other) noexcept;
    mailbox(mailbox&& other) noexcept : m_impl(std::exchange(other.m_impl, nullptr)){}

    ~mailbox() {
      if (m_impl)
        close();
    }



    void    attach_sender() noexcept {
      [[maybe_unused]] status result = this->try_connect_for(action::attach_sender, JEM_WAIT);
      JEM_assert(result == status::success);
    }
    bool    try_attach_sender() noexcept {
      return this->try_connect_for(action::attach_sender, JEM_DO_NOT_WAIT) == status::success;
    }
    template <typename Rep, typename Per>
    bool    try_attach_sender_for(const std::chrono::duration<Rep, Per>& timeout) noexcept {
      return this->try_connect_for(action::attach_sender, duration_cast<std::chrono::microseconds>(timeout).count()) == status::success;
    }
    template <typename Clock, typename Dur>
    bool    try_attach_sender_until(const std::chrono::time_point<Clock, Dur>& deadline) noexcept {
      return this->try_attach_sender_for(deadline - Clock::now());
    }
    void    detach_sender() noexcept {
      [[maybe_unused]] status result = this->try_connect_for(action::detach_sender, JEM_DO_NOT_WAIT);
      JEM_assert(result == status::success);
    }

    void    attach_receiver() noexcept {
      [[maybe_unused]] status result = this->try_connect_for(action::attach_receiver, JEM_WAIT);
      JEM_assert(result == status::success);
    }
    bool    try_attach_receiver() noexcept {
      return this->try_connect_for(action::attach_receiver, JEM_DO_NOT_WAIT) == status::success;
    }
    template <typename Rep, typename Per>
    bool    try_attach_receiver_for(const std::chrono::duration<Rep, Per>& timeout) noexcept {
      return this->try_connect_for(action::attach_receiver,
                                   duration_cast<std::chrono::microseconds>(timeout).count()) == status::success;
    }
    template <typename Clock, typename Dur>
    bool    try_attach_receiver_until(const std::chrono::time_point<Clock, Dur>& deadline) noexcept {
      return this->try_attach_receiver_for(deadline - Clock::now());
    }
    void    detach_receiver() noexcept {
      [[maybe_unused]] status result = this->try_connect_for(action::detach_receiver, JEM_DO_NOT_WAIT);
      JEM_assert(result == status::success);
    }



    slot   acquire_slot(jem_size_t slotSize) noexcept {
      slot slot;
      if (try_acquire_slot_for(slot, slotSize, JEM_WAIT) == status::error_message_too_large)
        return {};
      return slot;
    }
    status acquire_slot(slot& slot, jem_size_t slotSize) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, JEM_WAIT);
    }
    status try_acquire_slot(slot& slot, jem_size_t slotSize) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, JEM_DO_NOT_WAIT);
    }
    template <typename Rep, typename Per>
    status try_acquire_slot_for(slot& slot, jem_size_t slotSize, const std::chrono::duration<Rep, Per>& timeout) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
    }
    template <typename Clock, typename Dur>
    status try_acquire_slot_until(slot& slot, jem_size_t slotSize, const std::chrono::time_point<Clock, Dur>& deadline) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, deadline - Clock::now());
    }



    // message receive() noexcept;
    message  receive() noexcept {
      message msg;
      this->receive(msg);
      return msg;
    }
    void     receive(message& msg) noexcept {
      [[maybe_unused]] status result = try_receive_for(msg, JEM_WAIT);
      JEM_assert( result == status::success );
    }
    bool try_receive(message& msg) noexcept {
      status result = this->try_receive_for(msg, JEM_DO_NOT_WAIT);
      JEM_assert(result == status::success || result == status::error_mailbox_is_empty);
      return result == status::success;
    }
    template <typename Rep, typename Per>
    bool try_receive_for(message& msg, const std::chrono::duration<Rep, Per>& timeout) noexcept {
      status result = this->try_receive_for(msg, duration_cast<std::chrono::microseconds>(timeout).count());
      JEM_assert(result == status::success || result == status::error_mailbox_is_empty);
      return result == status::success;
    }
    template <typename Clock, typename Dur>
    bool try_receive_until(message& msg, const std::chrono::time_point<Clock, Dur>& deadline) noexcept {
      return this->try_receive_for(msg, deadline - Clock::now());
    }


    void    close() noexcept;
  };


  class actor {
  public:
    virtual ~actor() = default;

    virtual status process_message(const message& msg) noexcept = 0;
  };


  class agency;

  class agent {
    actor*                 m_actor;
    agency*                m_agency;

    virtual status try_acquire_slot_for(slot& slot, jem_size_t messageSize, jem_u64_t timeout_us) noexcept = 0;



  public:

    virtual ~agent() {
      close();
    }


    status acquire_slot(slot& slot, jem_size_t messageSize) noexcept {
      return this->try_acquire_slot_for(slot, messageSize, JEM_WAIT);
    }
    status try_acquire_slot(slot& slot, jem_size_t messageSize) noexcept {
      return this->try_acquire_slot_for(slot, messageSize, JEM_DO_NOT_WAIT);
    }
    template <typename Rep, typename Per>
    status try_acquire_slot_for(slot& slot, jem_size_t slotSize, const std::chrono::duration<Rep, Per>& timeout) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
    }
    template <typename Clock, typename Dur>
    status try_acquire_slot_until(slot& slot, jem_size_t slotSize, const std::chrono::time_point<Clock, Dur>& deadline) noexcept {
      return this->try_acquire_slot_for(slot, slotSize, deadline - Clock::now());
    }


    void close() noexcept;
  };

  class agency {
  public:

  };
}

#endif//JEMSYS_OPAL_AGENT_HPP
