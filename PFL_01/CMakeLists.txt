cmake_minimum_required(VERSION 3.1)
project(PFL_01)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 打印 C++ 标准和编译器标志
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Standard Required: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Flags: ${CMAKE_CXX_FLAGS}")

# 检查是否使用 MSVC 编译器
if (MSVC)
    # 使用 /std:c++17 代替 -std=c++17
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
    # 对于其他编译器，使用 -std=c++17
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

# 添加可执行文件或其他目标
add_executable(PFL_01 PFL_01.cpp)