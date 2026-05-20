// #include <stdio.h>
// #include <stdlib.h>
//
// #define WIDTH 16
//
// // CUDA 核函数
// __global__ void matMul(float *A, float *B, float *C,
//                        int width) {
//     int row = blockIdx.y * blockDim.y + threadIdx.y;
//     int col = blockIdx.x * blockDim.x + threadIdx.x;
//
//     if (row < width && col < width) {
//         float sum = 0.0f;
//         for (int k = 0; k < width; k++) {
//             sum += A[row * width + k] * B[k * width + col];
//         }
//         C[row * width + col] = sum;
//     }
// }
//
// int main() {
//     const int width = WIDTH;
//     size_t size = width * width * sizeof(float);
//
//     float *h_A = (float*)malloc(size);
//     float *h_B = (float*)malloc(size);
//     float *h_C = (float*)malloc(size);
//
//     // 初始化
//     for (int i = 0; i < width * width; i++) {
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
//     dim3 block(16, 16);
//     dim3 grid((width + 15)/16, (width + 15)/16);
//
//     matMul<<<grid, block>>>(d_A, d_B, d_C, width);
//
//     cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
//
//     printf("C[0][0] = %f\n", h_C[0]);  // 32.0
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