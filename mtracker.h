
#ifndef _MTRACKER_H
#define _MTRACKER_H

#include <stddef.h> // size_t
#include <stdbool.h> // bool
#include <stdlib.h> // malloc/free

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct mt_Inst mt_Inst; // memory tracker instance

typedef enum {
    // kTypeNil/kTrackerNil or kMTNil is wield
    MT_NIL = 0,

    //for epoch-based trackers.
    MT_RCU = 2,
    MT_Interval = 4,
    // MT_Range = 6,
    MT_RangeNew = 8,
    MT_QSBR = 10,
    // MT_RangeTP = 12,
    MT_SSMEM = 14,

    //for HP-like trackers.
    MT_Hazard = 1,
    // MT_HazardDynamic = 3,
    MT_HE = 5,
    // MT_WFE = 7,
    // MT_FORK = 13,

} mt_Type;

typedef struct {
    int task_num;
    int slot_num;
    int epoch_freq;
    int empty_freq;
    bool collect; // tracker really works? or just dummy.
    struct {
        // we need and use this as max size, for some trackers may 
        // alloc tracker info after memory (same address, with offset).
        size_t mem_size;
        // maybe you want to use a mempool?
        void *(*alloc_func)(size_t);
        // you can also wrap dtor in this.
        void (*free_func)(void *);
    }; // anonymous (mt_MemDes)
} mt_Config;

// tid: task_idx, sid: slot_idx
mt_Inst *mt_Create(mt_Type type, mt_Config config);
void    mt_Destroy(mt_Inst *handle);
void    *mt_Alloc(mt_Inst *handle, int tid); // malloc, size is in config
void    mt_Reclaim(mt_Inst *handle, int tid, void *mem); // free
void    *mt_Read(mt_Inst *handle, int tid, int sid, void *mem); // acquire
void    mt_Retire(mt_Inst *handle, int tid, void *mem); // release
void    mt_StartOp(mt_Inst *handle, int tid); // enter
void    mt_EndOp(mt_Inst *handle, int tid); // leave, clear (all slot)
// void    mt_LastEndOp(mt_Inst *handle, int tid); // last leave (in current tid)
void    mt_Transfer(mt_Inst *handle, int tid, int src_sid, int dst_sid);

// default config
#define MT_DEFAULT_CONF(_size) \
    ((mt_Config){ \
        /* .task_num = */ 32, \
        /* .slot_num = */ 4, \
        /* .epoch_freq = */ 150, \
        /* .empty_freq = */ 30, \
        /* .collect = */ true, \
        { \
            /* .mem_size = */ _size, \
            /* .alloc_func = */ malloc, \
            /* .free_func = */ free, \
        } \
    })
// default task idx
extern int mt_GetTid(void);
#define MT_DEFAULT_TID mt_GetTid()
// default slot idx
#define MT_DEFAULT_SID 0

#if defined(__cplusplus)
}
#endif

#endif /* _MTRACKER_H */
