
#ifndef _MTRACKER_H
#define _MTRACKER_H

// #include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct mt_Inst mt_Inst; // memory tracker instance

typedef enum {
    NIL = 0,
    //for epoch-based trackers.
    RCU = 2,
    Interval = 4,
    // Range = 6,
    RangeNew = 8,
    QSBR = 10,
    // RangeTP = 12,
    //for HP-like trackers.
    Hazard = 1,
    // HazardDynamic = 3,
    HE = 5,
    // WFE = 7,
    // FORK = 13,
} mt_Type;

mt_Inst *mt_Create(mt_Type type, int task_num, int epoch_freq, int empty_freq, int slot_num);
void    mt_Destroy(mt_Inst *handle);
void    *mt_Alloc(mt_Inst *handle, int tid);
void    mt_Reclaim(mt_Inst *handle, int tid, void *mem); // free
void    *mt_Read(mt_Inst *handle, int tid, int slot_idx); // acquire
void    mt_Retire(mt_Inst *handle, int tid, void *mem); // release
void    mt_StartOp(mt_Inst *handle, int tid); // enter
void    mt_EndOp(mt_Inst *handle, int tid); // leave
// void    mt_LastEndOp(mt_Inst *handle, int tid); // last leave
void    mt_ClearAll(mt_Inst *handle); // leave all (EndOp all tid)
// void    mt_Transfer(mt_Inst *handle, int src_slot_idx, int dst_slot_idx, int tid);
// void    mt_Release(mt_Inst *handle, int tid, int slot_idx);

#if defined(__cplusplus)
}
#endif

#endif /* _MTRACKER_H */
