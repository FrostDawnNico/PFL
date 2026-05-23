// 无锁（Lock-Free）生产者-消费者模型
// 无锁（Lock-Free）生产者-消费者模型通常依赖 原子操作（CAS）和 环形缓冲区。
// 相比互斥锁，它避免了线程挂起/唤醒的开销，在高并发下性能更好，但实现难度更高。

#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

template<typename T>
class LockFreeSPSCQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        Node(const T& val) : data(val), next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeSPSCQueue() {
        Node* dummy = new Node(T());
        head_ = dummy;
        tail_ = dummy;
    }

    ~LockFreeSPSCQueue() {
        while (pop());
        delete head_.load();
    }

    // 生产者调用
    void push(const T& value) {
        Node* newNode = new Node(value);
        Node* oldTail = tail_.load(std::memory_order_relaxed);
        oldTail->next.store(newNode, std::memory_order_release);
        tail_.store(newNode, std::memory_order_relaxed);
    }

    // 消费者调用
    bool pop(T* result = nullptr) {
        Node* oldHead = head_.load(std::memory_order_relaxed);
        Node* next = oldHead->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return false; // 队列为空
        }

        if (result) {
            *result = next->data;
        }

        head_.store(next, std::memory_order_relaxed);
        delete oldHead;
        return true;
    }
};

// int main() {
//     LockFreeSPSCQueue<int> queue;
//
//     std::thread producer([&]() {
//         for (int i = 0; i < 10; ++i) {
//             queue.push(i);
//             std::cout << "Produced: " << i << std::endl;
//         }
//     });
//
//     std::thread consumer([&]() {
//         int value;
//         for (int i = 0; i < 10; ++i) {
//             while (!queue.pop(&value));
//             std::cout << "Consumed: " << value << std::endl;
//         }
//     });
//
//     producer.join();
//     consumer.join();
// }

// 为什么这个实现是无锁的？
// 无 mutex
// 无阻塞-线程不会被挂起
// CAS 隐式完成-使用原子指针
// 内存序控制-acquire/release保证可见性

// 1️⃣ 为什么只能 SPSC？
// 多生产者 → 需要 CAS 处理 tail_竞争（复杂）
// 多消费者 → 需要 CAS 处理 head_竞争
// SPSC 可以保证线程安全且简单
//
// 2️⃣ ABA 问题如何解决？
// 本例中使用 链表 + dummy node，避免 ABA
// 更复杂实现会使用 Tagged Pointer
//
// 3️⃣ 内存回收问题（难点）
// 本例直接 delete（SPSC 安全）
// 多生产者/消费者需用：
// Hazard Pointers
// RCU
// Epoch-Based Reclamation