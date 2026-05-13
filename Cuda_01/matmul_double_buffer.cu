#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TILE 16

__global__ void matMulDoubleBuffer(
    float *A, float *B, float *C, int N) {

    __shared__ float As[2][TILE][TILE];
    __shared__ float Bs[2][TILE][TILE];

    int row = blockIdx.y * TILE + threadIdx.y;
    int col = blockIdx.x * TILE + threadIdx.x;

    float sum = 0.0f;
    int cur = 0, nxt = 1;

    // 预加载第一块
    int t = 0;
    int aCol = t * TILE + threadIdx.x;
    int bRow = t * TILE + threadIdx.y;

    if (row < N && aCol < N)
        As[cur][threadIdx.y][threadIdx.x] = A[row * N + aCol];
    else
        As[cur][threadIdx.y][threadIdx.x] = 0.0f;

    if (bRow < N && col < N)
        Bs[cur][threadIdx.y][threadIdx.x] = B[bRow * N + col];
    else
        Bs[cur][threadIdx.y][threadIdx.x] = 0.0f;

    __syncthreads();

    int numTiles = (N + TILE - 1) / TILE;

    for (t = 0; t < numTiles; t++) {
        // 如果不是最后一块，预加载下一块
        if (t < numTiles - 1) {
            int next_aCol = (t + 1) * TILE + threadIdx.x;
            int next_bRow = (t + 1) * TILE + threadIdx.y;

            if (row < N && next_aCol < N)
                As[nxt][threadIdx.y][threadIdx.x] = A[row * N + next_aCol];
            else
                As[nxt][threadIdx.y][threadIdx.x] = 0.0f;

            if (next_bRow < N && col < N)
                Bs[nxt][threadIdx.y][threadIdx.x] = B[next_bRow * N + col];
            else
                Bs[nxt][threadIdx.y][threadIdx.x] = 0.0f;
        }

        // 计算当前 tile
        for (int k = 0; k < TILE; k++) {
            sum += As[cur][threadIdx.y][k] * Bs[cur][k][threadIdx.x];
        }

        __syncthreads();  // 确保预加载完成，才能交换

        // 交换缓冲区
        cur = 1 - cur;
        nxt = 1 - nxt;
    }

    if (row < N && col < N)
        C[row * N + col] = sum;
}

int main() {
    const int N = 1024;
    size_t size = N * N * sizeof(float);

    float *h_A = (float*)malloc(size);
    float *h_B = (float*)malloc(size);
    float *h_C = (float*)malloc(size);

    for (int i = 0; i < N * N; i++) {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
    }

    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size);
    cudaMalloc(&d_B, size);
    cudaMalloc(&d_C, size);

    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

    dim3 block(TILE, TILE);
    dim3 grid((N + TILE - 1) / TILE, (N + TILE - 1) / TILE);

    matMulDoubleBuffer<<<grid, block>>>(d_A, d_B, d_C, N);

    cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);

    printf("C[0][0] = %f\n", h_C[0]);           // 2048
    printf("C[%d][%d] = %f\n", N-1, N-1, h_C[N*N-1]);

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    free(h_A);
    free(h_B);
    free(h_C);

    return 0;
}