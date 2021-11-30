//
// Created by maxwe on 2021-11-25.
//

#include "handle.hpp"
#include "vtable.hpp"

using namespace Agt;


AgtStatus Handle::duplicate(AgtHandle* pOut) const noexcept {
  if (isShared()) {
    SharedHandle* shared = asShared();
    if (AgtStatus result = shared->vtable->acquireRef(shared->object))
      return result;
    if (SharedHandle* pNewShared = getContext().newSharedHandle(shared)) [[likely]]
      *pOut = pNewShared;
    else
      return AGT_ERROR_BAD_ALLOC;
  }
  else {
    LocalObject* local = asLocal();
    if (AgtStatus result = local->vtable->acquireRef(local))
      return result;
    *pOut = value;
  }
  return AGT_SUCCESS;
}

void Handle::close() const noexcept  {



  if (isShared()) {
    auto shared = asShared();
    Id id = shared->localId;
    Context ctx = shared->context;
    ctx.release(shared->localId);

    if (shared->vtable->releaseRef(shared->object)) {
      ctx.releaseSharedObject(shared->object);
    }

    ctx.destroySharedHandle(shared);

  }
  else {
    auto local = asLocal();
    Id id = local->id;
    Context ctx = local->context;
    if (local->vtable->releaseRef(local)) {
      ctx.release(id);
    }
  }
}
