// #include <iostream>
// #include <fstream>
// #include <cstdint>
// #include <iomanip>
// #include <string>
// #include <locale>
//
// using namespace std;
//
// // 辅助函数：打印字节为十六进制
// void printHex(const uint8_t* data, size_t len) {
//     cout << hex << setfill('0');
//     for (size_t i = 0; i < len; ++i) {
//         cout << "0x" << setw(2) << static_cast<int>(data[i]) << " ";
//     }
//     cout << dec << endl;
// }
//
// int main() {
//     // ==================== 1. 生成UTF-16 LE文件 ====================
//     {
//         ofstream leFile("utf16_le.txt", ios::binary); // 必须用binary模式，否则Windows会自动改换行符
//         // 写入UTF-16 LE BOM：0xFF 0xFE
//         const uint8_t leBom[] = {0xFF, 0xFE};
//         leFile.write(reinterpret_cast<const char*>(leBom), sizeof(leBom));
//         // 写入「中」（U+4E2D → LE: 0x2D 0x4E）
//         const uint8_t leZhong[] = {0x2D, 0x4E};
//         leFile.write(reinterpret_cast<const char*>(leZhong), sizeof(leZhong));
//         // 写入「文」（U+6587 → LE: 0x87 0x65）
//         const uint8_t leWen[] = {0x87, 0x65};
//         leFile.write(reinterpret_cast<const char*>(leWen), sizeof(leWen));
//     }
//
//     // ==================== 2. 生成UTF-16 BE文件 ====================
//     {
//         ofstream beFile("utf16_be.txt", ios::binary);
//         // 写入UTF-16 BE BOM：0xFE 0xFF
//         const uint8_t beBom[] = {0xFE, 0xFF};
//         beFile.write(reinterpret_cast<const char*>(beBom), sizeof(beBom));
//         // 写入「中」（U+4E2D → BE: 0x4E 0x2D）
//         const uint8_t beZhong[] = {0x4E, 0x2D};
//         beFile.write(reinterpret_cast<const char*>(beZhong), sizeof(beZhong));
//         // 写入「文」（U+6587 → BE: 0x65 0x87）
//         const uint8_t beWen[] = {0x65, 0x87};
//         beFile.write(reinterpret_cast<const char*>(beWen), sizeof(beWen));
//     }
//
//     // ==================== 3. 验证文件二进制内容 ====================
//     cout << "生成的文件原始字节：" << endl;
//     
//     uint8_t leBuf[6], beBuf[6];
//     ifstream("utf16_le.txt", ios::binary).read(reinterpret_cast<char*>(leBuf), 6);
//     ifstream("utf16_be.txt", ios::binary).read(reinterpret_cast<char*>(beBuf), 6);
//     
//     cout << "utf16_le.txt 前6字节："; printHex(leBuf, 6); // 0xff 0xfe 0x2d 0x4e 0x87 0x65
//     cout << "utf16_be.txt 前6字节："; printHex(beBuf, 6); // 0xfe 0xff 0x4e 0x2d 0x65 0x87
//
//     // ==================== 4. 正确读取：靠BOM识别字节序 ====================
//     cout << "\n✅ 正确读取结果（程序靠BOM自动识别字节序）：" << endl;
//     
//     wifstream leIn("utf16_le.txt", ios::binary);
//     leIn.imbue(locale("")); // Windows下会自动识别BOM，按LE解析
//     wstring leContent; getline(leIn, leContent);
//     wcout << L"UTF-16 LE文件内容：" << leContent << endl; // 正常显示「中文」
//     
//     wifstream beIn("utf16_be.txt", ios::binary);
//     beIn.imbue(locale(""));
//     wstring beContent; getline(beIn, beContent);
//     wcout << L"UTF-16 BE文件内容：" << beContent << endl; // 正常显示「中文」
//
//     // ==================== 5. 错误读取：字节序不匹配 ====================
//     cout << "\n❌ 错误读取（把UTF-16 LE当成BE解析，模拟没有BOM时程序猜错的情况）：" << endl;
//     // LE的「中」是0x2D4E，当成BE解析的话，码点变成U+2D4E（完全不同的字符）
//     uint16_t wrongCode = (static_cast<uint16_t>(leBuf[2]) << 8) | leBuf[3];
//     wcout << L"把LE的「中」强行按BE解析后的码点：U+" << hex << wrongCode << dec << endl;
//     // 这个码点对应的是「ⵎ」（提非纳字母），或者直接显示为乱码/问号
//
//     return 0;
// }
//
// // 验证小实验（Rider专属）
// // 
// // 用Rider打开生成的utf16_le.txt，右下角编码默认是UTF-16 LE，显示正常「中文」
// //
// // 点击右下角编码，切换为UTF-16 BE，你会立刻看到乱码——这就是字节序不匹配的后果
// // 
// // 反过来打开utf16_be.txt，切换为UTF-16 LE也会乱码
//
// // 重要补充
// //
// // 这个示例仅适用于Windows：Linux的wchar_t是4字节（对应UTF-32），没有字节序问题，跑这个示例会出错
// //
// // 日常开发里UTF-16用得很少，主要是Windows API、老系统、部分文件格式（PDF/Office）在用；跨平台开发几乎全用UTF-8
// //
// // UTF-8的BOM是可选的（因为UTF-8没有字节序问题），但UTF-16的BOM是强推荐的——没有BOM的话，程序根本不知道该按LE还是BE解析，100%会猜错