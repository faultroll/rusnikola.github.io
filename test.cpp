
// global variables (for extern)
int count_retired_ = 1;
int task_num_ = 24; // total thread number
int task_stall_ = 8; // stall threads

#if 1
#include "rideables/SGLUnorderedMap.hpp"
#include "rideables/SortedUnorderedMap.hpp"
#include "rideables/LinkList.hpp"
#include "rideables/BonsaiTree.hpp"
#include "rideables/NatarajanTree.hpp"
#include "nbds/map.h"
#include <sstream>
#include <stdlib.h> // srand/rand
#include <time.h> // time
#include "thread_c.h"
#include "atomic_c.h"

extern "C" {
    int mt_GetTid(void);
}

#define kNumThreads 10
#define kNumIterations 1000000
static volatile ATOMIC_VAR(int) waiting_ = kNumThreads;
void put_get(RUnorderedMap<std::string, std::string> *m, std::string key, std::string value, int tid)
{
    std::ostringstream stream;

    stream.str("");
    stream << tid << " put<'" << key << "','" << value << "'>" << std::endl;
    std::cout << stream.str();
    m->put(key, value, tid);

    stream.str("");
    stream << tid << " get '" << key << "':'" << m->get(key, tid) << "'" << std::endl;
    std::cout << stream.str();
}
void remove_get(RUnorderedMap<std::string, std::string> *m, std::string key, int tid)
{
    std::ostringstream stream;

    stream.str("");
    stream << tid << " remove '" << key << "':'" << m->remove(key, tid) << "'" << std::endl;
    std::cout << stream.str();

    stream.str("");
    stream << tid << " get '" << key << "':'" << m->get(key, tid) << "'" << std::endl;
    std::cout << stream.str();
}
int execute(void *args)
{
    // thrd_detach(thrd_current()); // both are OK

    int tid = mt_GetTid();
    // RUnorderedMap<std::string, std::string> *m = static_cast<RUnorderedMap<std::string, std::string> *>(args);
    map_t *m = (map_t *)args;

    // wait all tasks begin
    ATOMIC_VAR_FAA(&waiting_, -1);
    do {  } while(waiting_);

    /* put_get(m, "b", "b", tid);
    put_get(m, "c", "c", tid);
    remove_get(m, "b", tid);
    remove_get(m, "c", tid); // remove this will cause memleak, why?
    RetiredMonitorable *rm_ptr = dynamic_cast<RetiredMonitorable *>(m);
    std::ostringstream stream;
    stream.str("");
    stream << tid << " retired_cnt: " << rm_ptr->report_retired(tid) << std::endl;
    std::cout << stream.str(); */

    // HE is OK
    // SSMEM not pass this test
    // Hazard not pass valgrind test (invalid read/...)
    // Others will leak memory (not reclaim when destroyed) 
    printf("(%zu) begin\n", (size_t)(uintptr_t)thrd_current());
    srand(time(NULL));
    for (int i = 0; i < kNumIterations / kNumThreads; ++i)
    {
        unsigned int r = rand();
        int key = r & 0xF;

        if (r & (1 << 8))
            // m->put(std::to_string(key + 1), "1", tid);
            map_add(m, (map_key_t)(key + 1), map_val_t(1));
        else
            // m->remove(std::to_string(key + 1), tid);
            map_remove(m, (map_key_t)(key + 1));
        
        if ((i % 3000) == 0) {
            // printf("(%zu) running iteration %d\n", (size_t)(uintptr_t)thrd_current(), i);
        }
    }
    printf("(%zu) end\n", (size_t)(uintptr_t)thrd_current());

    return 0;
}

int nbds_no_alloc(map_t *map)
{
    const int n = 20;
    map_key_t key = 0;
    map_val_t val = 0;

    srand(time(NULL));
    for (int i = 1; i < n; ++i)
    {
        // CAUTION key and val cannot be 0
        key = rand() % n + 1;
        val = rand() % n + 1; // n - key;
        printf("[%s] write: key (%d), val (%d).\n", __func__, (int)key, (int)val);
        map_add(map, key, val);
    }

    map_iter_t *iter = map_iter_alloc(map);
    printf("[%s] ============= begin iter\n", __func__);
    map_iter_begin(iter, DOES_NOT_EXIST);
    while ((val = map_iter_next(iter, &key)) != DOES_NOT_EXIST)
    {
        printf("[%s] read: key (%d), val (%d).\n", __func__, (int)key, (int)val);
    }
    printf("[%s] ============= reset iter\n", __func__);
    map_iter_begin(iter, DOES_NOT_EXIST);
    while ((val = map_iter_next(iter, &key)) != DOES_NOT_EXIST)
    {
        printf("[%s] read: key (%d), val (%d).\n", __func__, (int)key, (int)val);
    }
    printf("[%s] ============= end iter\n", __func__);
    map_iter_free(iter);

    map_print(map, false);

    return 0;
}

int main(void)
{
    RideableFactory *p = new SortedUnorderedMapFactory<std::string, std::string>();
    Rideable *r = p->build();
    // RUnorderedMap<std::string, std::string> *q = dynamic_cast<RUnorderedMap<std::string, std::string> *>(r);
    map_t *q = map_alloc(&MAP_IMPL_LL, NULL);

    // execute(q);
    // nbds_no_alloc(q);

    thrd_t tp[kNumThreads] = {0};
    size_t i, len = sizeof(tp) / sizeof(tp[0]);
    for (i = 0; i < len; i++) {
        int rc = thrd_create(&tp[i], execute, q);
        if (rc != 0) {
            fprintf(stderr, "thrd_create %zu error %#x\n", i, rc);
            continue;
        }
    }
    // sleep(1); // if detached, sleep to wait
    // printf("detach wait end, exit\n");
    for (i = 0; i < len; i++) {
        if (tp[i]) {
            thrd_join(tp[i], NULL);
        }
    }

    map_free(q);
    delete r;
    delete p;

    return 0;
}

#else // 0
/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"


extern "C" {
    void TestMTCreateAndDestroy(CuTest *);
    void TestMTAllocAndReclaim(CuTest *);
    void TestMTWriteAndRead(CuTest *);
}

void RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestMTCreateAndDestroy);
    SUITE_ADD_TEST(suite, TestMTAllocAndReclaim);
    SUITE_ADD_TEST(suite, TestMTWriteAndRead);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    CuStringDelete(output);
    CuSuiteDelete(suite);
}

int main(void)
{
    RunAllTests();

    return 0;
}


#include "mtracker.h"

void TestMTCreateAndDestroy(CuTest *tc)
{
    mt_Inst *handle = NULL;
    mt_Config config;
    config.task_num = 16;
    config.slot_num = 4;
    config.epoch_freq = 150;
    config.empty_freq = 30;
    config.collect = true;
    config.mem_size = sizeof(CuString);
    config.alloc_func = malloc;
    config.free_func = free;

    handle = mt_Create(MT_NIL, MT_DEFAULT_CONF(sizeof(CuString)));
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(MT_RCU, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(MT_QSBR, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(MT_SSMEM, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(MT_Hazard, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);

    printf("%s done\n", __func__);
}
void TestMTAllocAndReclaim(CuTest *tc)
{
    mt_Type type = MT_RCU;
    mt_Inst *handle = NULL;
    mt_Config config = MT_DEFAULT_CONF(sizeof(CuString));
    int tid = 0;
    void *mem1 = NULL, *mem2 = NULL;

    handle = mt_Create(type, config);
    CuAssertTrue(tc, handle != NULL);
    mem1 = mt_Alloc(handle, tid);
    CuAssertTrue(tc, mem1 != NULL);
    mem2 = mt_Alloc(handle, tid);
    CuAssertTrue(tc, mem2 != NULL);
    // mt_Reclaim(handle, tid, mem1);
    // mt_Reclaim(handle, tid, mem2);
    mt_Reclaim(handle, tid, mem2);
    mt_Reclaim(handle, tid, mem1);
    mt_Destroy(handle);

    printf("%s done\n", __func__);
}
void TestMTWriteAndRead(CuTest *tc)
{
    mt_Type type = MT_RCU;
    mt_Inst *handle = NULL;
    mt_Config config = MT_DEFAULT_CONF(sizeof(mt_Config));
    int tid = MT_DEFAULT_TID, sid = 2;
    void *mem_alloc = NULL;
    mt_Config *mem_read = NULL;

    handle = mt_Create(type, config);
    CuAssertTrue(tc, handle != NULL);
    mem_alloc = mt_Alloc(handle, tid);
    CuAssertTrue(tc, mem_alloc != NULL);
    mem_read = (mt_Config *)mt_Read(handle, tid, sid, mem_alloc);
    CuAssertTrue(tc, mem_read != NULL);
    mem_read->mem_size = 0xdeafbeaf;
    mem_read = (mt_Config *)mt_Read(handle, tid, sid, mem_alloc);
    CuAssertTrue(tc, mem_read != NULL);
    CuAssertTrue(tc, mem_read->mem_size == 0xdeafbeaf);
    mt_Reclaim(handle, tid, mem_alloc);
    mt_Destroy(handle);

    printf("%s done\n", __func__);
}

#endif // 0