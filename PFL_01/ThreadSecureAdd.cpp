// // 线程安全单例模式+加入计数器验证唯一性+线程安全输出
// #include <iostream>
// #include <mutex>
// #include <atomic>
// #include <thread>
// #include <vector>
// #include <sstream>
//
// // 全局输出互斥锁
// std::mutex g_outputMutex;
//
// // 线程安全的输出函数
// void threadSafePrint(const std::string& msg) {
//     std::lock_guard<std::mutex> lock(g_outputMutex);
//     std::cout << msg << std::endl;
// }
//
// // ================== Meyers Singleton ==================
// class MeyersSingleton {
// public:
//     static MeyersSingleton& instance() {
//         static MeyersSingleton instance;
//         return instance;
//     }
//
//     void incrementAndPrint(int threadId) {
//         int oldCount = counter_.fetch_add(1, std::memory_order_relaxed);
//         int currentCount = oldCount + 1;
//         
//         std::ostringstream oss;
//         oss << "[Meyers] Thread " << threadId 
//             << ", Instance: " << this 
//             << ", Counter: " << currentCount
//             << ", Total calls: " << totalCalls_.fetch_add(1, std::memory_order_relaxed) + 1;
//         
//         threadSafePrint(oss.str());
//     }
//
//     int getCounter() const {
//         return counter_.load(std::memory_order_relaxed);
//     }
//
//     ~MeyersSingleton() {
//         std::ostringstream oss;
//         oss << "MeyersSingleton destroyed. Final counter: " << counter_.load();
//         threadSafePrint(oss.str());
//     }
//
//     MeyersSingleton(const MeyersSingleton&) = delete;
//     MeyersSingleton& operator=(const MeyersSingleton&) = delete;
//
// private:
//     MeyersSingleton() : counter_(0) {
//         std::ostringstream oss;
//         oss << "MeyersSingleton constructed at " << this;
//         threadSafePrint(oss.str());
//     }
//
//     std::atomic<int> counter_;
//     static std::atomic<int> totalCalls_;
// };
//
// // ================== DCL Singleton ==================
// class DCLSingleton {
// public:
//     static DCLSingleton* instance() {
//         DCLSingleton* tmp = instance_.load(std::memory_order_acquire);
//         if (tmp == nullptr) {
//             std::lock_guard<std::mutex> lock(mutex_);
//             tmp = instance_.load(std::memory_order_relaxed);
//             if (tmp == nullptr) {
//                 tmp = new DCLSingleton();
//                 instance_.store(tmp, std::memory_order_release);
//             }
//         }
//         return tmp;
//     }
//
//     void incrementAndPrint(int threadId) {
//         int oldCount = counter_.fetch_add(1, std::memory_order_relaxed);
//         int currentCount = oldCount + 1;
//         
//         std::ostringstream oss;
//         oss << "[DCL] Thread " << threadId 
//             << ", Instance: " << this 
//             << ", Counter: " << currentCount
//             << ", Total calls: " << totalCalls_.fetch_add(1, std::memory_order_relaxed) + 1;
//         
//         threadSafePrint(oss.str());
//     }
//
//     int getCounter() const {
//         return counter_.load(std::memory_order_relaxed);
//     }
//
//     static void destroy() {
//         DCLSingleton* ptr = instance_.load();
//         if (ptr) {
//             std::ostringstream oss;
//             oss << "DCLSingleton destroyed. Final counter: " << ptr->counter_.load();
//             threadSafePrint(oss.str());
//             delete ptr;
//             instance_.store(nullptr);
//         }
//     }
//
//     DCLSingleton(const DCLSingleton&) = delete;
//     DCLSingleton& operator=(const DCLSingleton&) = delete;
//
// private:
//     DCLSingleton() : counter_(0) {
//         std::ostringstream oss;
//         oss << "DCLSingleton constructed at " << this;
//         threadSafePrint(oss.str());
//     }
//
//     std::atomic<int> counter_;
//     static std::atomic<int> totalCalls_;
//     static std::atomic<DCLSingleton*> instance_;
//     static std::mutex mutex_;
// };
//
// // 静态成员初始化
// std::atomic<int> MeyersSingleton::totalCalls_{0};
// std::atomic<int> DCLSingleton::totalCalls_{0};
// std::atomic<DCLSingleton*> DCLSingleton::instance_{nullptr};
// std::mutex DCLSingleton::mutex_;
//
// // 使用 decltype 的多态测试函数
// template <typename SingletonType>
// void testSingletonUniqueness(const char* name, int numThreads = 5) {
//     std::ostringstream header;
//     header << "\n========================================";
//     threadSafePrint(header.str());
//     
//     std::ostringstream title;
//     title << "Testing " << name;
//     threadSafePrint(title.str());
//     
//     std::ostringstream footer;
//     footer << "========================================";
//     threadSafePrint(footer.str());
//     
//     std::atomic<int> completedThreads{0};
//     std::vector<std::thread> threads;
//     
//     auto worker = [&](int id) {
//         using InstanceType = decltype(SingletonType::instance());
//         
//         if constexpr (std::is_pointer_v<InstanceType>) {
//             SingletonType::instance()->incrementAndPrint(id);
//         } else {
//             SingletonType::instance().incrementAndPrint(id);
//         }
//         
//         completedThreads.fetch_add(1, std::memory_order_relaxed);
//     };
//
//     // 创建并启动线程
//     for (int i = 0; i < numThreads; ++i) {
//         threads.emplace_back(worker, i);
//     }
//
//     // 等待所有线程完成
//     for (auto& t : threads) {
//         t.join();
//     }
//
//     // 验证结果
//     if constexpr (std::is_pointer_v<decltype(SingletonType::instance())>) {
//         auto* instance = SingletonType::instance();
//         std::ostringstream oss;
//         oss << "\n✅ " << name << " Verification:";
//         oss << "\n   Final counter value: " << instance->getCounter();
//         oss << "\n   Expected calls: " << numThreads;
//         oss << "\n   Instance address: " << instance;
//         oss << "\n   All calls processed: " 
//             << (instance->getCounter() == numThreads ? "YES" : "NO");
//         threadSafePrint(oss.str());
//     } else {
//         auto& instance = SingletonType::instance();
//         std::ostringstream oss;
//         oss << "\n✅ " << name << " Verification:";
//         oss << "\n   Final counter value: " << instance.getCounter();
//         oss << "\n   Expected calls: " << numThreads;
//         oss << "\n   Instance address: " << &instance;
//         oss << "\n   All calls processed: " 
//             << (instance.getCounter() == numThreads ? "YES" : "NO");
//         threadSafePrint(oss.str());
//     }
// }
//
// int main() {
//     std::ostringstream start;
//     start << "=== Thread-Safe Singleton Uniqueness Test ===";
//     threadSafePrint(start.str());
//     
//     std::ostringstream desc;
//     desc << "This test verifies that only ONE instance exists,";
//     threadSafePrint(desc.str());
//     
//     std::ostringstream desc2;
//     desc2 << "and all threads access the same instance.\n";
//     threadSafePrint(desc2.str());
//
//     // 测试 Meyers Singleton
//     testSingletonUniqueness<MeyersSingleton>("Meyers Singleton", 5);
//
//     // 测试 DCL Singleton
//     testSingletonUniqueness<DCLSingleton>("Double-Checked Locking Singleton", 5);
//
//     // 清理 DCL Singleton
//     DCLSingleton::destroy();
//
//     std::ostringstream end;
//     end << "\n=== All Tests Completed ===";
//     threadSafePrint(end.str());
//     
//     return 0;
// }