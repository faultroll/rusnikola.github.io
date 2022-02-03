
#ifndef _MTRACKER_IMPL_H
#define _MTRACKER_IMPL_H

#if defined(__cplusplus)
extern "C" {
#endif

// Tracker core
typedef struct mt_Tracker mt_Tracker;

// Tracker functions, for override and inherit
// Create function is not same (eg. rcu/qsbr has its own parameter)
// typedef mt_Tracker *(*mt_Create)(int task_num, int epoch_freq, int empty_freq, int slot_num);
typedef void (*mt_Destroy)(mt_Tracker *handle);
typedef void *(*mt_Alloc)(mt_Tracker *handle, int tid);
typedef void (*mt_Reclaim)(mt_Tracker *handle, int tid, void *mem);
typedef void *(*mt_Read)(mt_Tracker *handle, int tid, int slot_idx);
typedef void (*mt_Retire)(mt_Tracker *handle, int tid, void *mem);
typedef void (*mt_StartOp)(mt_Tracker *handle, int tid);
typedef void (*mt_EndOp)(mt_Tracker *handle, int tid);
typedef void (*mt_ClearAll)(mt_Tracker *handle);
// functions
typedef struct mt_Function {
    // mt_Create create_func;
    mt_Destroy destroy_func;
    mt_Alloc alloc_func;
    mt_Reclaim reclaim_func;
    mt_Read read_func;
    mt_Retire retire_func;
    mt_StartOp start_op_func;
    mt_EndOp end_op_func;
    mt_StartOp start_op_func;
    mt_ClearAll clear_all_func;
    // ...
} mt_Function;
// Each tracker has its init functions
// extern void mt_InitFuncXXX(mt_Function *func);
// Each tracker has its create functions
// extern mt_Tracker *mt_CreateXXX(...);

#if defined(__cplusplus)
}
#endif

#endif /* _MTRACKER_IMPL_H */
