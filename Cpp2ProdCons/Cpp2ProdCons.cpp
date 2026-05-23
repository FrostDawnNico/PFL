// 生产者 - 消费者模型
// 生产者-消费者模型是一种经典的并发设计模式，通过共享缓冲区解耦生产者和消费者。
// 核心要点是线程安全：必须使用互斥锁（std::mutex）保护共享资源，并使用条件变量（std::condition_variable）协调线程间的等待与唤醒。
//
// 下面是一个简化但完整的 C++ 示例（C++11 及以上），使用有界队列实现单生产者单消费者：
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class BoundedBuffer
{
private:
    std::queue<int> buffer; // 动态内存替代手动循环缓冲区
    std::mutex mtx; // 互斥锁
    std::condition_variable not_full; // 条件变量 not_full：缓冲区满时，生产者等待；消费者取走数据后唤醒。
    std::condition_variable not_empty; // not_empty：缓冲区空时，消费者等待；生产者放入数据后唤醒。
    size_t capacity; // 容量
    bool finished = false; // 增加 finished标志，防止死锁

public:
    explicit BoundedBuffer(size_t cap) : capacity(cap)
    {
        // 如果写了手动循环缓冲区 int* buffer;
        // 在这里需要 buffer = new int[capacity];
        // 以及析构时 delete[] buffer;        
    }
    // 关键字 explicit
    // ❌ 隐式转换
    // class MyInt {
    // public:
    //     MyInt(int x) { }  // 普通构造函数
    // };
    //
    // void func(MyInt m) { }
    //
    // int main() {
    //     func(10);  // ⚠️ 编译通过：int 被隐式转换为 MyInt
    // }
    // ✅ 使用 explicit
    // class MyInt {
    // public:
    //     explicit MyInt(int x) { }
    // };
    //
    // void func(MyInt m) { }
    //
    // int main() {
    //     // func(10);      // ❌ 编译错误：不能隐式转换
    //     func(MyInt(10)); // ✅ 必须显式构造
    // }
    // 常见使用场景:
    // 1️⃣ 单参数构造函数（最常见）
    // class Buffer {
    // public:
    //     explicit Buffer(size_t size) { }
    // };
    // 防止：Buffer b = 1024; // ❌ 不允许
    // 2️⃣ 智能指针 / RAII 类
    // std::unique_ptr<int> p = new int(10); // ❌ std::unique_ptr的构造函数就是 explicit。
    // std::unique_ptr<int> p(new int(10));  // ✅
    // 3️⃣ 防止 bool 被误用
    // class Status {
    // public:
    //     explicit Status(bool ok) : ok_(ok) {}
    // private:
    //     bool ok_;
    // };
    // 避免：Status s = true; // ❌ 不允许
    
    
    void produce(int item)
    {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待缓冲区非满，自动释放/获取锁：wait()会自动释放 lock，被唤醒时重新获取
        not_full.wait(lock, [this]()
        {
            return buffer.size() < capacity;
        });

        buffer.push(item);
        //std::cout << "Produced: " << item << std::endl;
        not_empty.notify_one(); // 通知消费者
        //1️⃣ 互斥锁保证串行访问2️⃣ notify_one()避免了惊群效应
    }

    int consume()
    {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待缓冲区非空或者已经停止生产，自动释放/获取锁：wait()会自动释放 lock，被唤醒时重新获取
        not_empty.wait(lock, [this]()// std::condition_variable::wait的第二个参数必须是一个可调用对象（Callable）
        {
            return !buffer.empty() || finished;
        });
        // 执行流程
        // ① 当前线程持有锁 → ② 检查条件
        //    ├─ 条件为真 → 立即继续执行（不阻塞）
        //    └─ 条件为假 → 释放锁，线程阻塞等待
        //         ↓
        // ③ 被 notify_*() 唤醒
        //    ├─ 重新获取锁
        //    ├─ 再次检查条件（防虚假唤醒）
        //    │    ├─ 条件为真 → 继续执行
        //    │    └─ 条件为假 → 再次阻塞
        //    └─ 退出 wait()
        // ❌ 错误写法（可能死锁）：
        // while (buffer.size() == capacity) {
        //     not_full.wait(lock);  // 无谓词，可能虚假唤醒
        // }
        if (buffer.empty()) return -1;

        int item = buffer.front();
        buffer.pop();
        //std::cout << "Consumed: " << item << std::endl;
        not_full.notify_one(); // 通知生产者
        return item;
    }

    void setFinished()
    {
        // 生产者结束通知所有消费者
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        std::cout << "Production Finished" << std::endl;
        not_empty.notify_all();
        not_full.notify_all();
    }
};

void producer(BoundedBuffer& buf, int id, int n)
{
    for (int i = 0; i < n; ++i)
    {
        buf.produce(i);
        std::cout << "Producer " << id << " -> " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer(BoundedBuffer& buf, int id, int n)
{
    for (int i = 0; i < n; ++i)
    {
        int item = buf.consume();
        if (item == -1)
        {
            std::cout << "Consumer " << id << " still need " << (n-i) << std::endl;
            break;
        }
        std::cout << "Consumer " << id << " <- " << item << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

int main()
{
    BoundedBuffer buffer(5); // 容量为5
    std::thread p1(producer, std::ref(buffer), 1, 6);
    std::thread p2(producer, std::ref(buffer), 2, 7);
    std::thread c1(consumer, std::ref(buffer), 1, 8);
    std::thread c2(consumer, std::ref(buffer), 2, 9);

    p1.join();
    p2.join();
    
    buffer.setFinished(); // 生产者完成通知消费者退出
    
    c1.join();
    c2.join();

    return 0;
}

// 关键点说明：
//
// 互斥锁：std::mutex确保同一时间只有一个线程访问缓冲区。
//
// 条件变量：not_full和 not_empty用于阻塞线程，直到满足条件（缓冲区有空位或有数据）。
//
// 虚假唤醒处理：wait使用谓词（lambda）检查条件，避免虚假唤醒导致错误。
//
// 通知机制：notify_one()唤醒一个等待线程，减少竞争。

// ✅ 进阶说法：
// std::queue有堆分配开销，高性能场景可用 无锁队列 或 ring buffer
//
// C++20 可用 std::binary_semaphore简化实现
//
// 可扩展为 线程池任务队列
