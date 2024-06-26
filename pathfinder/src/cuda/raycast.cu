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

__device__ bool ashadowcast(bool* req,
                          int lenX, int lenY, int lenZ,
                          float locX, float locY, float locZ,
                          float targetX, float  targetY, float  targetZ) {

    double dx = locX - targetX;
    double dy = locY - targetY;
    double dz = locZ - targetZ;
    short maxVal = max(abs(dx), max(abs(dy), abs(dz)));

    dx /= maxVal;
    dy /= maxVal;
    dz /= maxVal;

    for (short i = 0; i < maxVal; i++) {
        targetX += dx;
        targetY += dy;
        targetZ += dz;

        short x = (short) targetX;
        short y = (short) targetY;
        short z = (short) targetZ;

        int idx = z * lenX * lenY + y * lenX + x;
        if (req[idx]) {
            return false;
        }
    }
    return true;
}

__global__ void shadowcast(bool *req, int lenX, int lenY, int lenZ,
                           int offX, int offY, int offZ,
                           int resLenx, int resLeny, int resLenz,
                           short targetX, short targetY, short targetZ,
                           float offset, int rad,
                           Coordinate* arr) {
    short locX = blockIdx.x * blockDim.x + threadIdx.x + offX;
    short locY = blockIdx.y * blockDim.y + threadIdx.y + offY;
    short locZ = blockIdx.z * blockDim.z + threadIdx.z + offZ;

    short dx = targetX - locX;
    short dy = targetY - locY;
    short dz = targetZ - locZ;

    if (locX < offX || locY < offY || locZ < offZ || locX >= resLenx + offX || locY >= resLeny + offY || locZ >= resLenz + offZ) {
        return;
    }
    if (dx * dx + dy * dy + dz * dz > rad * rad) {
        return;
    }


    for (int z = 0; z < 2; z++) {
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                int idx = (locZ-z) * lenX * lenY + (locY-2-y) * lenX + locX-x;
                if (idx < 0) continue;
                if (req[idx]) {
                    goto label;
                }
            }
        }
    }
    return;
    label:

    for (int x = 0; x < 1; x++) {
        for (int z = 0; z < 1; z++) {
            for (int y = 0; y < 2; y++) {
                bool flag = false;
                int xTarget;
                if (dx < 0) {
                    xTarget = targetX + 0.5 + offset;
                } else {
                    xTarget = targetX + 0.5 - offset;
                }
                flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY+ y / 2.0, locZ + z/2.0, xTarget, targetY + 0.5,
                                    targetZ + 0.5);

                if (dy < 0) {
                    xTarget = targetY + 0.5 + offset;
                } else {
                    xTarget = targetY + 0.5 - offset;
                }
                flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY + y / 2.0, locZ + z/2.0, targetX + 0.5, xTarget,
                                    targetZ + 0.5);
                
                if (dz < 0) {
                    xTarget = targetZ + 0.5 + offset;
                } else {
                    xTarget = targetZ + 0.5 - offset;
                }
                flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY + y / 2.0, locZ+ z/2.0, targetX + 0.5, targetY + 0.5,
                                   xTarget);

                if (flag) {
                    int val = atomicAdd(&listIdx, 1);
                    arr[val] = {
                            locX * 2 + x, locY * 2 + y, locZ * 2 + z
                    };
                }
            }

        }
    }

}

Coordinate* gpu_coord;
void setupCudaMemory() {
    gpuErrchk( cudaMalloc((void**) &gpu_coord, sizeof(Coordinate) * GPU_RETURN_SIZE) );
}

int callShadowCast(bool *req, int lenX, int lenY, int lenZ,
                    int fromX, int fromY, int fromZ, int toX, int toY, int toZ,
                   short targetX, short targetY, short targetZ, float offset, int rad, Coordinate* arr) {

    int count = 0;
    cudaMemcpyToSymbol(listIdx, &count, sizeof(int), 0, cudaMemcpyHostToDevice);


    dim3 blockDim(32,1,32);
        dim3 gridDim(ceil((toX - fromX) / 32.0),ceil((toY - fromY) / 1.0),ceil((toZ - fromZ) / 32.0));
    shadowcast<<<gridDim, blockDim>>>(req, lenX, lenY, lenZ, fromX, fromY, fromZ,
                                      (toX - fromX), (toY - fromY), (toZ - fromZ),
                                      targetX, targetY, targetZ, offset, rad, gpu_coord);

    gpuErrchk( cudaPeekAtLastError() );
    gpuErrchk( cudaDeviceSynchronize() );

    gpuErrchk(cudaMemcpyFromSymbol(&count, listIdx, sizeof(int), 0, cudaMemcpyDeviceToHost));

    std::cout << count << std::endl;
    gpuErrchk( cudaMemcpy(arr, gpu_coord, sizeof(Coordinate ) * count, cudaMemcpyDeviceToHost) );

    return count;
}