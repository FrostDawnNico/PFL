// #pragma once
// #include <iostream>
// #include <mutex>
// #include <atomic>
// #include <chrono>
// #include <thread>
//
// // Meyers Singleton（最推荐）
// // C++11 起 天然线程安全;简洁;无锁;防拷贝 / 防移动;实际工程中最常用
// class MeyersSingleton
// {
// public:
//     // 全局访问点
//     static MeyersSingleton& instance()
//     {
//         static MeyersSingleton instance; // C++11 起线程安全
//         return instance;
//     }
//
//     // 禁止拷贝和赋值
//     MeyersSingleton(const MeyersSingleton&) = delete;
//     MeyersSingleton& operator=(const MeyersSingleton&) = delete;
//
//     // 业务接口
//     void doWork(int threadId) {
//         std::cout << "[Meyers] Thread " << threadId
//                   << ", Instance address: " << this << std::endl;
//     }
//
// private:
//     // 私有构造
//     MeyersSingleton()
//     {
//         std::cout << "[Meyers] Constructor called! Instance address: "
//             << this << std::endl;
//         // 模拟构造耗时，增加竞态条件概率
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
//
//     // 可选：私有析构
//     ~MeyersSingleton()
//     {
//         std::cout << "[Meyers] Destructor called!" << std::endl;
//     }
// };
//
// // ✅ 为什么线程安全？
// // C++11 标准规定：
// // 函数内的 static局部变量初始化是线程安全的
// // 编译器自动加锁
//
// // 追问：
// // ❓为什么删除拷贝构造？→ 防止多个实例
// // ❓为什么返回引用而不是指针？→ 更安全、语义清晰
// // ❓析构顺序有问题吗？→ 局部静态对象在程序结束时自动析构
//
//
// //✅ 双重检查锁定(双检锁)（Double-Checked Locking）
// class DCLSingleton
// {
// public:
//     static DCLSingleton* instance()
//     {
//         // 第一次检查（无锁）
//         DCLSingleton* tmp = instance_.load(std::memory_order_acquire); // 内存序（Memory Order），确保后续读操作不会被重排到前面
//         // ❓为什么不能直接if (!instance_)或者if (instance_ == nullptr)
//         // ❌ 问题根源：指令重排
//         // instance_ = new Singleton();这一步实际拆成 3 步：分配内存；调用构造函数；把地址赋给 instance_
//         // ⚠️ 编译器和 CPU 可能重排为：1 → 3 → 2
//         // ➡️ 结果：另一个线程看到 instance_ != nullptr，但对象还没构造完！
//         // 直接返回一个尚未完全构造的对象，这是未定义行为
//         // ✅ 即使加了 mutex，也无法阻止这种重排
//         if (tmp == nullptr)
//         {
//             std::lock_guard<std::mutex> lock(mutex_);
//             // 第二次检查（有锁）
//             tmp = instance_.load(std::memory_order_relaxed);
//             if (tmp == nullptr)
//             {
//                 tmp = new DCLSingleton();
//                 instance_.store(tmp, std::memory_order_release);// 确保前面的写操作不会被重排到后面
//                 atexit(destroy); // 程序退出时调用
//             }
//         }
//         return tmp;
//     }
//
//     DCLSingleton(const DCLSingleton&) = delete;
//     DCLSingleton& operator=(const DCLSingleton&) = delete;
//
//     void doWork(int threadId) {
//         std::cout << "[DCL] Thread " << threadId
//                   << ", Instance address: " << this << std::endl;
//     }
//
//     // 清理资源（必须手动释放）
//     static void destroy() {
//         delete instance_.load();
//         instance_.store(nullptr);
//     }
// private:
//     DCLSingleton()
//     {
//         std::cout << "[DCL] Constructor called! Instance address: "
//             << this << std::endl;
//         // 模拟构造耗时
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }
//
//     ~DCLSingleton()
//     {
//         std::cout << "[DCL] Destructor called!" << std::endl;
//     }
//
//     static std::atomic<DCLSingleton*> instance_;
//     static std::mutex mutex_;
// };
//
// // ❌ 缺点
// // 容易写错，可读性差
// // 早期 C++ 存在内存可见性问题（需 atomic）
//

// 加入计数器验证唯一性
#pragma once
#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

// ================== Meyers Singleton（推荐）==================
class MeyersSingleton {
public:
    static MeyersSingleton& instance() {
        static MeyersSingleton instance;
        return instance;
    }

    void incrementAndPrint(int threadId) {
        int oldCount = counter_.fetch_add(1, std::memory_order_relaxed);
        std::cout << "[Meyers] Thread " << threadId 
                  << ", Instance: " << this 
                  << ", Counter: " << oldCount + 1 
                  << ", Expected: " << totalCalls_.fetch_add(1, std::memory_order_relaxed) + 1 
                  << std::endl;
    }

    int getCounter() const {
        return counter_.load(std::memory_order_relaxed);
    }

    ~MeyersSingleton() {
        std::cout << "MeyersSingleton destroyed. Final counter: " 
                  << counter_.load() << std::endl;
    }

    MeyersSingleton(const MeyersSingleton&) = delete;
    MeyersSingleton& operator=(const MeyersSingleton&) = delete;

private:
    MeyersSingleton() : counter_(0) {
        std::cout << "MeyersSingleton constructed at " << this << std::endl;
    }

    std::atomic<int> counter_;
    static std::atomic<int> totalCalls_;
};

// ================== DCL Singleton（使用原子操作）==================
class DCLSingleton {
public:
    static DCLSingleton* instance() {
        DCLSingleton* tmp = instance_.load(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            tmp = instance_.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                tmp = new DCLSingleton();
                instance_.store(tmp, std::memory_order_release);
            }
        }
        return tmp;
    }

    void incrementAndPrint(int threadId) {
        int oldCount = counter_.fetch_add(1, std::memory_order_relaxed);
        std::cout << "[DCL] Thread " << threadId 
                  << ", Instance: " << this 
                  << ", Counter: " << oldCount + 1 
                  << ", Expected: " << totalCalls_.fetch_add(1, std::memory_order_relaxed) + 1 
                  << std::endl;
    }

    int getCounter() const {
        return counter_.load(std::memory_order_relaxed);
    }

    static void destroy() {
        DCLSingleton* ptr = instance_.load();
        if (ptr) {
            std::cout << "DCLSingleton destroyed. Final counter: " 
                      << ptr->counter_.load() << std::endl;
            delete ptr;
            instance_.store(nullptr);
        }
    }

    DCLSingleton(const DCLSingleton&) = delete;
    DCLSingleton& operator=(const DCLSingleton&) = delete;

private:
    DCLSingleton() : counter_(0) {
        std::cout << "DCLSingleton constructed at " << this << std::endl;
    }

    std::atomic<int> counter_;
    static std::atomic<int> totalCalls_;
    static std::atomic<DCLSingleton*> instance_;
    static std::mutex mutex_;
};

