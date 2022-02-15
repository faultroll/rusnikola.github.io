
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include "atomic_c.h"
#include "align_c.h"

#define MAX_HE		16
typedef struct HEInfo HEInfo;
struct HEInfo {
	HEInfo* next;
	uint64_t birth_epoch;
	uint64_t retire_epoch;
};
typedef struct HESlot HESlot;
struct HESlot {
	ATOMIC_VAR(uint64_t) entry[MAX_HE];
	// alignas(128) char pad[0];
};

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
	HESlot* reservations;
	HESlot* local_reservations;
	uint64_t* retire_counters; // padded
	uint64_t* alloc_counters; // padded
	HEInfo** retired; // padded

	ATOMIC_VAR(uint64_t) epoch; // padded
};

/* static inline
void mt_Reserve(mt_Core *core, int tid, int sid, void* ptr){
	uint64_t prev_epoch = ATOMIC_VAR_LOAD(&core->reservations[tid].entry[sid]);
	while(true){
		uint64_t curr_epoch = ATOMIC_VAR_LOAD(&core->epoch);
		if (curr_epoch == prev_epoch){
			return;
		} else {
            mt_ReserveSlot(core->reservations, tid, sid, curr_epoch);
			prev_epoch = curr_epoch;
		}
	}
} */
static inline
void mt_ReserveSlot(HESlot *reservations, int tid, int sid, uint64_t epoch){
    ATOMIC_VAR_STOR(&reservations[tid].entry[sid], epoch);
}
static inline
void mt_ClearSlot(HESlot *reservations, int tid, int sid){
    mt_ReserveSlot(reservations, tid, sid, 0);
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem);

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;

    int task_num = core->config.task_num,
        slot_num = core->config.slot_num;
	core->reservations = (HESlot*) aligned_alloc(alignof(HESlot), sizeof(HESlot) * task_num);
	core->local_reservations = (HESlot*) aligned_alloc(alignof(HESlot), sizeof(HESlot) * task_num * task_num);
	for (int i = 0; i < task_num; i++) {
		for (int j = 0; j < slot_num; j++) {
			core->reservations[i].entry[j]=0; // mt_ClearSlot(core->reservations, i, j);
		}
	}
	core->retired = malloc(sizeof(HEInfo*) * task_num);
	core->retire_counters = malloc(sizeof(uint64_t) * task_num);
	core->alloc_counters = malloc(sizeof(uint64_t) * task_num);
	for (int i = 0; i < task_num; i++) {
        core->retired[i] = NULL;
        core->retire_counters[i] = 0;
        core->alloc_counters[i] = 0;
	}
	ATOMIC_VAR_STOR(&core->epoch, 1);

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    int task_num = core->config.task_num;
	for (int i = 0; i < task_num; i++) {
		HEInfo** field = &(core->retired[i]);
		HEInfo* info = *field;
		if (info == NULL) continue;
		do {
			HEInfo* curr = info;
			info = curr->next;
			void *ptr = (void *)((uintptr_t)curr - core->config.mem_size);
			*field = info;
			mt_CoreReclaim(core, i, ptr);
			// dec_retired(i);
		} while (info != NULL);
	}
    free(core->alloc_counters);
    free(core->retire_counters);
    free(core->retired);
    aligned_free(core->local_reservations);
    aligned_free(core->reservations);
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
	if((++core->alloc_counters[tid])%
       (core->config.epoch_freq * core->config.task_num)==0){
		ATOMIC_VAR_FAA(&core->epoch, 1);
	}
	uintptr_t block = (uintptr_t) core->config.alloc_func(core->config.mem_size+sizeof(HEInfo));
	HEInfo* info = (HEInfo*) (block + core->config.mem_size);
	info->birth_epoch = ATOMIC_VAR_LOAD(&core->epoch);
	return (void*)block;
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    core->config.free_func(mem);
}
static void *mt_CoreAcquire(mt_Core *core, int tid, int sid, void *volatile mem)
{
	uint64_t prev_epoch = ATOMIC_VAR_LOAD(&core->reservations[tid].entry[sid]);
	while(true){
		void* ptr = (void *)mem;
		uint64_t curr_epoch = ATOMIC_VAR_LOAD(&core->epoch);
		if (curr_epoch == prev_epoch){
			return ptr;
		} else {
			mt_ReserveSlot(core->reservations, tid, sid, curr_epoch);
			prev_epoch = curr_epoch;
		}
	}
}
static inline
bool mt_Danger(const uint64_t epo, uint64_t birth_epoch, uint64_t retire_epoch) {
	if (epo < birth_epoch || epo > retire_epoch || epo == 0){
	    return false;
	} else {
		return true;
	}
}
static void mt_Empty(mt_Core *core, int tid)
{
	// erase safe objects
    int task_num = core->config.task_num,
        slot_num = core->config.slot_num;
	HESlot* local = core->local_reservations + tid * task_num;
	HEInfo** field = &(core->retired[tid]);
	HEInfo* info = *field;
	if (info == NULL) return;
	for (int i = 0; i < task_num; i++) {
		for (int j = 0; j < slot_num; j++) {
            mt_ReserveSlot(local, i, j, core->reservations[i].entry[j]);
		}
	}
	do {
		HEInfo* curr = info;
		info = curr->next;
		bool danger = false;
        void *ptr = (void *)((uintptr_t)curr - core->config.mem_size);
        for (int i = 0; i < task_num; i++){
            for (int j = 0; j < slot_num; j++){
                const uint64_t epo = ATOMIC_VAR_LOAD(&local[i].entry[j]);
                if (mt_Danger(epo, curr->birth_epoch, curr->retire_epoch)) {
                    danger = true;
                    break;
                }
            }
        }
		if (!danger) {
			*field = info;
			mt_CoreReclaim(core, tid, ptr);
			// dec_retired(tid);
			continue;
		}
		field = &curr->next;
	} while (info != NULL);
}
static void mt_CoreRetire(mt_Core *core, int tid, void *mem)
{
	if(mem==NULL){return;}
	HEInfo** field = &(core->retired[tid]);
	HEInfo* info = (HEInfo*) ((uintptr_t)mem + core->config.mem_size);
	info->retire_epoch = ATOMIC_VAR_LOAD(&core->epoch);
	info->next = *field;
	*field = info;
	if(core->config.collect && core->retire_counters[tid]%core->config.empty_freq==0){
		mt_Empty(core, tid);
	}
	++core->retire_counters[tid];
}
static void mt_CoreStartOp(mt_Core *core, int tid)
{

}
static void mt_CoreEndOp(mt_Core *core, int tid)
{
    int slot_num = core->config.slot_num;
	for(int i = 0; i<slot_num; i++){
		mt_ClearSlot(core->reservations, tid, i);
	}
}

void mt_InitFuncHE(mt_Inst *handle)
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
