
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include "atomic_c.h"

typedef struct RCUInfo RCUInfo;
struct RCUInfo {
    RCUInfo *next;
    uint64_t epoch;
};

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
    RCUInfo **retired; // padded
    ATOMIC_VAR(uint64_t) *reservations; // padded
    uint64_t *retire_counters; // padded
    uint64_t *alloc_counters; // padded

    ATOMIC_VAR(uint64_t) epoch; // padded
};

/* static inline
uint64_t mt_GetEpoch(mt_Core *core) {
    return ATOMIC_VAR_LOAD(&core->epoch);
}
static inline
void mt_IncrementEpoch(mt_Core *core) {
    ATOMIC_VAR_FAA(&core->epoch, 1);
} */

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;

    int task_num = core->config.task_num;
    core->retired = malloc(sizeof(RCUInfo *) * task_num);
    core->reservations = malloc(sizeof(ATOMIC_VAR(uint64_t)) * task_num);
    core->retire_counters = malloc(sizeof(uint64_t) * task_num);
    core->alloc_counters = malloc(sizeof(uint64_t) * task_num);
    for (int i = 0; i < task_num; i++) {
        core->retired[i] = NULL;
        ATOMIC_VAR_STOR(&core->reservations[i], UINT64_MAX);
        core->retire_counters[i] = 0;
        core->alloc_counters[i] = 0;
    }
    ATOMIC_VAR_STOR(&core->epoch, 0);

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    // TODO(lgY): reclaim all retired memory here

    free(core->alloc_counters);
    free(core->retire_counters);
    free(core->reservations);
    free(core->retired);
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    if ((++core->alloc_counters[tid]) %
        (core->config.epoch_freq * core->config.task_num) == 0)
        ATOMIC_VAR_FAA(&core->epoch, 1); // mt_IncrementEpoch(core);

    return core->config.alloc_func(core->config.mem_size + sizeof(RCUInfo));
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    core->config.free_func(mem);
}
static void *mt_CoreRead(mt_Core *core, int tid, int sid, void *mem)
{
    return mem;
}
static void mt_Empty(mt_Core *core, int tid)
{
    uint64_t min_epoch = UINT64_MAX;
    for (int i = 0; i < core->config.task_num; i++) {
        uint64_t res = ATOMIC_VAR_LOAD(&core->reservations[i]);
        if (res < min_epoch) {
            min_epoch = res;
        }
    }

    // erase safe objects
    RCUInfo **field = &(core->retired[tid]);
    RCUInfo *info = *field;
    while (info != NULL) {
        RCUInfo *curr = info;
        info = curr->next;
        if (curr->epoch < min_epoch) {
            *field = info;
            mt_CoreReclaim(core, tid, (void *)((uintptr_t)curr - core->config.mem_size));
            // dec_retired(tid);
            continue;
        }
        field = &curr->next;
    }
}
static void mt_CoreRetire(mt_Core *core, int tid, void *mem)
{
    if (NULL == mem) {
        return;
    }
    RCUInfo **field = &(core->retired[tid]);
    RCUInfo *info = (RCUInfo *)((uintptr_t)mem + core->config.mem_size); // find info
    info->epoch = ATOMIC_VAR_LOAD(&core->epoch); // mt_GetEpoch(core);
    info->next = *field;
    *field = info;
    if (core->config.collect && core->retire_counters[tid] % core->config.empty_freq == 0) {
        mt_Empty(core, tid);
    }
    ++core->retire_counters[tid];
}
static void mt_CoreRCUStartOp(mt_Core *core, int tid)
{
    // if (type == RCU) {
    uint64_t e = ATOMIC_VAR_LOAD(&core->epoch);
    ATOMIC_VAR_STOR(&core->reservations[tid], e);
    // }
}
static void mt_CoreRCUEndOp(mt_Core *core, int tid)
{
    // if (type == RCU) {
    ATOMIC_VAR_STOR(&core->reservations[tid], UINT64_MAX);
    // } else { //if type == QSBR
    //     uint64_t e = ATOMIC_VAR_LOAD(&core->epoch);
    //     ATOMIC_VAR_STOR(&core->reservations[tid], e);
    // }
}
static void mt_CoreQSBRStartOp(mt_Core *core, int tid)
{
}
static void mt_CoreQSBREndOp(mt_Core *core, int tid)
{
    uint64_t e = ATOMIC_VAR_LOAD(&core->epoch);
    ATOMIC_VAR_STOR(&core->reservations[tid], e);
}

void mt_InitFuncRCU(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->read_func       = mt_CoreRead;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreRCUStartOp;
    handle->end_op_func     = mt_CoreRCUEndOp;
}

void mt_InitFuncQSBR(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->read_func       = mt_CoreRead;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreQSBRStartOp;
    handle->end_op_func     = mt_CoreQSBREndOp;
}
