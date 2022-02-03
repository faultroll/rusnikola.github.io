
#include "mtracker.h"
#include <stdbool.h>

static const bool collect = FALSE;

mt_Inst *mt_Create(mt_Type type, int task_num, int epoch_freq, int empty_freq, int slot_num)
{
    mt_Inst *inst;

    switch (type) {
        case NIL:
            inst = mt_CreateBase(task_num);
            break;
        /* case RCU:
            inst = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_RCU, collect);
            break;
        case RangeNew:
            inst = mt_CreateRangeNew(task_num, epoch_freq, empty_freq, collect);
            break;
        case Hazard:
            inst = mt_CreateHazard(task_num, slot_num, empty_freq, collect);
            break;
        case WFE:
            inst = mt_CreateWFE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case HE:
            inst = mt_CreateHE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case QSBR:
            inst = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_QSBR, collect);
            break;
        case Interval:
            inst = mt_CreateInterval(task_num, epoch_freq, empty_freq, collect);
            break; */
#if 0 // !(__x86_64__ || __ppc64__) // only compile in 32 bit mode
        case RangeTP:
            inst = mt_CreateRangeTP(task_num, epoch_freq, empty_freq, collect);
            break;
#endif
        default:
            fprintf(stderr, "constructor - tracker type %d error.", type);
            inst = NULL;
            break;
    }

    return inst;
}
void mt_Destroy(mt_Inst *handle)
{

}
void *mt_Alloc(mt_Inst *handle, int tid)
{

}
void mt_Reclaim(mt_Inst *handle, int tid, void *mem)
{

}
void *mt_Read(mt_Inst *handle, int tid, int idx)
{

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

