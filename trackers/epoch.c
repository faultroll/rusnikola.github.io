#define N_THREADS 4 //一共4个线程
bool active[N_THREADS] = {false};
int epoches[N_THREADS] = {0};
int global_epoch = 0;
vector<int*> retire_list[3];
void read(int thread_id)
{
  active[thread_id] = true;
  epoches[thread_id] = global_epoch;
  //进入临界区了。可以安全的读取
  //...... 
  //读取完毕，离开临界区
  active[thread_id] = false;
}
void logical_deletion(int thread_id)
{
  active[thread_id] = true;
  epoches[thread_id] = global_epoch;
  //进入临界区了，这里，我们可以安全的读取
  //好了，假如说我们现在要删除它了。先逻辑删除。
  //而被逻辑删除的tmp指向的节点还不能马上被回收，因此把它加入到对应的retire list
  retire_list[global_epoch].push_back(tmp);
  //离开临界区
  active[thread_id] = false;
  //看看能不能物理删除
  try_gc();
}
bool try_gc()
{
  int &e = global_epoch;
  for (int i = 0; i < N_THREADS; i++) {
    if (active[i] && epoches[i] != e) {
        //还有部分线程没有更新到最新的全局的epoch值
        //这时候可以回收(e + 1) % 3对应的retire list。
        free((e + 1) % 3);//不是free(e)，也不是free(e-1)。参看下面
        return false;
    }
  }
  //更新global epoch
  e = (e + 1) % 3;
  //更新之后，那些active线程中，部分线程的epoch值可能还是e - 1（模3）
  //那些inactive的线程，之后将读到最新的值，也就是e。
  //不管如何，(e + 1) % 3对应的retire list的那些内存，不会有人再访问到了，可以回收它们了
  //因此epoch的取值需要有三种，仅仅两种是不够的。
  free((e + 1) % 3);//不是free(e)，也不是free(e-1)。参看下面
}
bool free(int epoch)
{
  for each pointer in retire_list[epoch]
    if (pointer is not NULL)
      delete pointer;
}
