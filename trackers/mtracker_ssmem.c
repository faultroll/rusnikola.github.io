
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdlib.h>
#include "trackers/ssmem/include/ssmem.h"
// #include "thread_c.h"

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
    ssmem_allocator_t *allocator; // TODO(lgY): each thread use thread_local allocator
    bool *flag_inited; // once_flag
};

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;
    // How to archive this?
    // config.alloc_func = ssmem_alloc;
    // config.free_func = ssmem_free;

    int task_num = core->config.task_num;
    core->allocator = malloc(sizeof(ssmem_allocator_t) * task_num);
    core->flag_inited = malloc(sizeof(bool) * task_num);
    for (int i = 0; i < task_num; i++) {
        // cannot init here, each thread init once
        // ssmem_alloc_init(&core->allocator[i], SSMEM_DEFAULT_MEM_SIZE, i);
        core->flag_inited[i] = false; // ONCE_FLAG_INIT
    }

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    int task_num = core->config.task_num;
    for (int i = 0; i < task_num; i++) {
        if (core->flag_inited[i])
            // ssmem_alloc_term(&core->allocator[i]); // TODO(lgY): may this working
    }
    // ssmem_term();
    free(core->flag_inited);
    free(core->allocator);
    free(core);
}
// not found a way to xfer param, so bool flag instead
/* static void mt_OnceInitAllocator(void)
{
    ssmem_alloc_init(&core->allocator[tid], SSMEM_DEFAULT_MEM_SIZE, tid);
} */
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    // call_once(&core->flag_inited[tid], mt_OnceInitAllocator);
    if (!core->flag_inited[tid]) {
        ssmem_alloc_init(&core->allocator[tid], SSMEM_DEFAULT_MEM_SIZE, tid);
        core->flag_inited[tid] = true;
    }
    return ssmem_alloc(&core->allocator[tid], core->config.mem_size);
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    ssmem_free(&core->allocator[tid], mem);
}
static void *mt_CoreRead(mt_Core *core, int tid, int sid, void *mem)
{
    return mem; // ATOMIC_VAR_LOAD
}
static void mt_CoreRetire(mt_Core *core, int tid, void *mem)
{

}
static void mt_CoreStartOp(mt_Core *core, int tid)
{

}
static void mt_CoreEndOp(mt_Core *core, int tid)
{

}
static void mt_CoreClearAll(mt_Core *core)
{

}

void mt_InitFuncSSMem(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->read_func       = mt_CoreRead;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreStartOp;
    handle->end_op_func     = mt_CoreEndOp;
    handle->clear_all_func  = mt_CoreClearAll;
}
