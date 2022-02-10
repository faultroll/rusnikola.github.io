
#ifndef _MTRACKER_IMPL_H
#define _MTRACKER_IMPL_H

#include "mtracker.h" // mt_Config, mt_Inst

#if defined(__cplusplus)
extern "C" {
#endif

// Two naming methods
// 1. Namespace is rtc
/* typedef struct rtc_WavWriter rtc_WavWriter;
rtc_WavWriter* rtc_WavOpen(const char* filename, int sample_rate, size_t num_channels);
void rtc_WavClose(rtc_WavWriter* wf); */
// 2. Namespace is webrtc and vad
/* typedef struct WebRtcVadInst VadInst;
VadInst* WebRtcVad_Create(void);
void WebRtcVad_Free(VadInst* handle); */

// Tracker functions, for override and inherit
typedef struct mt_Core mt_Core; // tracker core
typedef mt_Core *(*mt_CreateFunc)(mt_Config config);
typedef void (*mt_DestroyFunc)(mt_Core *handle);
typedef void *(*mt_AllocFunc)(mt_Core *handle, int tid);
// NOTE: reclaim (obj, tid) should be used on all retired objects.
// NOTE: reclaim (obj) shall be only used to thread-local objects.
typedef void (*mt_ReclaimFunc)(mt_Core *handle, int tid, void *mem);
typedef void *(*mt_ReadFunc)(mt_Core *handle, int tid, int sid, void *mem);
typedef void (*mt_RetireFunc)(mt_Core *handle, int tid, void *mem);
typedef void (*mt_StartOpFunc)(mt_Core *handle, int tid);
typedef void (*mt_EndOpFunc)(mt_Core *handle, int tid);
typedef void (*mt_ClearAllFunc)(mt_Core *handle);
struct mt_Inst {
    mt_Type type;
    struct {
        mt_CreateFunc   create_func;
        mt_DestroyFunc  destroy_func;
        mt_AllocFunc    alloc_func;
        mt_ReclaimFunc  reclaim_func;
        mt_ReadFunc     read_func;
        mt_RetireFunc   retire_func;
        mt_StartOpFunc  start_op_func;
        mt_EndOpFunc    end_op_func;
        mt_ClearAllFunc clear_all_func;
    }; // anonymous (mt_Func)
    // handle for tracker impl
    mt_Core *core;
    // Used for transfer
    int task_num;
    int slot_num;
    int **slot_renamers; // padded
};
// Each tracker has its init functions
// extern void mt_InitFuncXXX(mt_Inst *handle);

#if defined(__cplusplus)
}
#endif

#endif /* _MTRACKER_IMPL_H */
