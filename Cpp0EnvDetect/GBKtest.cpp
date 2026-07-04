// #include <iostream>
// #include <cstdio>
//
// int main()
// {
//     const char* s = "中文";
//     std::cout << s << std::endl;
//     std::cout << sizeof("中文") << std::endl;
//     std::cout << "十六进制字节";
//     for (int i = 0; s[i]; ++i)
//         printf("%02X ", (unsigned char)s[i]);
//     std::cout << std::endl;
//     return 0;
// }

// JetBrains Rider UTF-8控制台运行中文乱码问题解决
// 1.软件部分
// 软件未汉化：
// Ctrl + ALT + S 打开设置。或者点击顶部菜单栏中的 File → Settings…
// 找到 Editor → General → Console
// 将 Default Encoding 改为 UTF-8
// 找到 Editor → File Encodings
// 将 Global Encoding、Project Encoding、Default encoding for properties files 全部改为 UTF-8
// 将代码文件的编码改为UTF-8。（编辑器右下角找到文件字符集的按钮，此时你文件是什么字符集，该按钮就显示的什么文字）
// 软件已汉化：
// Ctrl + ALT + S 打开设置。或者点击顶部菜单栏中的 文件 → 设置
// 找到 编辑器 → 常规 → 控制台
// 将 默认编码 改为 UTF-8
// 找到 编辑器 → 文件编码
// 将 全局编码、项目编码、属性文件的默认编码 全部改为 UTF-8
// 将代码文件的编码改为UTF-8。（编辑器右下角找到文件字符集的按钮，此时你文件是什么字符集，该按钮就显示的什么文字）
// 2.系统部分
// 因为Windows默认的字符集是GBK或者GB2312（可能不同国家/地区会不同），所以控制台对UTF-8编码的中文就会产生乱码。
//
// Win10/Win11解决方案：
//
// 打开控制面板
// 点击时钟和区域
// 点击区域
// 切换至管理选项卡
// 点击更改系统区域设置(C)…按钮
// 勾选上Beta 版：使用 Unicode UTF-8 提供全球语言支持(U)
// 点击确定按钮。
// 重启电脑。
// 其他系统：
//
// 去网上找更改对应系统字符集的方法，给系统默认字符集改掉就行了。
// 实在不行就Rider不用UTF-8的字符集。
// ————————————————
// 版权声明：本文为CSDN博主「阴雨霉湿」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
// 原文链接：https://blog.csdn.net/qq_38652288/article/details/139121945