
#include "mtracker.h"
#include <stdbool.h>

// Two naming methods
// 1. Namespace is rtc
/* typedef struct rtc_WavWriter rtc_WavWriter;
rtc_WavWriter* rtc_WavOpen(const char* filename, int sample_rate, size_t num_channels);
void rtc_WavClose(rtc_WavWriter* wf); */
// 2. Namespace is webrtc and vad
/* typedef struct WebRtcVadInst VadInst;
VadInst* WebRtcVad_Create(void);
void WebRtcVad_Free(VadInst* handle); */

struct mt_Inst {
    mt_Type type;
    mt_Function func;
    mt_Tracker *tracker;

    int **slot_renamers; // padded
};

static const bool collect = true;

mt_Inst *mt_Create(mt_Type type, int task_num, int epoch_freq, int empty_freq, int slot_num)
{
    mt_Inst *handle = malloc(sizeof(mt_Inst));
    // extern void mt_InitFuncBase(mt_Function *func); // Shoule include header file
    mt_InitFuncBase(&handle->func);

    switch (type) {
        case NIL:
            // extern mt_Tracker *mt_CreateBase(int task_num);
            handle->tracker = mt_CreateBase(task_num);
            break;
#if 0
        case RCU:
            // extern void mt_InitFuncRCU(mt_Function *func);
            mt_InitFuncRCU(&handle->func); // Override some of base funcs
            handle->tracker = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_RCU, collect);
            break;
        case RangeNew:
            handle->tracker = mt_CreateRangeNew(task_num, epoch_freq, empty_freq, collect);
            break;
        case Hazard:
            handle->tracker = mt_CreateHazard(task_num, slot_num, empty_freq, collect);
            break;
        case WFE:
            handle->tracker = mt_CreateWFE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case HE:
            handle->tracker = mt_CreateHE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case QSBR:
            handle->tracker = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_QSBR, collect);
            break;
        case Interval:
            handle->tracker = mt_CreateInterval(task_num, epoch_freq, empty_freq, collect);
            break;
#if !(__x86_64__ || __ppc64__) // only compile in 32 bit mode
        case RangeTP:
            handle->tracker = mt_CreateRangeTP(task_num, epoch_freq, empty_freq, collect);
            break;
#endif
#endif // 0
        default:
            fprintf(stderr, "constructor - tracker type %d error.", type);
            free(handle);
            handle = NULL;
            break;
    }

    if (handle != NULL) {
        handle->slot_renamers = malloc(task_num);
        for (int i = 0; i < task_num; i++) {
            handle->slot_renamers[i] = malloc(sizeof(int) * slot_num);
            for (int j = 0; j < slot_num; j++) {
                handle->slot_renamers[i][j] = j;
            }
        }
    }

    return handle;
}
void mt_Destroy(mt_Inst *handle)
{
    if (NULL == handle)
        return;

    handle->func.destroy_func(handle->tracker);
    free(handle);
}

void *mt_Alloc(mt_Inst *handle, int tid)
{
    if (NULL == handle)
        return NULL;

    return handle->func.alloc_func(handle->tracker, tid);
}
//NOTE: reclaim (obj, tid) should be used on all retired objects.
//NOTE: reclaim (obj) shall be only used to thread-local objects.
void mt_Reclaim(mt_Inst *handle, int tid, void *mem)
{
    if (NULL == handle)
        return;

    handle->func.reclaim_func(handle->tracker, tid, mem);
}
void *mt_Read(mt_Inst *handle, int tid, int slot_idx)
{
    if (NULL == handle)
        return NULL;

    return handle->func.read_func(handle->tracker, tid, handle->slot_renamers[tid][slot_idx]);
}
void mt_Retire(mt_Inst *handle, int tid, void *mem)
{
    if (NULL == handle)
        return;

    handle->func.retire_func(handle->tracker, tid, mem);
}
void mt_StartOp(mt_Inst *handle, int tid)
{
    if (NULL == handle)
        return;

    handle->func.start_op_func(handle->tracker, tid);
}
void mt_EndOp(mt_Inst *handle, int tid)
{
    if (NULL == handle)
        return;

    handle->func.end_op_func(handle->tracker, tid);
}
void mt_LastEndOp(mt_Inst *handle, int tid)
{
    if (NULL == handle)
        return;

    handle->func.last_end_op_func(handle->tracker, tid);
}
void mt_ClearAll(mt_Inst *handle)
{
    if (NULL == handle)
        return;

    handle->func.clear_all_func(handle->tracker);
}
void mt_Transfer(mt_Inst *handle, int src_idx, int dst_idx, int tid)
{
    if (NULL == handle)
        return;

    int tmp = handle->slot_renamers[tid][src_idx];
    handle->slot_renamers[tid][src_idx] = handle->slot_renamers[tid][dst_idx];
    handle->slot_renamers[tid][dst_idx] = tmp;
}
void mt_Release(mt_Inst *handle, int tid, int slot_idx)
{
    if (NULL == handle)
        return;

    handle->func->release_func(handle->slot_renamers[tid][slot_idx], tid);
}

