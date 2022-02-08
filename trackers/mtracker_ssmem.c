
#include "trackers/mtracker_impl.h"
#include <stdlib.h>
#include "trackers/ssmem/include/ssmem.h"

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
    ssmem_allocator_t **allocator;
};

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;
    // How to archive this?
    // config.alloc_func = ssmem_alloc;
    // config.free_func = ssmem_free;

    int task_num = core->config.task_num;
    core->allocator = malloc(sizeof(ssmem_allocator_t *) * task_num);
    for (int i = 0; i < task_num; i++) {
        ssmem_alloc_init(core->allocator[i], SSMEM_DEFAULT_MEM_SIZE, i);
    }

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    free(core->allocator);
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    return ssmem_alloc(core->allocator[tid], core->config.mem_size);
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    ssmem_free(core->allocator[tid], mem);
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
