# 在rider中编写cuda
## 先通过vs在解决方案中创建一个cuda项目
## 然后用rider打开项目后添加.cu结尾的文件
## 右键该文件选择属性->构建操作->CudaCompile

# 解析add.cu——向量加法
1️⃣ __global__

    __global__ void add(...)

表示这是一个 GPU 核函数

由 CPU 调用，GPU 执行

2️⃣ threadIdx.x

    cpp
    int i = threadIdx.x;

每个线程有一个编号

这里只有 1 个 block，N 个线程

3️⃣ <<<1, N>>>

    cpp
    add<<<1, N>>>(...);

意思是：

1 个线程块

每个块 N 个线程

4️⃣ 内存流程（必记）

CPU 内存  →  GPU 内存  →  计算  →  GPU 内存  →  CPU 内存

# 解析matmul.cu——矩阵乘法

1️⃣ 一个线程算一个 C 元素

✅ 这是 CUDA 矩阵乘法的标准思路

2️⃣ 线程索引怎么算？

    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

blockIdx:第几个 block

threadIdx:block 内第几个线程

blockDim:block 大小

3️⃣ dim3是什么？

    dim3 blockSize(16, 16);  // 256 个线程
    dim3 gridSize((N+15)/16, (M+15)/16);

二维线程网格

类似一个“线程矩阵”

4️⃣ 为什么要有边界判断？

    if (row < M && col < N)

防止线程数多于矩阵大小。


# 解析matmul_shared_memory.cu——共享内存优化矩阵乘法

❌ 朴素版本的问题

每个线程 重复读取 A、B

全局内存（Global Memory）慢

✅ Shared Memory 优化

一个 Block 内的线程共享数据

每个元素只从 Global Memory 读一次

速度通常 快 5～10 倍

❗核心思想（一句话）：把大矩阵切成小块（Tile），每块放进 Shared Memory

1️⃣ `__shared__`

    __shared__ float As[TILE][TILE];

只在 同一个 Block 内共享

比 global memory 快几十倍

2️⃣ `__syncthreads()`

    __syncthreads();

Block 内所有线程等待

防止数据还没写完就被使用

❗ 没有它会算错

3️⃣ Tile 思想

A 的一小块 × B 的一小块
→ 累加到 C

4️⃣ 线程分工

线程负责:加载数据;计算;同步

5️⃣ 限制条件

矩阵大小必须是 TILE的整数倍

真实工程里要补 padding

| 版本            | 相对速度   |
|---------------|--------|
| 朴素 CUDA       | 1×     |
| Shared Memory | 5～10×  |
| cuBLAS        | 15～20× |

# 解析matmul_any_size.cu——支持任意尺寸的 Shared Memory 矩阵乘法 ✅

✅ 1️⃣ Grid / Block 仍然按 TILE 对齐


    gridDim = ceil(N / TILE)
✅ 2️⃣ 加载时做边界检查


    if (global_idx < N)
✅ 3️⃣ 计算时也做边界检查

    if (row < N && col < N)
✅ Grid 配置（向上取整）

    (N + TILE - 1) / TILE

这是 CUDA 的 标准写法。

✅ 越界填充 0

    else
        As[...] = 0.0f;

保证计算正确，不影响结果。

✅ 最终写回检查

    if (row < N && col < N)

防止多余线程写非法内存。

# 解析matmul_double_buffer.cu——Double Buffering 矩阵乘法

1️⃣ 两个缓冲区

    __shared__ float As[2][TILE][TILE];
    __shared__ float Bs[2][TILE][TILE];

| 缓冲区 | 用途   |
|-----|------|
| cur | 正在计算 |
| nxt | 正在加载 |

2️⃣ 缓冲区交换

    cur = 1 - cur;
    nxt = 1 - nxt;
3️⃣ 同步点唯一

    __syncthreads();

✅ 只在 计算完 + 预加载完 之后同步

| 版本               | 性能     |
|------------------|--------|
| Naive CUDA       | 1×     |
| Shared Memory    | 5～10×  |
| Double Buffering | 7～15×  |
| cuBLAS           | 15～20× |


