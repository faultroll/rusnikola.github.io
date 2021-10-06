
#include "rideables/SortedUnorderedMap.hpp"

int main(void)
{
    SortedUnorderedMap<std::string, std::string> *p = new SortedUnorderedMapFactory<std::string, std::string>();
    delete p;

    return 0;
}
