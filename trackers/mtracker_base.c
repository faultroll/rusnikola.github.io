
#include "mtracker.h"
#include <stdlib.h>
// #include "atmoic_c.h"

struct mt_Inst {
    // private:
    int task_num;
    // public:
    // ATOMIC_VAR(int64_t) retired; // padded
};
/* // TODO(lgY): retire monitor
static const bool count_retired_ = false;
ATOMIC_VAR_STOR(&handle->retired, 0);
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

mt_Inst *mt_CreateBase(int task_num)
{
    mt_Inst *handle = malloc(sizeof(mt_Inst));
    handle->task_num = task_num;

    return handle;
}
void mt_Destroy(mt_Inst *handle)
{
    free(handle);
}
void *mt_Alloc(mt_Inst *handle, int tid)
{
    return malloc(sizeof(T));
}
//NOTE: reclaim (obj, tid) should be used on all retired objects.
//NOTE: reclaim (obj) shall be only used to thread-local objects.
void mt_Reclaim(mt_Inst *handle, int tid, void *mem)
{
    /* assert(mem != NULL);
    mem->~T(); */
    free(mem);
}
void *mt_Read(mt_Inst *handle, int tid, void *mem, int idx)
{
    // return mem.load(std::memory_order_acquire);
    return mem;
}
void mt_Retire(mt_Inst *handle, int tid, void *mem)
{

}
void mt_StartOp(mt_Inst *handle, int tid)
{

}
void mt_EndOp(mt_Inst *handle, int tid)
{

}
void mt_LastEndOp(mt_Inst *handle, int tid)
{

}
void mt_ClearAll(mt_Inst *handle)
{

}
void mt_Transfer(mt_Inst *handle, int src_idx, int dst_idx, int tid)
{

}
void mt_Release(mt_Inst *handle, int idx, int tid)
{

}
