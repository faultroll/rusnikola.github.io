
#include "mtracker.h"
#include "trackers/mtracker_impl.h"
#include <stddef.h> // NULL
// #include <stdbool.h> // true/false
#include <stdlib.h> // malloc/free
#include <stdio.h> // fprintf

mt_Inst *mt_Create(mt_Type type, mt_Config config)
{
    mt_Inst *handle = malloc(sizeof(mt_Inst));
    if (NULL == handle)
        return NULL;

    handle->type = type;
    // Default
    extern void mt_InitFuncBase(mt_Inst * handle);
    mt_InitFuncBase(handle);
    // Override
    switch (type) {
        case RCU: {
            extern void mt_InitFuncRCU(mt_Inst * handle);
            mt_InitFuncRCU(handle);
            break;
        }
        case QSBR: {
            extern void mt_InitFuncQSBR(mt_Inst * handle);
            mt_InitFuncQSBR(handle);
            break;
        }
        case SSMEM: {
            extern void mt_InitFuncSSMem(mt_Inst * handle);
            mt_InitFuncSSMem(handle);
            break;
        }
#if 0
        case RangeNew: {
            extern void mt_InitFuncRangeNew(mt_Inst * handle);
            mt_InitFuncRangeNew(handle);
            break;
        }
        case Hazard: {
            extern void mt_InitFuncHazard(mt_Inst * handle);
            mt_InitFuncHazard(handle);
            break;
        }
        case WFE: {
            extern void mt_InitFuncWFE(mt_Inst * handle);
            mt_InitFuncWFE(handle);
            break;
        }
        case HE: {
            extern void mt_InitFuncHE(mt_Inst * handle);
            mt_InitFuncHE(handle);
            break;
        }
        case Interval: {
            extern void mt_InitFuncInterval(mt_Inst * handle);
            mt_InitFuncInterval(handle);
            break;
        }
#if !(__x86_64__ || __ppc64__) // only compile in 32 bit mode
        case RangeTP: {
            extern void mt_InitFuncRangeTP(mt_Inst * handle);
            mt_InitFuncRangeTP(handle);
            break;
        }
#endif
#endif // 0
        case NIL:
        default: {
            // No override
            fprintf(stderr, "constructor - tracker type %d error, use Base.\n", type);
            break;
        }
    }

    if (NULL == config.alloc_func)
        config.alloc_func = malloc;
    if (NULL == config.free_func)
        config.free_func = free;

    if (handle->create_func != NULL)
        handle->core = handle->create_func(config);

    handle->task_num = config.task_num;
    handle->slot_num = config.slot_num;
    handle->slot_renamers = malloc(sizeof(int *) * handle->task_num);
    for (int i = 0; i < handle->task_num; i++) {
        handle->slot_renamers[i] = malloc(sizeof(int) * handle->slot_num);
        for (int j = 0; j < handle->slot_num; j++) {
            handle->slot_renamers[i][j] = j;
        }
    }

    return handle;
}
void mt_Destroy(mt_Inst *handle)
{
    if (NULL == handle)
        return;

    for (int i = 0; i < handle->task_num; i++) {
        free(handle->slot_renamers[i]);
    }
    free(handle->slot_renamers);

    if (handle->destroy_func != NULL)
        handle->destroy_func(handle->core);

    free(handle);
}

void *mt_Alloc(mt_Inst *handle, int tid)
{
    if (NULL == handle || NULL == handle->alloc_func)
        return NULL;

    return handle->alloc_func(handle->core, tid);
}
void mt_Reclaim(mt_Inst *handle, int tid, void *mem)
{
    if (NULL == handle || NULL == handle->reclaim_func)
        return;

    handle->reclaim_func(handle->core, tid, mem);
}
void *mt_Read(mt_Inst *handle, int tid, int sid, void *mem)
{
    if (NULL == handle || NULL == handle->read_func)
        return NULL;

    return handle->read_func(handle->core, tid, handle->slot_renamers[tid][sid], mem);
}
void mt_Retire(mt_Inst *handle, int tid, void *mem)
{
    if (NULL == handle || NULL == handle->retire_func)
        return;

    handle->retire_func(handle->core, tid, mem);
}
void mt_StartOp(mt_Inst *handle, int tid)
{
    if (NULL == handle || NULL == handle->start_op_func)
        return;

    handle->start_op_func(handle->core, tid);
}
void mt_EndOp(mt_Inst *handle, int tid)
{
    if (NULL == handle || NULL == handle->end_op_func)
        return;

    handle->end_op_func(handle->core, tid);
}
/* void mt_LastEndOp(mt_Inst *handle, int tid)
{
    if (NULL == handle || NULL == handle->last_end_op_func)
        return;

    handle->last_end_op_func(handle->core, tid);
} */
void mt_ClearAll(mt_Inst *handle)
{
    if (NULL == handle || NULL == handle->clear_all_func)
        return;

    handle->clear_all_func(handle->core);
}
/* void mt_Transfer(mt_Inst *handle, int src_sid, int dst_sid, int tid)
{
    if (NULL == handle)
        return;

    int tmp = handle->slot_renamers[tid][src_sid];
    handle->slot_renamers[tid][src_sid] = handle->slot_renamers[tid][dst_sid];
    handle->slot_renamers[tid][dst_sid] = tmp;
}
void mt_Release(mt_Inst *handle, int tid, int sid)
{
    if (NULL == handle || NULL == handle->release_func)
        return;

    handle->release_func(handle->core, tid, handle->slot_renamers[tid][sid]);
} */

