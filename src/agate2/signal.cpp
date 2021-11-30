//
// Created by maxwe on 2021-11-29.
//

#include "signal.hpp"

#include "context.hpp"
#include "async.hpp"

using namespace Agt;

extern "C" {

struct AgtSignal_st {

  AsyncData asyncData;

  bool      isRaised;



};


}

AsyncData Signal::getAttachment() const noexcept {
  return value->asyncData;
}


void Signal::attach(Async handle) const noexcept {
  AsyncData data;
  if ((data = handle.getData())) {
    if (data.tryAttach(*this)) {
      goto setAttachment;
    }
    data.release();
  }
  data = handle.getContext().allocAsyncData();
  data.attach(*this);

  setAttachment:
  if (value->asyncData) {
    value->asyncData.detachSignal();
  }
  value->asyncData = data;
  value->isRaised = false;
}

void Signal::detach() const noexcept {
  if (value->asyncData) {
    value->asyncData.detachSignal();
    value->asyncData = AsyncData();
    value->isRaised = false;
  }
}

void Signal::raise() const noexcept {
  if (!value->isRaised) {
    value->asyncData.arrive();
    value->isRaised = true;
  }
}

void Signal::close() const noexcept {

}
