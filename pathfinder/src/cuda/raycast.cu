#include <iostream>
#include <cuda.h>
#include <cuda_runtime.h>
#include "../raycast.h"
#include "PathfindRequest.h"


#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
    if (code != cudaSuccess)
    {
        fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}

__device__ int listIdx;

__global__ void shadowcast(bool *req, int lenX, int lenY, int lenZ,
                           int offX, int offY, int offZ,
                           int resLenx, int resLeny, int resLenz, short targetX, short targetY, short targetZ, int rad,
                           Coordinate* arr) {
    short locX = blockIdx.x * blockDim.x + threadIdx.x + offX;
    short locY = blockIdx.y * blockDim.y + threadIdx.y + offY;
    short locZ = blockIdx.z * blockDim.z + threadIdx.z + offZ;

//    x,y,z;
    short dx = targetX - locX;
    short dy = targetY - locY;
    short dz = targetZ - locZ;
    short maxVal = max(abs(dx), max(abs(dy), abs(dz)));


    float stepX = dx / (float) maxVal;
    float stepY = dy / (float) maxVal;
    float stepZ = dz / (float) maxVal;

    float currX = locX;
    float currY = locY;
    float currZ = locZ;

    if (locX < offX || locY < offY || locZ < offZ || locX >= resLenx + offX || locY >= resLeny + offY || locZ >= resLenz + offZ) {
        return;
    }
    if (dx * dx + dy * dy + dz * dz > rad * rad) {
        return;
    }

    for (short i = 0; i <= maxVal; i++) {
        currX += stepX;
        currY += stepY;
        currZ += stepZ;

        short x = (short) currX;
        short y = (short) currY;
        short z = (short) currZ;

        if (x < 0 || y < 0 || z < 0 || x >= lenX || y >= lenY || z >= lenZ) {
            return;
        }

        int idx = z * lenX * lenY + y * lenX + x;
        if (req[idx]) {
            return;
        }
    }
    int val = atomicAdd(&listIdx, 1);
    arr[val] = {
            locX,locY,locZ
    };
}

Coordinate* gpu_coord;
void setupCudaMemory() {
    gpuErrchk( cudaMalloc((void**) &gpu_coord, sizeof(Coordinate) * GPU_RETURN_SIZE) );
}

int callShadowCast(bool *req, int lenX, int lenY, int lenZ,
                    int fromX, int fromY, int fromZ, int toX, int toY, int toZ,
                    short targetX, short targetY, short targetZ, int rad, Coordinate* arr) {

    int count = 0;
    cudaMemcpyToSymbol(listIdx, &count, sizeof(int), 0, cudaMemcpyHostToDevice);


    dim3 blockDim(10,10,10);
    dim3 gridDim(ceil((toX - fromX) / 10.0),ceil((toY - fromY) / 10.0),ceil((toZ - fromZ) / 10.0));
    shadowcast<<<gridDim, blockDim>>>(req, lenX, lenY, lenZ, fromX, fromY, fromZ,
                                      (toX - fromX), (toY - fromY), (toZ - fromZ),
                                      targetX, targetY, targetZ, rad, gpu_coord);

//    gpuErrchk( cudaPeekAtLastError() );
//    gpuErrchk( cudaDeviceSynchronize() );

    gpuErrchk(cudaMemcpyFromSymbol(&count, listIdx, sizeof(int), 0, cudaMemcpyDeviceToHost));

    gpuErrchk( cudaMemcpy(arr, gpu_coord, sizeof(Coordinate ) * count, cudaMemcpyDeviceToHost) );

    return count;
}