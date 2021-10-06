/*

Copyright 2017 University of Rochester

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


#ifndef RETIREDMONITORABLE_HPP
#define RETIREDMONITORABLE_HPP

#include <queue>
#include <list>
#include <vector>
#include <atomic>
// #include "ConcurrentPrimitives.hpp"
// #include "RAllocator.hpp"
#include "MemoryTracker.hpp"

extern int task_num_;

class RetiredMonitorable{
private:
	int64_t* retired_cnt; // padded
	BaseMT* mem_tracker = NULL;
public:
	RetiredMonitorable(){
		int task_num = task_num_; // gtc->task_num+gtc->task_stall;
		retired_cnt = new int64_t[task_num];
		for (int i=0; i<task_num; i++){
			retired_cnt[i] = 0;
		}
	}

	void setBaseMT(BaseMT* base) {
		mem_tracker = base;
	}

	void collect_retired_size(int64_t size, int tid){
		retired_cnt[tid] += size;
	}
	int64_t report_retired(int tid){
		//calling this function at the end of the benchmark
		if (mem_tracker != NULL)
			mem_tracker->lastExit(tid);
		return retired_cnt[tid];
	}
};

#endif
