//
// Created by maxwe on 2021-11-29.
//

#ifndef JEMSYS_AGATE2_SIGNAL_HPP
#define JEMSYS_AGATE2_SIGNAL_HPP

#include "fwd.hpp"
#include "wrapper.hpp"

namespace Agt {

  class Signal : public Wrapper<Signal, AgtSignal> {
  public:

    AsyncData getAttachment() const noexcept;

    void      attach(Async handle) const noexcept;

    void      detach() const noexcept;

    void      raise() const noexcept;

    void      close() const noexcept;

  };


}

#endif//JEMSYS_AGATE2_SIGNAL_HPP
