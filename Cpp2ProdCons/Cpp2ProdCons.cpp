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

class BoundedBuffer {
private:
    std::queue<int> buffer;
    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    size_t capacity;

public:
    explicit BoundedBuffer(size_t cap) : capacity(cap) {}

    void produce(int item) {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待缓冲区非满
        not_full.wait(lock, [this]() { return buffer.size() < capacity; });
        buffer.push(item);
        std::cout << "Produced: " << item << std::endl;
        not_empty.notify_one(); // 通知消费者
    }

    int consume() {
        std::unique_lock<std::mutex> lock(mtx);
        // 等待缓冲区非空
        not_empty.wait(lock, [this]() { return !buffer.empty(); });
        int item = buffer.front();
        buffer.pop();
        std::cout << "Consumed: " << item << std::endl;
        not_full.notify_one(); // 通知生产者
        return item;
    }
};

void producer(BoundedBuffer& buf, int n) {
    for (int i = 0; i < n; ++i) {
        buf.produce(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer(BoundedBuffer& buf, int n) {
    for (int i = 0; i < n; ++i) {
        buf.consume();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

int main() {
    BoundedBuffer buffer(5); // 容量为5
    std::thread p(producer, std::ref(buffer), 10);
    std::thread c(consumer, std::ref(buffer), 10);

    p.join();
    c.join();

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