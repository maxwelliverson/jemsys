//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_QUARTZ_MM_H
#define JEMSYS_QUARTZ_MM_H

#include "core.h"

JEM_begin_c_namespace

JEM_api qtz_request_t JEM_stdcall qtz_alloc_pages(jem_u32_t requestCount, void** ppPages, const jem_u32_t* pPageCounts);
JEM_api void          JEM_stdcall qtz_free_pages(jem_u32_t requestCount, void* const * ppPages, const jem_u32_t* pPageCounts);


JEM_api qtz_request_t JEM_stdcall qtz_alloc_object(void** pAddress, jem_size_t size);
JEM_api qtz_request_t JEM_stdcall qtz_alloc_array(void** pAddress, jem_size_t elementSize, jem_size_t count);
JEM_api qtz_request_t JEM_stdcall qtz_realloc_array(void** pAddress, jem_size_t elementSize, jem_size_t newCount, jem_size_t oldCount);
JEM_api void          JEM_stdcall qtz_free_object(void* address, jem_size_t size);
JEM_api void          JEM_stdcall qtz_free_array(void* address, jem_size_t elementSize, jem_size_t count);

JEM_api qtz_request_t JEM_stdcall qtz_alloc_aligned_object(void** pAddress, jem_size_t size, jem_size_t alignment);
JEM_api qtz_request_t JEM_stdcall qtz_alloc_aligned_array(void** pAddress, jem_size_t elementSize, jem_size_t count, jem_size_t alignment);
JEM_api qtz_request_t JEM_stdcall qtz_realloc_aligned_array(void** pAddress, jem_size_t elementSize, jem_size_t newCount, jem_size_t oldCount, jem_size_t alignment);
JEM_api void          JEM_stdcall qtz_free_aligned_object(void* address, jem_size_t size, jem_size_t alignment);
JEM_api void          JEM_stdcall qtz_free_aligned_array(void* address, jem_size_t elementSize, jem_size_t count, jem_size_t alignment);

JEM_end_c_namespace

#endif//JEMSYS_QUARTZ_MM_H
