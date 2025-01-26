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

// Fix pixel perfect etherwarps and it would be fine.
__device__ bool ashadowcast(bool* req,
                          int lenX, int lenY, int lenZ,
                          float locX, float locY, float locZ,
                          float targetX, float  targetY, float  targetZ) {
    float dx = locX - targetX;
    float dy = locY - targetY;
    float dz = locZ - targetZ;
    short maxVal = max(abs(dx), max(abs(dy), abs(dz))) * 9;

    dx /= maxVal;
    dy /= maxVal;
    dz /= maxVal;

    for (short i = 0; i < maxVal-2; i++) {
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
                           int toX, int toY, int toZ,
                           short targetX, short targetY, short targetZ,
                           float offset, int rad,
                           Coordinate* arr,
                           Coordinate* potentialShadowcasts, int cnt) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx > cnt) return;

    Coordinate location = potentialShadowcasts[idx];
    short locX = location.x;
    short locY = location.y;
    short locZ = location.z;

    short dx = targetX - locX;
    short dy = targetY - locY;
    short dz = targetZ - locZ;

    if (locX < offX || locY < offY || locZ < offZ || locX >= toX + offX || locY >= toY + offY || locZ >= toZ + offZ) {
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

    for (int x = 0; x < 2; x++) {
        for (int z = 0; z < 2; z++) {
            for (int y = 0; y < 2; y++) {
                bool flag = false;
                int xTarget;
//                long begin = clock64();
                if (dx < 0) {
                    xTarget = targetX + 0.499999 + offset;
                } else {
                    xTarget = targetX + 0.500001 - offset;
                }
                flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY+ y / 2.0, locZ + z/2.0,
                                    xTarget, targetY + 0.499798, targetZ + 0.499889);
//                long end = clock64();
//                ahhh[atomicAdd(&listIdx2, 1)] = end-begin;

//                begin = clock64();
//                if (!flag) {
                    if (dy < 0) {
                        xTarget = targetY + 0.499798 + offset;
                    } else {
                        xTarget = targetY + 0.500202 - offset;
                    }
                    flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY + y / 2.0, locZ + z/2.0,
                                        targetX + 0.499999, xTarget, targetZ + 0.499889);
//                }
//                end = clock64();
//                ahhh[atomicAdd(&listIdx2, 1)] = end-begin;

//                begin = clock64();
//                if (!flag) {
                    if (dz < 0) {
                        xTarget = targetZ + 0.499889 + offset;
                    } else {
                        xTarget = targetZ + 0.500114 - offset;
                    }
                    flag |= ashadowcast(req, lenX, lenY, lenZ, locX + x/2.0, locY + y / 2.0, locZ+ z/2.0,
                                        targetX + 0.499999, targetY + 0.499798, xTarget);
//                }
//                end = clock64();
//                ahhh[atomicAdd(&listIdx2, 1)] = end-begin;


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

__global__ void findPotentialShadowcastDestinations(bool *req, int lenX, int lenY, int lenZ,
                                                    int fromY, int toY,
                                                    Coordinate* arr) {
    short locX = blockIdx.x * blockDim.x + threadIdx.x ;
    short locY = blockIdx.y * blockDim.y + threadIdx.y + fromY;
    short locZ = blockIdx.z * blockDim.z + threadIdx.z ;


    if (locX < 0 || locY < 0 || locZ < 0 || locX >= lenX || locY >= toY || locZ >= lenZ) {
        return;
    }

    for (int z = 0; z < 2; z++) {
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                int idx = (locZ-z) * lenX * lenY + (locY-2-y) * lenX + locX-x;
                if (locZ - z < 0) continue;
                if (locY - y  -2 <0) continue;
                if (locX - x < 0) continue;

                if (req[idx]) {
                    goto label;
                }
            }
        }
    }
    return;
    label:

    for (int z = 0; z < 2; z++) {
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                int idx = (locZ-z) * lenX * lenY + (locY-y) * lenX + locX-x;
                if (locZ - z < 0) continue;
                if (locY - y < 0) continue;
                if (locX - x < 0) continue;

                if (!req[idx]) {
                    goto label2;
                }
            }
        }
    }
    return;
    label2:

    int val = atomicAdd(&listIdx, 1);
    arr[val] = {
            locX, locY, locZ
    };
}

Coordinate* gpu_coord;
Coordinate* potentialShadowcasts;
int potentialShadowcastCount;
void setupCudaMemory() {
    gpuErrchk( cudaMalloc((void**) &gpu_coord, sizeof(Coordinate) * GPU_RETURN_SIZE) );
    gpuErrchk( cudaMalloc((void**) &potentialShadowcasts, sizeof(Coordinate) * 4000000) );
}

int setupPotentialShadowcasts(bool *req, int lenX, int lenY, int lenZ, int fromY, int toY) {
    int count = 0;
    cudaMemcpyToSymbol(listIdx, &count, sizeof(int), 0, cudaMemcpyHostToDevice);
    std::cout << lenX << "/"<<lenY << "/"<<lenZ<<"/"<<fromY<<"/"<<toY<<std::endl;

    dim3 blockDim(16,4,16);
    dim3 gridDim(ceil(lenX/16.0), ceil((toY-fromY)/4.0), ceil(lenZ/16.0));
    findPotentialShadowcastDestinations<<<gridDim, blockDim>>>(req, lenX, lenY, lenZ, fromY, toY,  potentialShadowcasts);

    gpuErrchk( cudaPeekAtLastError() );
    gpuErrchk( cudaDeviceSynchronize() );

    gpuErrchk(cudaMemcpyFromSymbol(&count, listIdx, sizeof(int), 0, cudaMemcpyDeviceToHost));

    std::cout << count << std::endl;
    potentialShadowcastCount = count;

    return count;
}



int callShadowCast(bool *req, int lenX, int lenY, int lenZ,
                    int fromX, int fromY, int fromZ, int toX, int toY, int toZ,
                   short targetX, short targetY, short targetZ, float offset, int rad, Coordinate* arr) {

    int count = 0;
    cudaMemcpyToSymbol(listIdx, &count, sizeof(int), 0, cudaMemcpyHostToDevice);;


    shadowcast<<<ceil(potentialShadowcastCount / 1024.0), 1024>>>
    (req, lenX, lenY, lenZ, fromX, fromY, fromZ, toX, toY, toZ,
                                      targetX, targetY, targetZ, offset, rad, gpu_coord, potentialShadowcasts,potentialShadowcastCount);

    gpuErrchk( cudaPeekAtLastError() );
    gpuErrchk( cudaDeviceSynchronize() );

    gpuErrchk(cudaMemcpyFromSymbol(&count, listIdx, sizeof(int), 0, cudaMemcpyDeviceToHost));

    gpuErrchk( cudaMemcpy(arr, gpu_coord, sizeof(Coordinate ) * count, cudaMemcpyDeviceToHost) );

    return count;
}