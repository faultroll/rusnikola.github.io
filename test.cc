
#include "rideables/SGLUnorderedMap.hpp"
#include "rideables/SortedUnorderedMap.hpp"
// #include "rideables/SortedUnorderedMapStall.hpp"
#include "rideables/LinkList.hpp"
// #include "rideables/LinkListStall.hpp"
// #include "rideables/BonsaiTree.hpp"
// #include "rideables/NatarajanTree.hpp"

int count_retired_ = 0;
int task_num_ = 32;

int main(void)
{
    RideableFactory *p = new SortedUnorderedMapFactory<std::string, std::string>();
    Rideable *r = p->build();
    RUnorderedMap<std::string, std::string> *q = dynamic_cast<RUnorderedMap<std::string, std::string> *>(r);
    delete r;
    delete p;

    return 0;
}
