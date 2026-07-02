#include <iostream>
#include <string>

// sizeof 和 strlen 区别
int main()
{
    int a;
    printf("%zu\n", sizeof(a));   // 输出 int 占用的字节数（通常 4）
    //编译器在编译阶段就知道类型大小;与数据内容无关
    char str[]= "Hello World!";
    std::cout << str << std::endl;
    std::cout << sizeof(str) << std::endl;//包含 '\0'
    std::cout << strlen(str) << std::endl;//从首地址开始;一直数到第一个 '\0';不把 '\0'计入长度
    return 0;
}