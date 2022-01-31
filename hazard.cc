#include <atomic>
#include <memory>
#include <unordered_set>

template <class T>
struct HazardPointer {
 public:
  class Holder {
   public:
    explicit Holder(HazardPointer<T> *pointer) : pointer_(pointer) {}
    Holder(const HazardPointer &) = delete;
    ~Holder() { pointer_->Release(); }
    T *get() const { return pointer_->target_.load(std::memory_order_acquire); }
    operator bool() const { return get(); }
    T *operator->() const { return get(); }
    T &operator*() const { return *get(); }

   private:
    HazardPointer<T> *pointer_;
  };

 public:
  ~HazardPointer() {
    if (next_) {
      delete next_;
    }
  }

  void Release() {
    target_.store(nullptr, std::memory_order_release);
    active_.clear(std::memory_order_release);
  }

  static Holder Acquire(const std::atomic<T *> &target) {
    auto pointer = HazardPointer<T>::Alloc();

    do {
      pointer->target_ = target.load(std::memory_order_acquire);
    } while (pointer->target_.load(std::memory_order_acquire) !=
             target.load(std::memory_order_acquire));
    return Holder(pointer);
  }

  static void Update(std::atomic<T *> &target, T *new_target) {
    auto old = target.exchange(new_target);
    Retire(old);
  }

  static void Reclaim() {
    // collect in-use pointers
    std::unordered_set<T *> in_use;
    for (auto p = head_list_.load(std::memory_order_acquire); p; p = p->next_) {
      in_use.insert(p->target_);
    }

    // delete useless pointers
    List *retire_head = nullptr;
    List *retire_tail = nullptr;

    auto p = retire_list_.exchange(nullptr);
    while (p != nullptr) {
      auto next = p->next;

      if (in_use.count(p->target.get()) == 0) {
        delete p;
      } else {
        p->next = retire_head;
        retire_head = p;
        if (retire_tail == nullptr) {
          retire_tail = p;
        }
      }

      p = next;
    }

    if (retire_head) {
      // push to retired list again
      auto &tail = retire_tail->next;
      do {
        tail = retire_list_.load(std::memory_order_acquire);
      } while (!retire_list_.compare_exchange_weak(tail, retire_head));
    }
  }

 private:
  static HazardPointer<T> *Alloc() {
    for (auto p = head_list_.load(std::memory_order_acquire); p; p = p->next_) {
      if (!p->active_.test_and_set()) {
        return p;
      }
    }

    auto p = new HazardPointer<T>();
    p->active_.test_and_set();
    do {
      p->next_ = head_list_.load(std::memory_order_acquire);
    } while (!head_list_.compare_exchange_weak(p->next_, p));
    return p;
  }

  static void Retire(T *ptr) {
    auto p = new List;
    p->target.reset(ptr);
    do {
      p->next = retire_list_.load(std::memory_order_acquire);
    } while (!retire_list_.compare_exchange_weak(p->next, p));

    if (++retire_count_ == 1000) {
      retire_count_ = 0;
      Reclaim();
    }
  }

 private:
  struct List {
    std::unique_ptr<T> target{nullptr};
    List *next = nullptr;
  };

 private:
  std::atomic<T *> target_{nullptr};
  HazardPointer<T> *next_;
  std::atomic_flag active_ = ATOMIC_FLAG_INIT;

  static std::atomic<HazardPointer<T> *> head_list_;
  static std::atomic<uint32_t> retire_count_;
  static std::atomic<List *> retire_list_;
};

template <class T>
std::atomic<HazardPointer<T> *> HazardPointer<T>::head_list_{nullptr};
template <class T>
std::atomic<uint32_t> HazardPointer<T>::retire_count_{0};
template <class T>
std::atomic<typename HazardPointer<T>::List *> HazardPointer<T>::retire_list_{
    nullptr};

// example
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<int> g_count{0};
class A {
 public:
  explicit A(int value) : value_(value) { ++g_count; }
  ~A() { --g_count; }
  int Value() { return value_; }

 private:
  int value_;
};

int main() {
  std::atomic<A *> target{new A(0)};

  constexpr int N = 8;
  constexpr int M = 1000000;
  constexpr int W = 1000;
  std::vector<std::thread> threads;
  std::atomic<uint64_t> sum{0};
  for (int t = 0; t < N; ++t) {
    threads.emplace_back([&, t] {
      for (int i = 0; i < M; ++i) {
        if (i % W == 0) {
          // write less
          HazardPointer<A>::Update(target, new A(t * M + i));
        } else {
          // read more
          auto holder = HazardPointer<A>::Acquire(target);
          sum.fetch_add(holder->Value());
        }
      }
    });
  }

  // wait finish
  for (auto &thread : threads) {
    thread.join();
  }

  HazardPointer<A>::Reclaim();
  printf("Remain: %d\n", g_count.load());
}
