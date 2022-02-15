
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include "atomic_c.h"

typedef struct IntervalInfo IntervalInfo;
struct IntervalInfo {
    IntervalInfo *next;
    void* obj;
    uint64_t birth_epoch;
    uint64_t retire_epoch;
};

struct mt_Core {
    // private:
    mt_Type type;
    mt_Config config;
    // public:
    // list
    IntervalInfo **retired; // padded
	ATOMIC_VAR(uint64_t) *upper_reservs; // padded
	ATOMIC_VAR(uint64_t) *lower_reservs; // padded
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
}
static inline
uint64_t mt_GetBirth(mt_Core *core, void* mem){
	uint64_t* birth_epoch = (uint64_t*)(((uintptr_t))mem + core->config.mem_size);
	return *birth_epoch;
} */

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;

    int task_num = core->config.task_num;
    core->retired = malloc(sizeof(IntervalInfo *) * task_num);
    core->upper_reservs = malloc(sizeof(ATOMIC_VAR(uint64_t)) * task_num);
    core->lower_reservs = malloc(sizeof(ATOMIC_VAR(uint64_t)) * task_num);
    core->retire_counters = malloc(sizeof(uint64_t) * task_num);
    core->alloc_counters = malloc(sizeof(uint64_t) * task_num);
    for (int i = 0; i < task_num; i++) {
        core->retired[i] = NULL;
        ATOMIC_VAR_STOR(&core->upper_reservs[i], 0); // what should we stor here?
        ATOMIC_VAR_STOR(&core->lower_reservs[i], UINT64_MAX);
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
    free(core->lower_reservs);
    free(core->upper_reservs);
    free(core->retired);
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    if ((++core->alloc_counters[tid]) %
        (core->config.epoch_freq * core->config.task_num) == 0)
        ATOMIC_VAR_FAA(&core->epoch, 1); // mt_IncrementEpoch(core);

    // here we can store IntervalInfo just like other trackers does
	uintptr_t block = (uintptr_t) core->config.alloc_func(core->config.mem_size+sizeof(uint64_t));
	uint64_t* birth_epoch = (uint64_t*) (block + core->config.mem_size);
	*birth_epoch = ATOMIC_VAR_LOAD(&core->epoch);
	return (void*)block;
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    core->config.free_func(mem);
}
static void *mt_CoreAcquire(mt_Core *core, int tid, int sid, void *volatile mem)
{
	uint64_t prev_epoch = ATOMIC_VAR_LOAD(&core->upper_reservs[tid]);
	while(true){
		void* ptr = (void *)mem;
		uint64_t curr_epoch = ATOMIC_VAR_LOAD(&core->epoch);
		if (curr_epoch == prev_epoch){
			return ptr;
		} else {
			ATOMIC_VAR_STOR(&core->upper_reservs[tid], curr_epoch);
			prev_epoch = curr_epoch;
		}
	}
}
static inline
bool mt_Validate(mt_Core *core, int tid){
	return (ATOMIC_VAR_LOAD(&core->lower_reservs[tid]) == ATOMIC_VAR_LOAD(&core->epoch));
}
/* static inline
bool mt_Danger(uint64_t* lower_epochs, uint64_t* upper_epochs, uint64_t birth_epoch, uint64_t retire_epoch){
    for (int i = 0; i < task_num; i++){
		if (upper_epochs[i] >= birth_epoch && lower_epochs[i] <= retire_epoch){
			return true;
		} 
    }
	return false;
} */
static void mt_Empty(mt_Core *core, int tid)
{
	//read all epochs
	//sequence matters.
	/* uint64_t upper_epochs_arr[task_num];
	uint64_t lower_epochs_arr[task_num];
	for (int i = 0; i < task_num; i++){
		lower_epochs_arr[i] = ATOMIC_VAR_LOAD(&core->lower_reservs[i]);
		upper_epochs_arr[i] = ATOMIC_VAR_LOAD(&core->upper_reservs[i]);
	} */
    uint64_t min_epoch = UINT64_MAX, max_epoch = 0;
    for (int i = 0; i < core->config.task_num; i++) {
        uint64_t lower_res = ATOMIC_VAR_LOAD(&core->lower_reservs[i]);
        uint64_t upper_res = ATOMIC_VAR_LOAD(&core->upper_reservs[i]);
        if (lower_res < min_epoch) {
            min_epoch = lower_res;
        }
        if (upper_res > max_epoch) {
            max_epoch = upper_res;
        }
    }

    // erase safe objects
    IntervalInfo **field = &(core->retired[tid]);
    IntervalInfo *info = *field;
    // for (auto iterator = list->begin(), end = list->end(); iterator != end; ) {
    while (info != NULL) {
        IntervalInfo *curr = info;
        info = curr->next;
        // if (!mt_Danger(lower_epochs_arr, upper_epochs_arr, curr->birth_epoch, curr->retire_epoch)) {
        if (!(max_epoch >= curr->birth_epoch && min_epoch <= curr->retire_epoch)) {
            // list->erase(curr);
            *field = info;
            mt_CoreReclaim(core, tid, curr->obj);
            // dec_retired(tid);
            free(curr); // freed here is ok, for it's erased from list before it reclaimed
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
    IntervalInfo **field = &(core->retired[tid]);
	// for(auto it = field->begin(); it!=field->end(); it++){
	// 	assert(it->obj!=obj && "double retire error");
	// }
    // list->push_back(IntervalInfo(obj, birth_epoch, retire_epoch));
    // cannot use alloc_func here which may trigger dtor in free_func
    IntervalInfo *info = (IntervalInfo *)malloc(sizeof(IntervalInfo));
    info->obj = mem;
    info->birth_epoch = *(uint64_t*)((uintptr_t)mem + core->config.mem_size); // mt_GetBirth(core, mem);
    info->retire_epoch = ATOMIC_VAR_LOAD(&core->epoch); // mt_GetEpoch(core);
    info->next = *field;
    *field = info;
    if (core->config.collect && core->retire_counters[tid] % core->config.empty_freq == 0) {
        mt_Empty(core, tid);
    }
    ++core->retire_counters[tid];
}
static void mt_CoreStartOp(mt_Core *core, int tid)
{
    uint64_t e = ATOMIC_VAR_LOAD(&core->epoch);
	//sequence matters.
    ATOMIC_VAR_STOR(&core->lower_reservs[tid], e);
    ATOMIC_VAR_STOR(&core->upper_reservs[tid], e);
}
static void mt_CoreEndOp(mt_Core *core, int tid)
{
	//sequence matters.
    ATOMIC_VAR_STOR(&core->upper_reservs[tid], UINT64_MAX);
    ATOMIC_VAR_STOR(&core->lower_reservs[tid], UINT64_MAX);
}

// TODO(lgY): Interval only use lower_reservs, try to reduce functions
// maybe we can use mt_CoreCreateInterval/mt_CoreCreateRangeNew to store
// type in mt_Core and use it in tracker functions
void mt_InitFuncInterval(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->acquire_func    = mt_CoreAcquire;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreStartOp;
    handle->end_op_func     = mt_CoreEndOp;
}

void mt_InitFuncRangeNew(mt_Inst *handle)
{
    handle->create_func     = mt_CoreCreate;
    handle->destroy_func    = mt_CoreDestroy;
    handle->alloc_func      = mt_CoreAlloc;
    handle->reclaim_func    = mt_CoreReclaim;
    handle->acquire_func    = mt_CoreAcquire;
    handle->retire_func     = mt_CoreRetire;
    handle->start_op_func   = mt_CoreStartOp;
    handle->end_op_func     = mt_CoreEndOp;
}
