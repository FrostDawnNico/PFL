// #include <stdio.h>
//
// // CUDA 核函数（在 GPU 上运行）
// __global__ void add(int *a, int *b, int *c) {
//     int i = threadIdx.x;
//     c[i] = a[i] + b[i];
// }
//
// int main() {
//     const int N = 5;
//     int a[N] = {1,2,3,4,5};
//     int b[N] = {10,20,30,40,50};
//     int c[N];
//
//     int *d_a, *d_b, *d_c;
//
//     // 1. 在 GPU 上分配内存
//     cudaMalloc(&d_a, N * sizeof(int));
//     cudaMalloc(&d_b, N * sizeof(int));
//     cudaMalloc(&d_c, N * sizeof(int));
//
//     // 2. 把数据从 CPU 拷贝到 GPU
//     cudaMemcpy(d_a, a, N * sizeof(int), cudaMemcpyHostToDevice);
//     cudaMemcpy(d_b, b, N * sizeof(int), cudaMemcpyHostToDevice);
//
//     // 3. 启动 GPU 核函数
//     add<<<1, N>>>(d_a, d_b, d_c);
//
//     // 4. 把结果从 GPU 拷回 CPU
//     cudaMemcpy(c, d_c, N * sizeof(int), cudaMemcpyDeviceToHost);
//
//     // 5. 打印结果
//     for (int i = 0; i < N; i++) {
//         printf("%d + %d = %d\n", a[i], b[i], c[i]);
//     }
//
//     // 6. 释放 GPU 内存
//     cudaFree(d_a);
//     cudaFree(d_b);
//     cudaFree(d_c);
//
//     return 0;
// }