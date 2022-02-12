#define _ENABLE_ATOMIC_ALIGNMENT_FIX 1    // VS2015 SP2 BUG
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cassert>
template<typename _Ty>
struct NodeT
{
  std::unique_ptr<_Ty> data;
  NodeT* next;
  NodeT(const _Ty& _val, NodeT* _next)
    : data(new _Ty(_val))
    , next(_next)
  {
  }
};
template<size_t _SleepWhenAcquireFailedInMicroSeconds = size_t(-1)>
class SpinLockByTasT
{
  std::atomic_flag locked_flag_ = ATOMIC_FLAG_INIT;
public:
  void lock()
  {
    while (locked_flag_.test_and_set()) {
      if (_SleepWhenAcquireFailedInMicroSeconds == size_t(-1)) {
        std::this_thread::yield();
      } else if (_SleepWhenAcquireFailedInMicroSeconds != 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(_SleepWhenAcquireFailedInMicroSeconds));
      }
    }
  }
  void unlock()
  {
    locked_flag_.clear();
  }
};
template<size_t _SleepWhenAcquireFailedInMicroSeconds = size_t(-1)>
class SpinLockByCasT
{
  std::atomic<bool> locked_flag_ = ATOMIC_VAR_INIT(false);
public:
  void lock()
  {
    bool exp = false;
    while (!locked_flag_.compare_exchange_strong(exp, true)) {
      exp = false;
      if (_SleepWhenAcquireFailedInMicroSeconds == size_t(-1)) {
        std::this_thread::yield();
      } else if (_SleepWhenAcquireFailedInMicroSeconds != 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(_SleepWhenAcquireFailedInMicroSeconds));
      }
    }
  }
  void unlock()
  {
    locked_flag_.store(false);
  }
};
template<typename _Ty, typename _Lock>
class LockedStackT
{
  typedef NodeT<_Ty> Node;
public:
  ~LockedStackT()
  {
    std::lock_guard<_Lock> lock(lock_);
    while (head_) {
      Node* node = head_;
      head_ = node->next;
      delete node;
    }
  }
  void Push(const _Ty& val)
  {
    Node* node(new Node(val, nullptr)); // 不需要锁构造函数，这个可能是一个耗时操作
    std::lock_guard<_Lock> lock(lock_);
    node->next = head_;
    head_ = node;
  }
  std::unique_ptr<_Ty> Pop()
  {
    std::unique_ptr<_Ty> ret;
    Node* node;
    {
      // 同上，只需要锁链表本身，其他操作可以放到链表外执行
      std::lock_guard<_Lock> lock(lock_);
      node = head_;
      if (node) head_ = node->next;
    }
    if (node) {
      ret.swap(node->data);
      delete node;
    }
    return std::move(ret);
  }
 
private:
  Node* head_ = nullptr;
  _Lock lock_;
};
template<typename _Node>
class MemoryReclamationByReferenceCountingT
{
  std::atomic<size_t> counter_ = ATOMIC_VAR_INIT(0);
  std::atomic<_Node*> will_be_deleted_list_ = ATOMIC_VAR_INIT(nullptr);
  void InsertToList(_Node* first, _Node* last)
  {
    last->next = will_be_deleted_list_;
    while (!will_be_deleted_list_.compare_exchange_strong(last->next, first));
  }
  void InsertToList(_Node* head)
  {
    _Node* last = head;
    while (_Node* next = last->next) last = next;
    InsertToList(head, last);
  }
public:
  ~MemoryReclamationByReferenceCountingT()
  {
    // 如果线程正常退出，Reference Counting算法能删除所有数据
    assert(will_be_deleted_list_.load() == nullptr);
    assert(counter_.load() == 0);
    _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
    while (to_delete_list) {
      _Node* node = to_delete_list;
      to_delete_list = node->next;
      delete node;
    }
  }
  inline void Addref()
  {
    ++counter_;
  }
  inline bool Store(_Node* node) { return true; }
  bool Release(_Node* node)
  {
    if (!node) return false;
    if (counter_ == 1)
    {
      _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
      if (!--counter_) {
        while (to_delete_list) {
          _Node* node = to_delete_list;
          to_delete_list = node->next;
          delete node;
        }
      } else if (to_delete_list) {
        InsertToList(to_delete_list);
      }
      delete node;
    } else {
      if (node) InsertToList(node, node);
      --counter_;
    }
    return true;
  }
};
template<class _Node, size_t _MaxPopThreadCount = 16>
class MemoryReclamationByHazardPointerT
{
  struct HazardPointer
  {
    std::atomic<std::thread::id> id;
    std::atomic<_Node*> ptr = ATOMIC_VAR_INIT(nullptr);
  };
  HazardPointer hps_[_MaxPopThreadCount];
  std::atomic<_Node*> will_be_deleted_list_ = ATOMIC_VAR_INIT(nullptr);
  void _ReleaseImpl(_Node* node)
  {
    // 检查HazardPointers中是否有线程正在访问当前指针
    size_t i = 0;
    while (i < _MaxPopThreadCount) {
      if (hps_[i].ptr.load() == node)
        break;
      ++i;
    }
    if (i == _MaxPopThreadCount) { // 无任何线程正在访问当前指针，直接删除
      delete node;
    } else {  // 有线程正在访问，加入缓存列表
      node->next = will_be_deleted_list_.load();
      while (!will_be_deleted_list_.compare_exchange_strong(node->next, node));
    }
  }
public:
  ~MemoryReclamationByHazardPointerT()
  {
    // 自己不能删除自己，正常退出HazardPointer始终会持有一个节点，只能在此做清理
    size_t count = 0;
    _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
    while (to_delete_list) {
      _Node* node = to_delete_list;
      to_delete_list = node->next;
      delete node;
      ++count;
    }
    assert(count < 2);
  }
  inline void Addref() {}
  bool Store(_Node* node)
  {
    struct HazardPointerOwner
    {
      HazardPointer* hp;
      HazardPointerOwner(HazardPointer* hps)
        : hp(nullptr)
      {
        for (size_t i = 0; i < _MaxPopThreadCount; ++i) {
          std::thread::id id;
          if (hps[i].id.compare_exchange_strong(id, std::this_thread::get_id())) {
            hp = &hps[i];
            break;
          }
        }
      }
      ~HazardPointerOwner()
      {
        if (hp) {
          hp->ptr.store(nullptr);
          hp->id.store(std::thread::id());
        }
      }
    };
    thread_local HazardPointerOwner owner(hps_);
    if (!node || !owner.hp) return false;
    owner.hp->ptr.store(node);
    return true;
  }
  bool Release(_Node* node)
  {
    if (!node) return false;
    _ReleaseImpl(node); // 对当前传入指针进行释放操作
                        // 循环检测will_be_deleted_list_, 可以另开一个线程定时检测效率会更高
    _Node* to_delete_list = will_be_deleted_list_.exchange(nullptr);
    while (to_delete_list) {
      _Node* node = to_delete_list;
      to_delete_list = node->next;
      _ReleaseImpl(node);
    }
    return true;
  }
};
template<typename _Ty, typename _MemoryReclamation>
class LockFreeStackT
{
  typedef NodeT<_Ty> Node;
  struct TaggedPointer
  {
    Node* ptr;
    size_t tag;
    TaggedPointer() {}
    TaggedPointer(Node* _ptr, size_t _tag)
      : ptr(_ptr)
      , tag(_tag)
    {
    }
  };
public:
  ~LockFreeStackT()
  {
    TaggedPointer o(nullptr, 0);
    head_.exchange(o);

    Node* head = o.ptr;
    while (head) {
      Node* node = head;
      head = node->next;
      delete node;
    }
  }
  void Push(const _Ty& val)
  {
    TaggedPointer o = head_.load();
    TaggedPointer n(new Node(val, o.ptr), o.tag + 1);
    while (!head_.compare_exchange_strong(o, n)) {
      n.ptr->next = o.ptr;
      n.tag = o.tag + 1;
    }
  }
  std::unique_ptr<_Ty> Pop()
  {
    memory_reclamation_.Addref();
    TaggedPointer o = head_.load(), n;
    while (true) {
      if (!o.ptr) break;
      memory_reclamation_.Store(o.ptr);
      // HazardPointer算法储存(相当于上锁)后，需要对有效值进行二次确认，否则还是有先删除的问题
      // 这样做并没效率问题，不等的情况CAS操作也会进行循环，因此可以作为针对任何内存回收算法的固定写法
      const TaggedPointer t = head_.load();
      if (memcmp(&t, &o, sizeof(TaggedPointer))) {
        o = t;
        continue;
      }
      n.ptr = o.ptr->next;
      n.tag = o.tag + 1;
      if (head_.compare_exchange_strong(o, n)) break;
    }
    memory_reclamation_.Store(nullptr);
    std::unique_ptr<_Ty> ret;
    if (o.ptr) {
      ret.swap(o.ptr->data);
      memory_reclamation_.Release(o.ptr);
    }
    return std::move(ret);
  }
private:
  std::atomic<TaggedPointer> head_ = ATOMIC_VAR_INIT(TaggedPointer(nullptr, 0));
  _MemoryReclamation memory_reclamation_;
};
template<typename _Ty, int _ThreadCount = 16, int _LoopCount = 100000>
struct LockFreePerformanceTestT
{
  template<class _ProcessUnit>
  static double Run(_ProcessUnit puf)
  {
    std::thread ths[_ThreadCount];
    auto st = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < _ThreadCount; ++i) ths[i] = std::thread([&puf]() {
      for (int i = 0; i < _LoopCount; ++i) {
        puf();
      }
    });
    for (int i = 0; i < _ThreadCount; ++i) ths[i].join();
    const double period_in_ms = static_cast<double>((std::chrono::high_resolution_clock::now() - st).count())
      / std::chrono::high_resolution_clock::period::den * 1000;
    return period_in_ms;
  }
  static void Run()
  {
    _Ty s;
    std::cout << Run([&s]() {s.Push(0); }) << "\t\t";
    std::cout << Run([&s]() { s.Pop(); }) << std::endl;
  }
};
int main()
{
  std::cout << "LockedStack with std::mutex" << "\t\t\t\t\t";
  LockFreePerformanceTestT<LockedStackT<uint32_t, std::mutex>>::Run();
  std::cout << "LockedStack with SpinLockByTas yield" << "\t\t\t\t";
  LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<>>>::Run();
  std::cout << "LockedStack with SpinLockByCas yield" << "\t\t\t\t";
  LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<>>>::Run();
  std::cout << "LockedStack with SpinLockByTas usleep(5)" << "\t\t\t";
  LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByTasT<5>>>::Run();
  std::cout << "LockedStack with SpinLockByCas usleep(5)" << "\t\t\t";
  LockFreePerformanceTestT<LockedStackT<uint32_t, SpinLockByCasT<5>>>::Run();
  std::cout << "LockFreeStack with MemoryReclamationByReferenceCounting" << "\t\t";
  LockFreePerformanceTestT<LockFreeStackT<uint32_t, MemoryReclamationByReferenceCountingT<NodeT<uint32_t>>>>::Run();
  std::cout << "LockFreeStack with MemoryReclamationByHazardPointer" << "\t\t";
  LockFreePerformanceTestT<LockFreeStackT<uint32_t, MemoryReclamationByHazardPointerT<NodeT<uint32_t>>>>::Run();
  return 0;
}
