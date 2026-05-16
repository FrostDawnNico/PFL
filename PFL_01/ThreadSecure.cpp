#include "ThreadSecure.h"

// 静态成员定义
std::atomic<DCLSingleton*> DCLSingleton::instance_{nullptr};
std::mutex DCLSingleton::mutex_;

// 使用 decltype 的多态测试函数
template <typename SingletonType>
void testSingletonThreadSafety(const char* name) {
    std::cout << "\n========== Testing " << name << " ==========\n";

    auto worker = [](int id) {
        using InstanceType = decltype(SingletonType::instance());
        
        // 模拟不同线程获取单例
        if constexpr (std::is_pointer_v<InstanceType>) {
            SingletonType::instance()->doWork(id);
        } else {
            SingletonType::instance().doWork(id);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    std::cout << "Testing Thread-Safe Singletons\n";

    // 测试 Meyers Singleton
    testSingletonThreadSafety<MeyersSingleton>("Meyers Singleton");

    // 测试 Double-Checked Locking Singleton
    testSingletonThreadSafety<DCLSingleton>("Double-Checked Locking Singleton");

    std::cout << "\n✅ All tests completed.\n";
    return 0;
}