//
// Created by maxwe on 2021-10-28.
//

#ifndef JEMSYS_OPAL_COROUTINES_HPP
#define JEMSYS_OPAL_COROUTINES_HPP

#include "agent.hpp"

#if !JEM_compiler_clang && JEM_compiler_msvc
#include <coroutine>
#else
#define __cpp_lib_coroutine
#include <coroutine>
#undef __cpp_lib_coroutine
#endif
#include <concepts>

namespace opal {

  template <typename T>
  concept nonvoid = !std::same_as<std::remove_cv_t<T>, void>;

  template <typename T>
  class async_base {
  protected:
    struct promise_base {

      template <typename ...Args>
      promise_base(Args&& ...args) noexcept
          : m_value(std::forward<Args>(args)...){}

      T m_value;

      template <std::convertible_to<T> Arg>
      void return_value(Arg&& arg) noexcept {
        m_value = std::forward<Arg>(arg);
      }
    };
  public:

  };

  template <>
  class async_base<void> {
  protected:
    struct promise_base {
      void return_void() noexcept { }
    };
  public:
  };


  template <typename T, typename Class = void>
  class async : public async_base<T> {

    template <typename, typename>
    friend class async;

    using class_type = std::remove_reference_t<Class>;
  public:

    class promise_type : public async_base<T>::promise_base {
    public:

      template <typename ...Args>
      promise_type(class_type& object, Args&& ...args) noexcept
          : async_base<T>::promise_base(std::forward<Args>(args)...),
            m_this(std::addressof(object))
      {}

      async<T, Class>     get_return_object() noexcept {
        return {*this};
      }
      std::suspend_always initial_suspend() noexcept { return {}; }
      std::suspend_never  final_suspend() noexcept { return {}; }
      void                unhandled_exception() noexcept {}

      class_type* m_this;
    };




    auto operator co_await() const noexcept {
      struct awaitable {
        bool await_ready() const noexcept {
          return false;
        }
        void await_suspend(std::coroutine_handle<> handle) const noexcept {}
        void await_resume() const noexcept {}
      };
    }


  private:

    async(promise_type& promise) noexcept
        : m_handle(std::coroutine_handle<promise_type>::from_promise(promise)){

    }

    std::coroutine_handle<promise_type> m_handle;
  };

  template <typename T>
  class async<T, void> : public async_base<T> {
    template <typename, typename>
    friend class async;
  public:
    class promise_type : public async_base<T>::promise_base {
    public:

      template <typename ...Args>
      promise_type(Args&& ...args) noexcept
          : async_base<T>::promise_base(std::forward<Args>(args)...)
      {}

      async<T>            get_return_object() noexcept {
        return {*this};
      }
      std::suspend_always initial_suspend() noexcept { return {}; }
      std::suspend_never  final_suspend() noexcept { return {}; }
      void                unhandled_exception() noexcept {}
    };

  private:
    async(promise_type& promise) noexcept
        : m_handle(std::coroutine_handle<promise_type>::from_promise(promise)){

    }


    std::coroutine_handle<promise_type> m_handle;
  };


  template <typename T>
  struct request_promise_base {
    std::optional<T> optValue = std::nullopt;

    template <typename Arg>
    void return_value(Arg&& arg) noexcept {
      optValue.emplace(std::forward<Arg>(arg));
    }
    template <typename Arg>
    void yield_value(Arg&& arg) noexcept {
      optValue.emplace(std::forward<Arg>(arg));
    }
  };

  template <>
  struct request_promise_base<void> {
    void return_void() noexcept {}
  };



  template <typename T, typename Class, typename ...Args>
  class request {
  public:
    class promise_type : public request_promise_base<T> {

    };
  };


  template <typename T, typename Class>
  class agent_result : public async_base<T> {

    template <typename, typename>
    friend class agent_result;

    using class_type = std::remove_reference_t<Class>;
  public:

    class promise_type : public async_base<T>::promise_base {
    public:

      template <typename ...Args>
      promise_type(class_type& object, Args&& ...args) noexcept
          : async_base<T>::promise_base(std::forward<Args>(args)...),
            m_this(std::addressof(object))
      {}

      agent_result<T, Class> get_return_object() noexcept {
        return {*this};
      }
      std::suspend_always    initial_suspend() noexcept { return {}; }
      std::suspend_never     final_suspend() noexcept { return {}; }
      void                   unhandled_exception() noexcept {}

      class_type* m_this;
    };




    auto operator co_await() const noexcept {
      struct awaitable {
        bool await_ready() const noexcept {
          return false;
        }
        void await_suspend(std::coroutine_handle<> handle) const noexcept {}
        void await_resume() const noexcept {}
      };
    }


  private:

    agent_result(promise_type& promise) noexcept
        : m_handle(std::coroutine_handle<promise_type>::from_promise(promise)){

    }

    signal                              m_signal;
    std::coroutine_handle<promise_type> m_handle;
  };



  class task {
  public:
    class promise_type {
    public:
      task get_return_object() noexcept { return {}; }
      std::suspend_never initial_suspend() noexcept { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }
      void return_void() noexcept {}
      void unhandled_exception() noexcept {}
    };
  };

  template <nonvoid T>
  class lazy {
  public:
    class promise_type {
    public:
      lazy<T> get_return_object() noexcept { }
      std::suspend_always initial_suspend() noexcept {
        return {};
      }
      std::suspend_never  final_suspend() noexcept {
        return {};
      }

    };
  };

}

template <typename T, std::derived_from<opal::agent> C, typename ...Args>
struct std::coroutine_traits<T, C&, Args...> {
  using promise_type = typename opal::async<T, C&>::promise_type;
};

template <typename T, std::derived_from<opal::agent> C, typename ...Args>
struct std::coroutine_traits<T, const C&, Args...> {
  using promise_type = typename opal::async<T, const C&>::promise_type;
};


/*
template <typename T, std::derived_from<opal::agent> C, typename ...Args>
struct std::coroutine_traits<T, C&&, Args...> {
  using promise_type = typename opal::async<T, C&&>::promise_type;
};

template <typename T, std::derived_from<opal::agent> C, typename ...Args>
struct std::coroutine_traits<T, const C&&, Args...> {
  using promise_type = typename opal::async<T, const C&&>::promise_type;
};*/



#endif//JEMSYS_OPAL_COROUTINES_HPP
