
#include "mtracker.h"
#include <stdbool.h>

static const bool collect = false;

mt_Inst *mt_Create(mt_Type type, int task_num, int epoch_freq, int empty_freq, int slot_num)
{
    mt_Inst *handle;

    switch (type) {
        case NIL:
            handle = mt_CreateBase(task_num);
            break;
#if 0
        case RCU:
            handle = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_RCU, collect);
            break;
        case RangeNew:
            handle = mt_CreateRangeNew(task_num, epoch_freq, empty_freq, collect);
            break;
        case Hazard:
            handle = mt_CreateHazard(task_num, slot_num, empty_freq, collect);
            break;
        case WFE:
            handle = mt_CreateWFE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case HE:
            handle = mt_CreateHE(task_num, slot_num, epoch_freq, empty_freq, collect);
            break;
        case QSBR:
            handle = mt_CreateRCU(task_num, epoch_freq, empty_freq, type_QSBR, collect);
            break;
        case Interval:
            handle = mt_CreateInterval(task_num, epoch_freq, empty_freq, collect);
            break;
#if !(__x86_64__ || __ppc64__) // only compile in 32 bit mode
        case RangeTP:
            handle = mt_CreateRangeTP(task_num, epoch_freq, empty_freq, collect);
            break;
#endif
#endif // 0
        default:
            fprintf(stderr, "constructor - tracker type %d error.", type);
            handle = NULL;
            break;
    }

    return handle;
}
