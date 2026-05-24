// #include "ThreadSecure.h"
//
// // 静态成员定义
// std::atomic<DCLSingleton*> DCLSingleton::instance_{nullptr};
// std::mutex DCLSingleton::mutex_;
//
// // 使用 decltype 的多态测试函数
// template <typename SingletonType>
// void testSingletonThreadSafety(const char* name) {
//     std::cout << "\n========== Testing " << name << " ==========\n";
//
//     auto worker = [](int id) {
//         using InstanceType = decltype(SingletonType::instance());
//         
//         // 模拟不同线程获取单例
//         if constexpr (std::is_pointer_v<InstanceType>) {
//             SingletonType::instance()->doWork(id);
//         } else {
//             SingletonType::instance().doWork(id);
//         }
//     };
//
//     std::vector<std::thread> threads;
//     for (int i = 0; i < 5; ++i) {
//         threads.emplace_back(worker, i);
//     }
//
//     for (auto& t : threads) {
//         t.join();
//     }
// }
//
// int main() {
//     std::cout << "Testing Thread-Safe Singletons\n";
//
//     // 测试 Meyers Singleton
//     testSingletonThreadSafety<MeyersSingleton>("Meyers Singleton");
//
//     // 测试 Double-Checked Locking Singleton
//     testSingletonThreadSafety<DCLSingleton>("Double-Checked Locking Singleton");
//
//     std::cout << "\n✅ All tests completed.\n";
//     return 0;
// }

// 加入计数器验证唯一性
#include "Cpp1ThreadSecure.h"
// 静态成员初始化
std::atomic<int> MeyersSingleton::totalCalls_{0};
std::atomic<int> DCLSingleton::totalCalls_{0};
std::atomic<DCLSingleton*> DCLSingleton::instance_{nullptr};
std::mutex DCLSingleton::mutex_;
// 使用 decltype 的多态测试函数
template <typename SingletonType>
void testSingletonUniqueness(const char* name, int numThreads = 5) {
    std::cout << "\n========== Testing " << name << " ==========\n";
    
    std::atomic<int> completedThreads{0};
    
    auto worker = [&](int id) {
        using InstanceType = decltype(SingletonType::instance());
        
        if constexpr (std::is_pointer_v<InstanceType>) {
            SingletonType::instance()->incrementAndPrint(id);
        } else {
            SingletonType::instance().incrementAndPrint(id);
        }
        //if constexpr (编译期布尔表达式) { //C++17 引入的编译期条件分支语句
        // // 编译期保留
        // } else {
        // // 编译期丢弃
        // }
        
        completedThreads.fetch_add(1, std::memory_order_relaxed);
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证结果
    if constexpr (std::is_pointer_v<decltype(SingletonType::instance())>) {
        auto* instance = SingletonType::instance();
        std::cout << "\n✅ " << name << " Verification:\n";
        std::cout << "   Final counter value: " << instance->getCounter() << "\n";
        std::cout << "   Expected calls: " << numThreads << "\n";
        std::cout << "   Instance address: " << instance << "\n";
        std::cout << "   All calls processed: " 
                  << (instance->getCounter() == numThreads ? "YES" : "NO") << "\n";
    } else {
        auto& instance = SingletonType::instance();
        std::cout << "\n✅ " << name << " Verification:\n";
        std::cout << "   Final counter value: " << instance.getCounter() << "\n";
        std::cout << "   Expected calls: " << numThreads << "\n";
        std::cout << "   Instance address: " << &instance << "\n";
        std::cout << "   All calls processed: " 
                  << (instance.getCounter() == numThreads ? "YES" : "NO") << "\n";
    }
}

int main() {
    std::cout << "=== Thread-Safe Singleton Uniqueness Test ===\n";
    std::cout << "This test verifies that only ONE instance exists,\n";
    std::cout << "and all threads access the same instance.\n\n";

    // 测试 Meyers Singleton
    testSingletonUniqueness<MeyersSingleton>("Meyers Singleton", 21);

    // 测试 DCL Singleton
    testSingletonUniqueness<DCLSingleton>("Double-Checked Locking Singleton", 19);

    // 清理 DCL Singleton
    DCLSingleton::destroy();

    std::cout << "\n=== All Tests Completed ===\n";
    return 0;
}