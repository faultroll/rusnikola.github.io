
// global variables (for extern)
int count_retired_ = 0;
int task_num_ = 24; // total thread number
int task_stall_ = 8; // stall threads

#if 0
#include "rideables/SGLUnorderedMap.hpp"
#include "rideables/SortedUnorderedMap.hpp"
#include "rideables/LinkList.hpp"
#include "rideables/BonsaiTree.hpp"
#include "rideables/NatarajanTree.hpp"

int main(void)
{
    RideableFactory *p = new SortedUnorderedMapFactory<std::string, std::string>();
    Rideable *r = p->build();
    RUnorderedMap<std::string, std::string> *q = dynamic_cast<RUnorderedMap<std::string, std::string> *>(r);
    delete r;
    delete p;

    return 0;
}

#else // 0
/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"


extern void TestMTCreateAndDestroy(CuTest *);
extern void TestMTAllocAndReclaim(CuTest *);
extern void TestMTWriteAndRead(CuTest *tc);

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

    handle = mt_Create(NIL, MT_DEFAULT_CONF(sizeof(CuString)));
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(RCU, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(QSBR, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);
    handle = mt_Create(SSMEM, config);
    CuAssertTrue(tc, handle != NULL);
    mt_Destroy(handle);

    printf("%s done\n", __func__);
}
void TestMTAllocAndReclaim(CuTest *tc)
{
    mt_Type type = SSMEM;
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
    mt_Type type = SSMEM;
    mt_Inst *handle = NULL;
    mt_Config config = MT_DEFAULT_CONF(sizeof(CuString));
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

int main(void)
{
    RunAllTests();

    return 0;
}

#endif // 0