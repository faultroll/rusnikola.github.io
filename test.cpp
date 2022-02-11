
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
#include <sstream>
#include "thread_c.h"

extern "C" {
    int mt_GetTid(void);
}

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
    thrd_detach(thrd_current()); // both are OK

    RUnorderedMap<std::string, std::string> *m = static_cast<RUnorderedMap<std::string, std::string> *>(args);
    int tid = mt_GetTid();
    put_get(m, "b", "b", tid);
    put_get(m, "c", "c", tid);
    remove_get(m, "b", tid);

    remove_get(m, "c", tid); // remove this will cause memleak, why?
    RetiredMonitorable *rm_ptr = dynamic_cast<RetiredMonitorable *>(m);
    std::ostringstream stream;
    stream.str("");
    stream << tid << " retired_cnt: " << rm_ptr->report_retired(tid) << std::endl;
    std::cout << stream.str();

    return 0;
}


int main(void)
{
    RideableFactory *p = new SortedUnorderedMapFactory<std::string, std::string>();
    Rideable *r = p->build();
    RUnorderedMap<std::string, std::string> *q = dynamic_cast<RUnorderedMap<std::string, std::string> *>(r);

    // execute(q);
    thrd_t t1, t2, t3, t4;
    thrd_create(&t1, execute, q);
    thrd_create(&t2, execute, q);
    thrd_create(&t3, execute, q);
    thrd_create(&t4, execute, q);

    sleep(1); // if detached, sleep to wait
    // thrd_join(t1, NULL);
    // thrd_join(t2, NULL);
    // thrd_join(t3, NULL);
    // thrd_join(t4, NULL);

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