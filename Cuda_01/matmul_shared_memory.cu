// #include <stdio.h>
// #include <stdlib.h>
//
// #define TILE 16
// #define WIDTH 64
//
// __global__ void matMulShared(float *A, float *B, float *C, int N) {
//     __shared__ float As[TILE][TILE];
//     __shared__ float Bs[TILE][TILE];
//
//     int row = blockIdx.y * TILE + threadIdx.y;
//     int col = blockIdx.x * TILE + threadIdx.x;
//
//     float sum = 0.0f;
//
//     // 按 tile 循环
//     for (int t = 0; t < N / TILE; t++) {
//         // 协作加载到 shared memory
//         As[threadIdx.y][threadIdx.x] =
//             A[row * N + t * TILE + threadIdx.x];
//
//         Bs[threadIdx.y][threadIdx.x] =
//             B[(t * TILE + threadIdx.y) * N + col];
//
//         __syncthreads();  // 必须同步！
//
//         // 计算一个 tile
//         for (int k = 0; k < TILE; k++) {
//             sum += As[threadIdx.y][k] * Bs[k][threadIdx.x];
//         }
//
//         __syncthreads();
//     }
//
//     C[row * N + col] = sum;
// }
//
// int main() {
//     const int N = WIDTH;
//     size_t size = N * N * sizeof(float);
//
//     float *h_A = (float*)malloc(size);
//     float *h_B = (float*)malloc(size);
//     float *h_C = (float*)malloc(size);
//
//     for (int i = 0; i < N * N; i++) {
//         h_A[i] = 1.0f;
//         h_B[i] = 2.0f;
//     }
//
//     float *d_A, *d_B, *d_C;
//     cudaMalloc(&d_A, size);
//     cudaMalloc(&d_B, size);
//     cudaMalloc(&d_C, size);
//
//     cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
//     cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);
//
//     dim3 block(TILE, TILE);
//     dim3 grid(N / TILE, N / TILE);
//
//     matMulShared<<<grid, block>>>(d_A, d_B, d_C, N);
//
//     cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
//
//     printf("C[0][0] = %f\n", h_C[0]);  // 应为 N * 2 = 128
//
//     cudaFree(d_A);
//     cudaFree(d_B);
//     cudaFree(d_C);
//     free(h_A);
//     free(h_B);
//     free(h_C);
//
//     return 0;
// }