
#include "cthread.h"
#include "catomic.h"
#include "ctime.h"

// TODO try to support multi |memory_tracker|
// eg. thread a may have tid_ 1 in tracker a and tid_ 3 in tracker b

static ATOMIC_VAR(int) thrd_num_ = ATOMIC_VAR_INIT(0);
static thread_local int tid_ = 0;
static thread_local once_flag flag_tid_ = ONCE_FLAG_INIT;
static void OnceGetTid(void)
{
    do {
        tid_ = ATOMIC_VAR_LOAD(&thrd_num_);
    } while (!ATOMIC_VAR_CAS(&thrd_num_, tid_, tid_ + 1));
}

int GetTid(void)
{
    call_once(&flag_tid_, OnceGetTid); // each thread will call once
    return tid_;
}
