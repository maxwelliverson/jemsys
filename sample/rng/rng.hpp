//
// Created by maxwe on 2021-10-30.
//

#ifndef JEMSYS_SAMPLE_RNG_HPP
#define JEMSYS_SAMPLE_RNG_HPP

#include "agate.h"

#include <curand.h>
#include <cuda.h>
#include <curand_kernel.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <memory>

#include <new>
#include <span>

namespace rng {
  enum message_type {
    init_blocks_message,
    refill_block_message,
    init_graph_message
  };

  struct block {
    block* nextBlock;
    char*  base;
    char*  top;
    char*  head;


    void next(void* toAddress, size_t bytes) noexcept {
      JEM_assert(top - head >= bytes);
      std::memcpy(toAddress, head, bytes);
      head += bytes;
    }
    bool try_next(void* toAddress, size_t bytes) noexcept {
      if (empty()) [[unlikely]]
        return false;
      std::memcpy(toAddress, head, bytes);
      head += bytes;
      return true;
    }
    bool empty() const noexcept {
      return head == top;
    }
    void reset() noexcept {
      head = base;
    }
  };
  struct queue {
    block*  front;
    block*  back;
    size_t  queueSize;

    block*  array;
    size_t  arraySize;
    size_t  blockSize;

    block* peek() const noexcept {
      if (empty()) [[unlikely]]
        return nullptr;
      return front;
    }
    block* pop() noexcept {
      block* result;
      if (empty()) [[unlikely]]
        return nullptr;
      result = front;
      front = front->nextBlock;
      --queueSize;
      return result;
    }
    void   push(block* blk) noexcept {
      if (empty()) [[unlikely]]
        front = blk;
      else
        back->nextBlock = blk;

      back = blk;
      ++queueSize;
    }
    bool   empty() const noexcept {
      return queueSize == 0;
    }
  };

  struct state;

  struct init_blocks_args {
    block*        blocks;
    size_t        blockCount;
    size_t        blockSize;
    CUdeviceptr*  buffer;
    CUstream      stream;
    agt_mailbox_t responseMailbox;
  };
  struct refill_block_args {
    block*            block;
    curandGenerator_t generator;
    CUstream          stream;
    CUdeviceptr       buffer;
    agt_agent_t       serverSelf;
    agt_mailbox_t     responseMailbox;
  };
  struct init_graph_args {
    block*        block;
    size_t        blockSize;
    CUstream      stream;
    CUgraphExec*  pGraph;
  };

  struct return_block_args {
    block* block;
  };

  struct state {
    agt_agent_t       self;
    curandGenerator_t generator;
    CUstream          stream;
    agt_agent_t       server;
    agt_mailbox_t     inbox;
    message_type      messageId;
    CUdeviceptr       deviceBuffer;
    queue             q;

    void init(size_t blockSize, size_t blockCount) noexcept {
      agt_create_mailbox_params_t mailboxParams{
        .min_slot_count = blockCount,
        .min_slot_size  = JEM_CACHE_LINE,
        .max_senders    = 1,
        .max_receivers  = 1,
        .scope          = AGT_MAILBOX_SCOPE_LOCAL,
        .async_handle_address = nullptr,
        .name           = {}
      };
      agt_status_t status = agt_create_mailbox(&inbox, &mailboxParams);
      JEM_assert( status == AGT_SUCCESS );
      curandStatus_t curandStatus = curandCreateGenerator(&generator, CURAND_RNG_PSEUDO_DEFAULT);
      JEM_assert( curandStatus == CURAND_STATUS_SUCCESS );
      CUresult result = cuStreamCreate(&stream, 0);
      JEM_assert( result == CUDA_SUCCESS );
      curandStatus = curandSetStream(generator, stream);
      JEM_assert( curandStatus == CURAND_STATUS_SUCCESS );

      q.array = new block[blockCount];
      q.arraySize = blockCount;
      q.blockSize = blockSize;
      q.queueSize = 0;

      for (auto& block : std::span<block>{ q.array, q.arraySize }) {
        q.push(&block);
      }

      agt_slot_t initMsg;

      agt_agent_acquire_slot(server, &initMsg, sizeof(init_blocks_args), JEM_WAIT);
      initMsg.id = init_blocks_message;

      new(initMsg.payload) init_blocks_args{
        .blocks = q.array,
        .blockCount = blockCount,
        .blockSize = blockSize,
        .buffer    = &deviceBuffer,
        .stream = stream,
        .responseMailbox = inbox
      };

      agt_send(&initMsg, self, AGT_IGNORE_RESULT);
    }

    block* receive_new_block() noexcept {
      agt_message_t msg;
      agt_mailbox_receive(inbox, &msg, JEM_WAIT);
      do {
        auto ret = static_cast<return_block_args*>(msg.payload);
        q.push(ret->block);
      } while(agt_mailbox_receive(inbox, &msg, JEM_DO_NOT_WAIT) == AGT_SUCCESS);
      return q.peek();
    }
    block* next_block() noexcept {
      agt_slot_t slot;
      block*     blk = q.pop();
      agt_agent_acquire_slot(server, &slot, sizeof(refill_block_args), JEM_WAIT);
      slot.id = messageId;
      new(slot.payload) refill_block_args{
        .block = blk,
        .generator = generator,
        .stream = stream,
        .buffer = deviceBuffer,
        .serverSelf = server,
        .responseMailbox = inbox
      };
      agt_send(&slot, self, AGT_IGNORE_RESULT);
      return q.peek();
    }

    void any_next(void* toAddress, jem_size_t bytes) noexcept {
      block* blk = q.peek();
      if (!blk) [[unlikely]]
        blk = receive_new_block();
      else if (blk->empty()) [[unlikely]] {
        blk = next_block();
        if (!blk) [[unlikely]]
          blk = receive_new_block();
      }
      blk->next(toAddress, bytes);
    }

    template <typename T>
    T next() noexcept {
      T value;
      any_next(&value, sizeof(T));
      return value;
    }
  };

  struct server {

    int deviceOrdinal;
    CUdevice device;





    void create_graph() noexcept {
      CUgraph graph;

      CUgraphNode memAllocNode;
      CUgraphNode generateNode;
      CUgraphNode memcpyNode;
      CUgraphNode callbackNode;
      CUgraphNode memFreeNode;

      CUDA_MEM_ALLOC_NODE_PARAMS allocNodeParams;

      cuGraphCreate(&graph, 0);
      cuGraphAddMemAllocNode(&memAllocNode, graph, nullptr, 0, );
    }
    void init_graph(agt_message_t* message) noexcept {

      auto args = static_cast<init_graph_args*>(message->payload);

      size_t blockSize = args->blockSize;

      CUgraph graph;

      CUfunction generateFunction;
      void*      generateParams;

      CUgraphNode memAllocNode;
      CUgraphNode generateNode;
      CUgraphNode memcpyNode;
      CUgraphNode callbackNode;
      CUgraphNode memFreeNode;


      CUmemAccessDesc memAccessDesc{
        .location = {
          .type = CU_MEM_LOCATION_TYPE_DEVICE,
          .id   = deviceOrdinal
        },
        .flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE
      };

      CUDA_MEM_ALLOC_NODE_PARAMS allocNodeParams{
        .poolProps = {
          .allocType = CU_MEM_ALLOCATION_TYPE_PINNED,
          .handleTypes = CU_MEM_HANDLE_TYPE_NONE,
          .location    = {
            .type = CU_MEM_LOCATION_TYPE_DEVICE,
            .id   = deviceOrdinal
          },
          .win32SecurityAttributes = nullptr
        },
        .accessDescs = &memAccessDesc,
        .accessDescCount = 1,
        .bytesize        = blockSize
      };

      CUfunction func;             /**< Kernel to launch */
      unsigned int gridDimX;       /**< Width of grid in blocks */
      unsigned int gridDimY;       /**< Height of grid in blocks */
      unsigned int gridDimZ;       /**< Depth of grid in blocks */
      unsigned int blockDimX;      /**< X dimension of each thread block */
      unsigned int blockDimY;      /**< Y dimension of each thread block */
      unsigned int blockDimZ;      /**< Z dimension of each thread block */
      unsigned int sharedMemBytes; /**< Dynamic shared-memory size per thread block in bytes */
      void **kernelParams;         /**< Array of pointers to kernel parameters */
      void **extra;                /**< Extra options */



      cuOccupancyMaxActiveBlocksPerMultiprocessor();

      curand();
      curand_discrete()

        CUDA_KERNEL_NODE_PARAMS kernelNodeParams{
          .func = /**/nullptr,
          .gridDimX  = 1,
          .gridDimY  = 1,
          .gridDimZ  = 1,
          .blockDimX = 64,
          .blockDimY = 64,
          .blockDimZ = 1,
          .sharedMemBytes = 0,

          .extra = nullptr
        };

      cuGraphCreate(&graph, 0);
      cuGraphAddMemAllocNode(&memAllocNode, graph, nullptr, 0, &allocNodeParams);
      cuGraphAddKernelNode(&generateNode, graph, &memAllocNode, 1);
    }

    void init_blocks(agt_message_t* msg) noexcept {
      auto args = static_cast<init_blocks_args*>(msg->payload);

    }
    void refill_block(agt_message_t* msg) noexcept {
      auto args = static_cast<refill_block_args*>(msg->payload);
      auto outputPtr = reinterpret_cast<unsigned int*>(args->buffer);
      size_t blockSize = args->block->top - args->block->base;
      size_t num = blockSize / sizeof(unsigned int);

      curandStatus_t status = curandGenerate(args->generator, outputPtr, num);
      CUresult memcpyResult = cuMemcpyDtoHAsync(args->block->base, args->buffer, blockSize, args->stream);

      CUresult callbackResult = cuLaunchHostFunc(args->stream, [](void* arg){
          auto msg = static_cast<agt_message_t*>(arg);
          auto args = static_cast<refill_block_args*>(msg->payload);
          args->block->reset();
          agt_slot_t slot;
          agt_mailbox_acquire_slot(args->responseMailbox, &slot, sizeof(return_block_args), JEM_WAIT);
          slot.id = 0;
          new(slot.payload) return_block_args{
            .block = args->block
          };
          agt_send(&slot, args->serverSelf, AGT_IGNORE_RESULT);
          agt_return(msg, AGT_SUCCESS);
        }, msg);
    }

  };


  template <typename T>
  struct buffer{

    buffer<T>* nextBuffer;
    T* base;
    T* head;
    T* top;

    T    next() noexcept {
      JEM_assert(top != head);
      return *head++;
    }
    bool empty() const noexcept {
      return head == top;
    }
    void reset() noexcept {
      head = base;
    }
  };

  template <typename T>
  struct buffer_queue {
    buffer<T>* front;
    buffer<T>* back;
    size_t     queueSize;

    buffer<T>* array;
    size_t     arraySize;
    size_t     blockSize;

    buffer<T>* peek() const noexcept {
      if (empty()) [[unlikely]]
        return nullptr;
      return front;
    }
    buffer<T>* pop() noexcept {
      buffer<T>* result;
      if (empty()) [[unlikely]]
        return nullptr;
      result = front;
      front = front->nextBuffer;
      --queueSize;
      return result;
    }
    void       push(buffer<T>* blk) noexcept {
      if (empty()) [[unlikely]]
        front = blk;
      else
        back->nextBuffer = blk;

      back = blk;
      ++queueSize;
    }
    bool       empty() const noexcept {
      return queueSize == 0;
    }
  };

  class rng_state {
  public:

  };

  struct generate_args {
    // agt_message_t* message;
    size_t         bufferSize;
    CUdeviceptr    deviceBuffer;
    // void*          hostBuffer;
    CUstream       stream;
    unsigned       blockSize;
    unsigned       gridSize;
    size_t         sharedMemSize;
  };

  class random_engine {
  public:

    virtual ~random_engine() = default;

    virtual void refill(const generate_args& args) noexcept = 0;

    virtual void anchor() noexcept;
  };

  class generator {
  public:

    virtual ~generator() = default;

    virtual void generate_32bit(const generate_args& args) noexcept = 0;
    virtual void generate_64bit(const generate_args& args) noexcept {
      generate_32bit(args);
    }

    virtual void generate_poisson(const generate_args& args, double lambda) noexcept = 0;
    virtual void generate_discrete(const generate_args& args, curandDiscreteDistribution_t discreteDistribution) noexcept = 0;


    virtual void generate_normal(const generate_args& args) noexcept = 0;
    virtual void generate_uniform(const generate_args& args) noexcept = 0;
    virtual void generate_log_normal(const generate_args& args, float mean, float stddev) noexcept = 0;


    virtual void generate_normal_double(const generate_args& args) noexcept = 0;
    virtual void generate_uniform_double(const generate_args& args) noexcept = 0;
    virtual void generate_log_normal_double(const generate_args& args, double mean, double stddev) noexcept = 0;


    virtual void anchor() noexcept;


    static std::unique_ptr<generator> create(unsigned type) noexcept;

  };

  namespace generators {

    struct xorwow_

  }

  namespace kernels {
    template <typename State>
    __global__ void generate(State* globalState, size_t n, size_t sharedMemorySize, CUdeviceptr out) noexcept;

    __global__ void generate4(curandStatePhilox4_32_10_t* globalState, size_t n, size_t sharedMemorySize, CUdeviceptr out) noexcept;

    template <typename State>
    __global__ void generate(State* globalState, size_t n, size_t sharedMemorySize, CUdeviceptr out) noexcept;
  }
}

/*class rng {



  agt_agent_t server;


public:

};*/



class rng_server {

  agt_mailbox_t self;
  agt_mailbox_t inbox;

public:

  void generate(agt_mailbox_t outbox, curandGenerator_t generator, cudaStream_t stream) {
    curandGenerate(generator);


    agt_close_mailbox(outbox);
  }
};

class random {
  agt_agent_t       server;
  agt_mailbox_t     inbox;
  curandGenerator_t generator;
  cudaStream_t      stream;
public:
  random(agt_agent_t server)
      : server(server),
        inbox(nullptr),
        generator(nullptr),
        stream(nullptr)
  {
    agt_create_mailbox_params_t mailboxParams{
      .min_slot_count = 8,
      .min_slot_size  = JEM_CACHE_LINE,
      .max_senders    = 1,
      .max_receivers  = 1,
      .scope          = AGT_MAILBOX_SCOPE_LOCAL,
      .async_handle_address = nullptr,
      .name           = {}
    };
    agt_status_t status = agt_create_mailbox(&inbox, &mailboxParams);
    JEM_assert( status == AGT_SUCCESS );
    curandStatus_t curandStatus = curandCreateGenerator(&generator, CURAND_RNG_PSEUDO_DEFAULT);
    JEM_assert( curandStatus == CURAND_STATUS_SUCCESS );
    cudaError_t error = cudaStreamCreate(&stream);
    JEM_assert( error == cudaSuccess);
    curandStatus = curandSetStream(generator, stream);
    JEM_assert( curandStatus == CURAND_STATUS_SUCCESS );
  }

  ~random() {
    curandDestroyGenerator(generator);
    cudaStreamDestroy(stream);
    agt_close_mailbox(inbox);
  }
};


#pragma region Extern template declarations


extern template __global__ void rng::kernels::generate(curandStateXORWOW_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateSobol32_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateSobol64_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateScrambledSobol32_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateScrambledSobol64_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateMtgp32_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStateMRG32k3a_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;
extern template __global__ void rng::kernels::generate(curandStatePhilox4_32_10_t* globalState,
                                                       size_t n,
                                                       size_t sharedMemorySize,
                                                       CUdeviceptr out) noexcept;


#pragma endregion





#endif//JEMSYS_SAMPLE_RNG_HPP
