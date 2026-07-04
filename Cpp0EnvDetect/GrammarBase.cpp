#include <iostream>
#include <string>
using namespace std;

// sizeof 和 strlen 区别
// int main()
// {
//     int a;
//     printf("%zu\n", sizeof(a));   // 输出 int 占用的字节数（通常 4）
//     //编译器在编译阶段就知道类型大小;与数据内容无关
//     char str[]= "Hello World!";
//     std::cout << str << std::endl;
//     std::cout << sizeof(str) << std::endl;//包含 '\0'
//     std::cout << strlen(str) << std::endl;//从首地址开始;一直数到第一个 '\0';不把 '\0'计入长度
//     return 0;
// }

// codex app每次打开重连5次Reconnecting问题解决
// 原因:
// 默认是使用websocket协议，在websocket重连等待五次（并且每次的超时时间足足有20s）之后才会切换 到可以正常通信的HTTP协议，至于websocket协议为什么不通，可能是代理不支持websocket协议.
//
// 方案1：
// 在.codex目录（windows对应目录C:\Users\Administrator\.codex）新建一个.env文件内容为：
//
// HTTP_PROXY="http://127.0.0.1:10809"
// HTTPS_PROXY="http://127.0.0.1:10809"
// ALL_PROXY="socks5://127.0.0.1:10808"
// 运行项目并下载源码
// bash
// ps：端口对应代理的端口。改完记得重启codex。
// ————————————————
// 版权声明：本文为CSDN博主「带你去学习」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
// 原文链接：https://blog.csdn.net/zhou870498/article/details/160312916


// 数组做参数退化为指针

// 函数接收“数组参数”
// void printSize(int arr[])
// {
//     cout << "函数内 sizeof(arr) = " << sizeof(arr) << endl; //指针大小
//     cout << "函数内 arr[0] = " << arr[0] << endl;
// }
//
// int main()
// {
//     int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//
//     cout << "main 中 sizeof(a) = " << sizeof(a) << endl; // 10*4字节
//     printSize(a);
//
//     return 0;
// }

// UTF-16的字节序差异、BOM的作用
// #include <cstdint>
// #include <iomanip>
//
// int main() {
//     // 汉字「中」的Unicode码点是 U+4E2D，UTF-16编码就是0x4E2D
//     const wchar_t zhong = L'中';
//
//     // 把wchar_t按字节拆分，看内存里的实际存储顺序
//     const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&zhong);
//
//     cout << "UTF-16 核心规则演示：" << endl;
//     cout << "1. UTF-16 LE BOM（小端序标记）：0xFF 0xFE" << endl;
//     cout << "2. UTF-16 BE BOM（大端序标记）：0xFE 0xFF" << endl;
//     cout << "3. 「中」的Unicode码点：U+4E2D" << endl;
//     
//     cout << "4. 当前Windows系统wchar_t（UTF-16）的字节序（默认小端LE）：";
//     cout << hex << setfill('0'); // 十六进制输出，补0
//     for (size_t i = 0; i < sizeof(wchar_t); ++i) {
//         cout << "0x" << setw(2) << static_cast<int>(bytes[i]) << " ";
//     }
//     cout << dec << endl;
//     cout << "   ✅ 验证：低字节0x2D在前，高字节0x4E在后，符合小端序" << endl;
//
//     return 0;
// }

