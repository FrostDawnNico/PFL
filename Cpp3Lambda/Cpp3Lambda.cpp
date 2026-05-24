#include <iostream>
#include <ostream>
// 函数模板（Function Template）
// 1️⃣ 基本写法
template <typename T>
T add(T a, T b)
{
    return a + b;
}

// 使用
int main()
{
    int x = add(1, 2);
    double y = add(3.5, 4.5);
    std::cout << x << " " << y << std::endl;
}

// ✅ 考点：
// 编译期推导类型
// 不支持隐式类型转换

// 2️⃣ 显示指定模板参数
template<typename T>
T max(T a, T b) { return a > b ? a : b; }

auto r = max<double>(3, 4.5);

// 类模板（Class Template）
// 1️⃣ 基本类模板
template<typename T>
class Box {
public:
    Box(T v) : val(v) {}
    T get() { return val; }
private:
    T val;
};

Box<int> b(10);

// 2️⃣ 类模板的成员函数类外定义
template<typename T>
class Box1 {
public:
    T get();
};

template<typename T>
T Box1<T>::get() {
    return T();
}

// 模板特化（Template Specialization）:某些类型需要特殊实现
// 1️⃣ 全特化（Full Specialization）
template<typename T>
struct TypeSize {
    static const int value = sizeof(T);
};

// 全特化
template<>
struct TypeSize<int> {
    static const int value = 4;
};

//📌 特点：所有模板参数都指定；相当于“重写”

// 2️⃣ 偏特化（Partial Specialization）
// （1）指针类型的偏特化
















