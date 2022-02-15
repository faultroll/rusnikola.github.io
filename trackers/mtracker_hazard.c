
#include "trackers/mtracker_impl.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include "atomic_c.h"
#include "align_c.h"

#define MAX_HP		16
typedef struct HazardInfo HazardInfo;
struct HazardInfo {
	HazardInfo* next;
};
typedef struct HazardSlot HazardSlot;
struct HazardSlot {
	ATOMIC_VAR(void *) entry[MAX_HP];
	// alignas(128) char pad[0];
};

struct mt_Core {
    // private:
    // mt_Type type; // No need, just different function
    mt_Config config;
    // public:
	HazardSlot* slots;
	HazardSlot* local_slots;
	HazardInfo** retired; // padded
	int* cntrs; // padded
};

static inline
void mt_ReserveSlot(HazardSlot *slots, int tid, int sid, void* ptr){
	slots[tid].entry[sid] = ptr;
}
static inline
void mt_ClearSlot(HazardSlot *slots, int tid, int sid){
    mt_ReserveSlot(slots, tid, sid, NULL);
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem);

static mt_Core *mt_CoreCreate(mt_Config config)
{
    mt_Core *core = malloc(sizeof(mt_Core));
    core->config = config;

    int task_num = core->config.task_num,
        slot_num = core->config.slot_num;
	core->slots = (HazardSlot*) aligned_alloc(alignof(HazardSlot), sizeof(HazardSlot) * task_num);
	core->local_slots = (HazardSlot*) aligned_alloc(alignof(HazardSlot), sizeof(HazardSlot) * task_num * task_num);
	for (int i = 0; i < task_num; i++) {
		for (int j = 0; j < slot_num; j++) {
			core->slots[i].entry[j]=NULL; // mt_ClearSlot(core->slots, i, j);
		}
	}
	core->retired = malloc(sizeof(HazardInfo*) * task_num);
	core->cntrs = malloc(sizeof(int) * task_num);
	for (int i = 0; i < task_num; i++){
		core->retired[i] = NULL;
		core->cntrs[i] = 0;
	}

    return core;
}
static void mt_CoreDestroy(mt_Core *core)
{
    int task_num = core->config.task_num;
	for (int i = 0; i < task_num; i++) {
		HazardInfo** field = &(core->retired[i]);
		HazardInfo* info = *field;
		if (info == NULL) continue;
		do {
			HazardInfo* curr = info;
			info = curr->next;
			void *ptr = (void *)((uintptr_t)curr - core->config.mem_size);
			*field = info;
			mt_CoreReclaim(core, i, ptr);
			// dec_retired(i);
		} while (info != NULL);
	}
    free(core->cntrs);
    free(core->retired);
    aligned_free(core->local_slots);
    aligned_free(core->slots);
    free(core);
}
static void *mt_CoreAlloc(mt_Core *core, int tid)
{
    return core->config.alloc_func(core->config.mem_size+sizeof(HazardInfo));
}
static void mt_CoreReclaim(mt_Core *core, int tid, void *mem)
{
    core->config.free_func(mem);
}
static void *mt_CoreAcquire(mt_Core *core, int tid, int sid, void *volatile mem)
{
	void* ret;
	void* realptr;
	while(true){
		ret = (void *)mem; // ATOMIC_VAR_LOAD(&mem); // atomic load here
		realptr = ret; // (void*)((uintptr_t)ret & 0xfffffffffffffffc); // find why origin use bitmask
		mt_ReserveSlot(core->slots, tid, sid, realptr);
		if(ret == mem){ // mem should be volatile and? atomic
			return ret;
		}
	}
    /* // same as prev, in order to make this func atomic
    void *ptr;
    do {
        ptr = mem;
        mt_ReserveSlot(core->slots, tid, sid, ptr);
    } while(ptr != mem);
    return ptr; */
}
static void mt_Empty(mt_Core *core, int tid)
{
    int task_num = core->config.task_num,
        slot_num = core->config.slot_num;
	HazardSlot* local = core->local_slots + tid * task_num;
	HazardInfo** field = &(core->retired[tid]);
	HazardInfo* info = *field;
	if (info == NULL) return;
	for (int i = 0; i < task_num; i++) {
		for (int j = 0; j < slot_num; j++) {
			mt_ReserveSlot(local, tid, j, core->slots[i].entry[j]);
		}
	}
	do {
		HazardInfo* curr = info;
		info = curr->next;
		bool danger = false;
		void *ptr = (void *)((uintptr_t)curr - core->config.mem_size);
		for (int i = 0; i < task_num; i++){
			for (int j = 0; j < slot_num; j++){ 
				if (ptr == local[i].entry[j]){
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
    if (NULL == mem) return;
	HazardInfo** field = &(core->retired[tid]);
	HazardInfo* info = (HazardInfo*) ((uintptr_t)mem + core->config.mem_size);
	info->next = *field;
	*field = info;
	if (core->config.collect && core->cntrs[tid]==core->config.empty_freq){
		core->cntrs[tid]=0;
		mt_Empty(core, tid);
	}
	++core->cntrs[tid];
}
static void mt_CoreStartOp(mt_Core *core, int tid)
{

}
static void mt_CoreEndOp(mt_Core *core, int tid)
{
    int slot_num = core->config.slot_num;
	for(int i = 0; i<slot_num; i++){
		mt_ClearSlot(core->slots, tid, i);
	}
}

void mt_InitFuncHazard(mt_Inst *handle)
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
