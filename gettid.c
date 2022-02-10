
#include "thread_c.h"
#include "atomic_c.h"

// TODO(lgY): Try to support multi |memory_tracker|, eg.
// 1. thread a may have tid_ 1 in tracker a and tid_ 3 in tracker b
// 2. like iter, may need dynamic tracker

static ATOMIC_VAR(int) thrd_num_ = ATOMIC_VAR_INIT(0);
static thread_local int tid_ = 0;
static thread_local once_flag flag_gen_ = ONCE_FLAG_INIT;
static void mt_OnceGenTid(void)
{
    // ATOMIC_VAR_FAA(&thrd_num_, 1); tid_ = thrd_num_;
    do {
        tid_ = ATOMIC_VAR_LOAD(&thrd_num_);
    } while (!ATOMIC_VAR_CAS(&thrd_num_, tid_, tid_ + 1));
}

int mt_GetTid(void)
{
    call_once(&flag_gen_, mt_OnceGenTid); // each thread will call once
    return tid_;
}
