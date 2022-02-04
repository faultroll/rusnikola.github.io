
#include "trackers/mtracker_impl.h"
#include <stdlib.h>
// #include "atmoic_c.h"

struct mt_Core {
    // private:
    int task_num;
    // public:
    // ATOMIC_VAR(int64_t) retired; // padded
};
/* // TODO(lgY): retire monitor
static const bool count_retired_ = false;
ATOMIC_VAR_STOR(&core->retired, 0);
int64_t get_retired_cnt(int tid)
{
    // An average per-task
    return count_retired_ ? ATOMIC_VAR_LOAD(&retired) / task_num) : 0;
}
static void inc_retired(int tid)
{
    if (count_retired_)
        ATOMIC_VAR_FAA(&retired, 1);
}
static void dec_retired(int tid)
{
    if (count_retired_)
        ATOMIC_VAR_FAA(&retired, -1);
} */

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->task_num = config.task_num;

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    // return malloc(sizeof(T));
    return NULL;
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    /* assert(mem != NULL);
    mem->~T();
    free(mem); */
}
static void *mt_CoreRead(mt_Core *core, int tid, int sid, void *mem)
{
    // return mem.load(std::memory_order_acquire);
    return mem;
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

void mt_InitFuncBase(mt_Inst *handle)
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
