
#ifndef _MTRACKER_H
#define _MTRACKER_H

// #include <stddef.h>

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
typedef struct mt_Inst mt_Inst; // memorytracker Instance

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
void    *mt_Read(mt_Inst *handle, int tid, int idx); // acquire
void    mt_Retire(mt_Inst *handle, int tid, void *mem); // release
void    mt_StartOp(mt_Inst *handle, int tid); // enter
void    mt_EndOp(mt_Inst *handle, int tid); // leave
// void    mt_LastEndOp(mt_Inst *handle, int tid); // last leave
void    mt_ClearAll(mt_Inst *handle); // leave all (EndOp all tid)
// void    mt_Transfer(mt_Inst *handle, int src_idx, int dst_idx, int tid);
// void    mt_Release(mt_Inst *handle, int idx, int tid);

#if defined(__cplusplus)
}
#endif

#endif /* _MTRACKER_H */
