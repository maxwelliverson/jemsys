//
// Created by maxwe on 2021-10-30.
//

#include "rng.hpp"

#include <cuda/barrier>
#include <cuda/atomic>
#include <cuda/std/chrono>
#include <cooperative_groups.h>
#include <cooperative_groups/memcpy_async.h>


#include <curand.h>



namespace cg = cooperative_groups;

extern __shared__ unsigned           tmpResults[];
extern __shared__ unsigned           scratchU32[];
extern __shared__ float              scratchFloats[];
extern __shared__ double             scratchDoubles[];
extern __shared__ unsigned long long scratchU64[];

namespace {

  template <typename State>
  __global__ void generate_32bit_kernel(State* globalState,
                                        size_t n,
                                        size_t sharedMemorySize,
                                        CUdeviceptr out) noexcept {

    __shared__ cuda::barrier<cuda::thread_scope_block> barrier[1];

    size_t id     = cg::thread_block::thread_rank();
    unsigned scale = sharedMemorySize / cg::thread_block::size();
    unsigned iterations = n / sharedMemorySize;
    size_t offset = id * scale;
    auto outPtr = reinterpret_cast<char*>(out) + offset;

    if (id == 0)
      init(barrier, cg::thread_block::size());

    State localState = globalState[id];

    for (unsigned i = 0; i < iterations; ++i) {

      for (unsigned j = 0; j < scale; ++j)
        scratchU32[offset + j] = curand(&localState);

      cuda::memcpy_async(cg::this_thread_block(), outPtr + i * sharedMemorySize, scratchU32, scale, barrier[0]);

      barrier->arrive_and_wait();

      // cuda::memcpy_async(block, out, tmpResults, n, barrier);

    }

    globalState[id] = localState;
  }

  template <typename State>
  __global__ void generate_normal_kernel(State* globalState,
                                         size_t n,
                                         size_t sharedMemorySize,
                                         CUdeviceptr out) noexcept {

    __shared__ cuda::barrier<cuda::thread_scope_block> barrier[1];

    size_t id     = cg::thread_block::thread_rank();
    unsigned scale = sharedMemorySize / cg::thread_block::size();
    unsigned iterations = n / sharedMemorySize;
    size_t offset = id * scale;
    auto outPtr = reinterpret_cast<char*>(out) + offset;

    if (id == 0)
      init(barrier, cg::thread_block::size());

    State localState = globalState[id];

    for (unsigned i = 0; i < iterations; ++i) {

      for (unsigned j = 0; j < scale; ++j)
        scratchFloats[offset + j] = curand_normal(&localState);

      cuda::memcpy_async(cg::this_thread_block(), outPtr + i * sharedMemorySize, tmpResults, scale, barrier[0]);

      barrier->arrive_and_wait();

      // cuda::memcpy_async(block, out, tmpResults, n, barrier);

    }

    globalState[id] = localState;
  }

  template <typename State>
  __global__ void generate_uniform_kernel(State* globalState,
                                          size_t n,
                                          size_t sharedMemorySize,
                                          CUdeviceptr out) noexcept {

    __shared__ cuda::barrier<cuda::thread_scope_block> barrier[1];

    size_t id     = cg::thread_block::thread_rank();
    unsigned scale = sharedMemorySize / cg::thread_block::size();
    unsigned iterations = n / sharedMemorySize;
    size_t offset = id * scale;
    auto outPtr = reinterpret_cast<char*>(out) + offset;

    if (id == 0)
      init(barrier, cg::thread_block::size());

    State localState = globalState[id];

    for (unsigned i = 0; i < iterations; ++i) {

      for (unsigned j = 0; j < scale; ++j)
        scratchFloats[offset + j] = curand_uniform(&localState);

      cuda::memcpy_async(cg::this_thread_block(), outPtr + i * sharedMemorySize, tmpResults, scale, barrier[0]);

      barrier->arrive_and_wait();

      // cuda::memcpy_async(block, out, tmpResults, n, barrier);

    }

    globalState[id] = localState;
  }

  template <typename State>
  __global__ void generate_log_normal_kernel(State* globalState,
                                              float mean,
                                              float stddev,
                                              size_t n,
                                              size_t sharedMemorySize,
                                              CUdeviceptr out) noexcept {

    __shared__ cuda::barrier<cuda::thread_scope_block> barrier[1];

    size_t id     = cg::thread_block::thread_rank();
    unsigned scale = sharedMemorySize / cg::thread_block::size();
    unsigned iterations = n / sharedMemorySize;
    size_t offset = id * scale;
    auto outPtr = reinterpret_cast<char*>(out) + offset;

    if (id == 0)
      init(barrier, cg::thread_block::size());

    State localState = globalState[id];

    for (unsigned i = 0; i < iterations; ++i) {

      for (unsigned j = 0; j < scale; ++j)
        scratchFloats[offset + j] = curand_log_normal(&localState, mean, stddev);

      cuda::memcpy_async(cg::this_thread_block(), outPtr + i * sharedMemorySize, tmpResults, scale, barrier[0]);

      barrier->arrive_and_wait();

      // cuda::memcpy_async(block, out, tmpResults, n, barrier);

    }

    globalState[id] = localState;
  }

  void hostCallbackFn(void* arg) noexcept {
    auto msg = static_cast<agt_message_t*>(arg);
    auto args = static_cast<rng::refill_block_args*>(msg->payload);
    args->block->reset();
    agt_slot_t slot;
    agt_mailbox_acquire_slot(args->responseMailbox, &slot, sizeof(rng::return_block_args), JEM_WAIT);
    slot.id = 0;
    new(slot.payload) rng::return_block_args{
      .block = args->block
    };
    agt_send(&slot, args->serverSelf, AGT_IGNORE_RESULT);
    agt_return(msg, AGT_SUCCESS);
  }

  class xorwow_generator : public rng::generator {
    curandStateXORWOW_t* globalState;
  public:
    void generate_32bit(const rng::generate_args& args) noexcept override {
      generate_32bit_kernel<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
    }
    void generate_discrete(const rng::generate_args& args, curandDiscreteDistribution_t discreteDistribution) noexcept override {

    }

    void generate_poisson(const rng::generate_args& args, double lambda) noexcept override {

    }


    void generate_normal(const rng::generate_args& args) noexcept override {
      generate_normal_kernel<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
    }
    void generate_uniform(const rng::generate_args& args) noexcept override {
      generate_uniform_kernel<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
    }
    void generate_log_normal(const rng::generate_args& args, float mean, float stddev) noexcept override {
      generate_log_normal_kernel<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, mean, stddev, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
    }


    void generate_normal_double(const rng::generate_args& args) noexcept override {}
    void generate_uniform_double(const rng::generate_args& args) noexcept override {}
    void generate_log_normal_double(const rng::generate_args& args, double mean, double stddev) noexcept override {}
  };



  class xorwow_engine : public rng::random_engine {

    curandStateXORWOW_t* globalState;


  public:

    void refill(const rng::generate_args& args) noexcept override {
      rng::kernels::generate<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
      /*auto memcpyResult = cuMemcpyDtoHAsync(blk->base, deviceBuffer, bufferSize, stream);
      auto callbackResult = cuLaunchHostFunc(stream, hostCallbackFn, msg);*/
    }

    void anchor() noexcept override {}
  };

  class sobol32_engine : public rng::random_engine {

    curandStateSobol32_t* globalState;


  public:

    void refill(const rng::generate_args& args) noexcept override {
      rng::kernels::generate<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
      /*auto memcpyResult = cuMemcpyDtoHAsync(blk->base, deviceBuffer, bufferSize, stream);
      auto callbackResult = cuLaunchHostFunc(stream, hostCallbackFn, msg);*/
    }

    void anchor() noexcept override {}
  };

  class sobol64_engine : public rng::random_engine {

    curandStateSobol64_t* globalState;


  public:

    void refill(const rng::generate_args& args) noexcept override {
      rng::kernels::generate<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
      /*auto memcpyResult = cuMemcpyDtoHAsync(blk->base, deviceBuffer, bufferSize, stream);
      auto callbackResult = cuLaunchHostFunc(stream, hostCallbackFn, msg);*/
    }

    void anchor() noexcept override {}
  };

  class scrambled_sobol32_engine : public rng::random_engine {

    curandStateScrambledSobol32_t* globalState;


  public:

    void refill(const rng::generate_args& args) noexcept override {
      rng::kernels::generate<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
      /*auto memcpyResult = cuMemcpyDtoHAsync(blk->base, deviceBuffer, bufferSize, stream);
      auto callbackResult = cuLaunchHostFunc(stream, hostCallbackFn, msg);*/
    }

    void anchor() noexcept override {}
  };

  class scrambled_sobol64_engine : public rng::random_engine {

    curandStateScrambledSobol64_t* globalState;


  public:

    void refill(const rng::generate_args& args) noexcept override {
      rng::kernels::generate<<<args.blockSize, args.gridSize, args.sharedMemSize, args.stream>>>(globalState, args.bufferSize, args.sharedMemSize, args.deviceBuffer);
      /*auto memcpyResult = cuMemcpyDtoHAsync(blk->base, deviceBuffer, bufferSize, stream);
      auto callbackResult = cuLaunchHostFunc(stream, hostCallbackFn, msg);*/
    }

    void anchor() noexcept override {}
  };
}

void rng::generator::anchor() noexcept {}




template <typename State>
__global__ void rng::kernels::generate(State* globalState,
                                       size_t n,
                                       size_t sharedMemorySize,
                                       CUdeviceptr out) noexcept {

  __shared__ cuda::barrier<cuda::thread_scope_block> barrier[1];

  size_t id     = cg::thread_block::thread_rank();
  unsigned scale = sharedMemorySize / cg::thread_block::size();
  unsigned iterations = n / sharedMemorySize;
  size_t offset = id * scale;
  auto outPtr = reinterpret_cast<char*>(out) + offset;

  if (id == 0)
    init(barrier, cg::thread_block::size());

  State localState = globalState[id];

  for (unsigned i = 0; i < iterations; ++i) {

    for (unsigned j = 0; j < scale; ++j)
      tmpResults[offset + j] = curand(&localState);

    cuda::memcpy_async(cg::this_thread_block(), outPtr + i * sharedMemorySize, tmpResults, scale, barrier[0]);

    barrier->arrive_and_wait();

    // cuda::memcpy_async(block, out, tmpResults, n, barrier);

  }

  globalState[id] = localState;
}


template __global__ void rng::kernels::generate(curandStateXORWOW_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateSobol32_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateSobol64_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateScrambledSobol32_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateScrambledSobol64_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateMtgp32_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStateMRG32k3a_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;
template __global__ void rng::kernels::generate(curandStatePhilox4_32_10_t* globalState,
                                                size_t n,
                                                size_t sharedMemorySize,
                                                CUdeviceptr out) noexcept;





