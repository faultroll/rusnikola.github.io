
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdlib.h>
#include "trackers/ssmem/include/ssmem.h"
#include "thread_c.h"
// #include "atomic_c.h"

// DONE(lgY): ssmem fixes
// 1. each thread use thread_local allocator, reduce mem use
// 2. it seems that ssmem only works proper(init/term/alloc/free)
//    if each thread have only 1 allocator?(ssmem use one list 
//    to reclaim, but each allocator have its own mempool)

#define MT_SSMEM_USE_LOCAL_ALLOCATOR 1

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
#if !MT_SSMEM_USE_LOCAL_ALLOCATOR
    ssmem_allocator_t *allocator;
    bool *flag_init; // once_flag
#endif // !MT_SSMEM_USE_LOCAL_ALLOCATOR
};

#if MT_SSMEM_USE_LOCAL_ALLOCATOR
static thread_local ssmem_allocator_t allocator_; // never destroy
static thread_local once_flag flag_init_ = ONCE_FLAG_INIT;
static void mt_OnceInitAllocator(void)
{
    ssmem_alloc_init(&allocator_, SSMEM_DEFAULT_MEM_SIZE, MT_DEFAULT_TID);
}
#endif // MT_SSMEM_USE_LOCAL_ALLOCATOR

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;
    // How to archive this?
    // config.alloc_func = ssmem_alloc;
    // config.free_func = ssmem_free;

#if !MT_SSMEM_USE_LOCAL_ALLOCATOR
    int task_num = core->config.task_num;
    core->allocator = malloc(sizeof(ssmem_allocator_t) * task_num);
    core->flag_init = malloc(sizeof(bool) * task_num); // sizeof(once_flag)
    for (int i = 0; i < task_num; i++) {
        // cannot init here, each thread init once
        // ssmem_alloc_init(&core->allocator[i], SSMEM_DEFAULT_MEM_SIZE, i);
        core->flag_init[i] = false; // ONCE_FLAG_INIT
    }
#endif // !MT_SSMEM_USE_LOCAL_ALLOCATOR

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
#if !MT_SSMEM_USE_LOCAL_ALLOCATOR
    int task_num = core->config.task_num;
    for (int i = 0; i < task_num; i++) {
        if (core->flag_init[i]) {
            // should term in each tid
            ssmem_alloc_term(&core->allocator[i]);
        }
    }
    free(core->flag_init);
    free(core->allocator);
#endif // !MT_SSMEM_USE_LOCAL_ALLOCATOR
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
#if MT_SSMEM_USE_LOCAL_ALLOCATOR
    call_once(&flag_init_, mt_OnceInitAllocator);
    return ssmem_alloc(&allocator_, core->config.mem_size);
#else // MT_SSMEM_USE_LOCAL_ALLOCATOR
    if (!core->flag_init[tid]) {
        ssmem_alloc_init(&core->allocator[tid], SSMEM_DEFAULT_MEM_SIZE, tid);
        core->flag_init[tid] = true;
    }
    return ssmem_alloc(&core->allocator[tid], core->config.mem_size);
#endif // MT_SSMEM_USE_LOCAL_ALLOCATOR
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
#if MT_SSMEM_USE_LOCAL_ALLOCATOR
    ssmem_free(&allocator_, mem);
#else // MT_SSMEM_USE_LOCAL_ALLOCATOR
    ssmem_free(&core->allocator[tid], mem);
#endif // MT_SSMEM_USE_LOCAL_ALLOCATOR
}
static void *mt_CoreAcquire(mt_Core *core, int tid, int sid, void *volatile mem)
{
    return (void *)mem;
}
static void mt_CoreRetire(mt_Core *core, int tid, void *mem)
{
    mt_CoreReclaim(core, tid, mem);
}
static void mt_CoreStartOp(mt_Core *core, int tid)
{

}
static void mt_CoreEndOp(mt_Core *core, int tid)
{

}

void mt_InitFuncSSMem(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->acquire_func    = mt_CoreAcquire;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreStartOp;
    handle->end_op_func     = mt_CoreEndOp;
}
