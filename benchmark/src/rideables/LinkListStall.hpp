/*

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 

*/


#ifndef LINK_LIST_STALL
#define LINK_LIST_STALL

#include "SortedUnorderedMapStall.hpp"

#ifdef NGC
#define COLLECT false
#else
#define COLLECT true
#endif

template <class K, class V>
class LinkListStallFactory : public RideableFactory{
	SortedUnorderedMapStall<K,V>* build(GlobalTestConfig* gtc){
		return new SortedUnorderedMapStall<K,V>(gtc,1);
	}
};

#endif
