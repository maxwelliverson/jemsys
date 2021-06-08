//
// Created by maxwe on 2021-06-02.
//

#ifndef JEMSYS_QUARTZ_EPOCH_H
#define JEMSYS_QUARTZ_EPOCH_H

#include "core.h"

JEM_begin_c_namespace

/**
 * Epoch subsystem
 *
 * Every Quartz resource has a lifetime, measured in epochs.
 * If a resource has a lifetime of 5 epochs, then it is guaranteed to be valid
 * for at least the next 5 epochs. Resource lifetimes can be extended by calling
 * qtz_extend_lifetime or qtz_ensure_lifetime. Extending a resource lifetime by
 * N epochs adds N epochs to the previous lifetime. Ensuring a lifetime
 * by N epochs sets the lifetime to N if it was previously less.
 *
 * The global epoch is incremented by calling qtz_advance_epoch.
 *
 * A resource with a lifetime of 0 is considered to be on "death row". It is not
 * valid to access a resource on death row, though its lifetime can still be
 * extended/ensured. Users can "recover" a resource on death row by extending or
 * ensuring its lifetime, at which point it becomes accessible once more. If a
 * resource remains on death row for a globally defined number of epochs without
 * being recovered, it is said to be abandoned. Abandoned resources are lost and
 * have no way of being recovered. Resources may not be made available to the
 * system again until they are abandoned.
 *
 * A resource can always be manually freed, whereby the resource is immediately
 * abandoned. Resources may also be pinned, removing them from the lifetime
 * tracking subsystem. Pinned resources effectively have an unlimited lifetime,
 * and the only way for them to be abandoned is for them to be manually freed.
 *
 *
 *
 * */


JEM_api void         JEM_stdcall qtz_advance_epoch();

/**
 * Informs the Quartz runtime that the specified memory regions must remain valid for
 * at least the next N epochs, where N is either the corresponding value in pNEpochs,
 * or 0 if pNEpochs is null.
 *
 * \param [in] ranges is the number of memory ranges to touch
 * \param [in] pRangeAddresses is a pointer to an array of memory addresses
 * \param [in] pRangeSizes is a pointer to an array of memory sizes
 * \param [in] pNEpochs is an optional pointer to an array of epoch counts
 * */
JEM_api jem_status_t JEM_stdcall qtz_touch_memory_ranges(jem_u32_t ranges, void* const * pRangeAddresses, const jem_size_t* pRangeSizes, const jem_u32_t* pNEpochs);




JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_EPOCH_H
