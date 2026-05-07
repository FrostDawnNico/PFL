// Hello World! of CPP
#include <iostream>
#include <ostream>
//
// int main(int argc, char* argv[])
// {
//     std::cout << "Hello World!" << std::endl;
//     return 0;
// }

// C++常见的面试题类型

//查看当前c++版本
// #include <iostream>
// #include <conio.h>  // 用于Windows下的_getch()
//
// int main() {
//     
//     //std::cout << "C++ version: " << __cplusplus << std::endl; 
//     //输出的__cplusplus宏 总是199711。
//     //Visual Studio官方说明 链接：https://learn.microsoft.com/zh-cn/cpp/build/reference/zc-cplusplus?view=msvc-170&viewFallbackFrom=vs-2022
//     //历史兼容性的原因，Visual C++ (MSVC) 编译器不管选择了哪个 C++ 语言标准（如 C++14, C++17），__cplusplus 宏默认都只报告 199711L（代表 C++98/03），_MSVC_LANG 这个是MSVC特有的宏，可以正确输出C++版本
//
//     //方法一 恢复__cplusplus宏
//     //在 Visual Studio 中设置此编译器选项，打开项目的“属性页” 对话框 。 有关详细信息，请参阅在 Visual Studio 中设置 C++ 编译器和生成属性。
//     //选择“配置属性”>“C/C++”>“命令行 ”属性页。将 /Zc:__cplusplus 或 /Zc:__cplusplus- 添加到“附加选项:”窗格中。
//     
//     //方法二 使用_MSVC_LANG宏
//     std::cout << "C++ version:"<< _MSVC_LANG << std::endl;
//     // 输入回车退出
//     // getchar();
//     
//     std::cout << "Press any key to exit..." << std::endl;
//     _getch();  // Windows下暂停程序直到按键被按下
//     
//     return 0;
// }

// 判断当前机器字节序（高频）
//✅ 更安全的 C++20 写法：// 目前测试17不行
#include <bit> // 经测试20里面必须有，23里面可以省略

int main()
{
    bool b = std::endian::native == std::endian::little; 
    std::cout<<b; // 1为小端序，0为大端序
}
