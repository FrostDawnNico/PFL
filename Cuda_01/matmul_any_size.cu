// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
//
// #define TILE 16
//
// __global__ void matMulSharedAny(
//     float *A, float *B, float *C,
//     int N) {
//
//     __shared__ float As[TILE][TILE];
//     __shared__ float Bs[TILE][TILE];
//
//     int row = blockIdx.y * TILE + threadIdx.y;
//     int col = blockIdx.x * TILE + threadIdx.x;
//
//     float sum = 0.0f;
//
//     for (int t = 0; t < (N + TILE - 1) / TILE; t++) {
//
//         // ---- 加载 A ----
//         int aCol = t * TILE + threadIdx.x;
//         if (row < N && aCol < N)
//             As[threadIdx.y][threadIdx.x] = A[row * N + aCol];
//         else
//             As[threadIdx.y][threadIdx.x] = 0.0f;
//
//         // ---- 加载 B ----
//         int bRow = t * TILE + threadIdx.y;
//         if (bRow < N && col < N)
//             Bs[threadIdx.y][threadIdx.x] = B[bRow * N + col];
//         else
//             Bs[threadIdx.y][threadIdx.x] = 0.0f;
//
//         __syncthreads();
//
//         // ---- 计算 ----
//         for (int k = 0; k < TILE; k++) {
//             sum += As[threadIdx.y][k] * Bs[k][threadIdx.x];
//         }
//
//         __syncthreads();
//     }
//
//     if (row < N && col < N)
//         C[row * N + col] = sum;
// }
//
// int main() {
//     const int N = 37;  // ❗ 非 TILE 倍数
//     size_t size = N * N * sizeof(float);
//
//     float *h_A = (float*)malloc(size);
//     float *h_B = (float*)malloc(size);
//     float *h_C = (float*)malloc(size);
//
//     // for (int i = 0; i < N * N; i++) {
//     //     h_A[i] = 1.0f;
//     //     h_B[i] = 2.0f;
//     // }
//     for (int i = 0; i < N * N; i++) {
//         h_A[i] = (float)(i % 7);
//         h_B[i] = 1.0f;
//     } // 将初始化复杂化，希望计算结果的两个位置数值不同
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
//     dim3 grid(
//         (N + TILE - 1) / TILE,
//         (N + TILE - 1) / TILE
//     );
//
//     matMulSharedAny<<<grid, block>>>(d_A, d_B, d_C, N);
//
//     cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
//
//     printf("C[0][0] = %f\n", h_C[0]);        // 应为 N*2
//     printf("C[%d][%d] = %f\n", N-1, N-1, h_C[N*N-1]);
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