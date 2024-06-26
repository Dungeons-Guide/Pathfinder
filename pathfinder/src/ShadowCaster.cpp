//
// Created by syeyoung on 6/25/24.
//

#include "ShadowCaster.h"
#include <cmath>
#include <iostream>
#include<functional>
#include "raycast.h"

using namespace std;

#ifdef PATHFINDER_USE_CUDA
#include <cuda_runtime.h>


#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
    if (code != cudaSuccess)
    {
        fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}

#endif

ShadowCaster::ShadowCaster(PathfindRequest &request): request(request) {
#ifdef PATHFINDER_USE_CUDA
    xLen = request.blockWorld.xLen;
    yLen = request.blockWorld.yLen;
    zLen = request.blockWorld.zLen;
    shadowCasts = 0;

    this->blockMap = static_cast<bool*>(malloc(
            sizeof(bool) * xLen * yLen * zLen));
    gpuErrchk( cudaMalloc((void**) &(this->cudaBlockMap),
                          sizeof(bool) * xLen * yLen * zLen));

    for (int z = 0; z < zLen; z++) {
        for (int y = 0; y < yLen; y++) {
            for (int x = 0; x < xLen; x++) {
                this->blockMap[z * xLen * yLen + y * xLen + x] = request.blockWorld.getBlock(x,y,z).id != 0;
            }
        }
    }

    gpuErrchk( cudaMemcpy(this->cudaBlockMap, this->blockMap, sizeof(bool) *xLen * yLen * zLen, cudaMemcpyHostToDevice) );

    setupCudaMemory();


    int state = 0;
    this->minY = -1;
    this-> maxY = -1;
    for (int i = 0; i < 256; i++) {
        Block b = request.blockWorld.getBlock(8, i, 8);
        if (b.id != 0 && state == 0) {
            state = 1;
            this->minY = i ;
        } else if (b.id == 0 && state == 1) {
            state = 2;
            this->maxY = i;
        } else if (b.id != 0 && state == 2) {
            state = 1;
        }
    }
    if (minY == maxY) {
        state = 0;
        for (int i = 0; i < 256; i++) {
            Block b = request.blockWorld.getBlock(request.blockWorld.xLen - 8,i,request.blockWorld.zLen - 8);
            if (b.id != 0 && state == 0){
                state = 1;
                this->minY = i;
            } else if (b.id == 0 && state == 1) {
                state = 2;
                this->maxY = i;
            } else if (b.id != 0 && state == 2) {
                state = 1;
            }
        }
    }

    setupPotentialShadowcasts(this->cudaBlockMap, xLen, yLen, zLen, minY, maxY);

#endif
}
int TRANSFORM_MATRICES[24][9] = {
        // rotated vers
        {1,0,0, 0,1,0,0,0,1},
        {0,1,0, 0,0,1,1,0,0},
        {0,0,1, 1,0,0,0,1,0}, // you might ask, "but shouldn't there be 6 rotated versions?" well xyz and yxz is same thing, because prism is symmetric. So divide by two.

        // flipping!
        {-1,0,0, 0,1,0,0,0,1},
        {0,-1,0, 0,0,1,1,0,0},
        {0,0,-1, 1,0,0,0,1,0},

        {1,0,0, 0,-1,0,0,0,1},
        {0,1,0, 0,0,-1,1,0,0},
        {0,0,1, -1,0,0,0,1,0},

        {-1,0,0, 0,-1,0,0,0,1},
        {0,-1,0, 0,0,-1,1,0,0},
        {0,0,-1, -1,0,0,0,1,0},

        {1,0,0, 0,1,0,0,0,-1},
        {0,1,0, 0,0,1,-1,0,0},
        {0,0,1, 1,0,0,0,-1,0},

        {-1,0,0, 0,1,0,0,0,-1},
        {0,-1,0, 0,0,1,-1,0,0},
        {0,0,-1, 1,0,0,0,-1,0},

        {1,0,0, 0,-1,0,0,0,-1},
        {0,1,0, 0,0,-1,-1,0,0},
        {0,0,1, -1,0,0,0,-1,0},
        {-1,0,0, 0,-1,0,0,0,-1},
        {0,-1,0, 0,0,-1,-1,0,0},
        {0,0,-1, -1,0,0,0,-1,0},

};

void ShadowCaster::ShadowCast(int centerX, int centerY, int centerZ, int startZ, float startSlopeX,
                            float endSlopeX, float startSlopeY, float endSlopeY, int  radius,
                            float xOffset, float yOffset, float zOffset, int trMatrix11,
                            int  trMatrix21, int  trMatrix31, int  trMatrix12, int  trMatrix22,
                            int  trMatrix32, int  trMatrix13, int  trMatrix23, int  trMatrix33,
                              const std::function <void (int, int, int)>& f) {
    if (startZ > radius) return;
    shadowCasts++;
    // boom. radius is manhatten radius. lol.
    float realZ = startZ - zOffset;

    int startY =  max(0, (int) floor(startSlopeY * (realZ - 0.5) - 0.5 + yOffset)) - 1;
    int endY = (int) ceil(endSlopeY * (realZ + 0.5) - 0.5 + yOffset) + 1;
    int startX = max(0, (int) floor(startSlopeX * (realZ - 0.5) - 0.5 + xOffset)) - 1;
    int endX = (int) ceil(endSlopeX * (realZ + 0.5) - 0.5 + xOffset) + 1;
    int yLen = endY - startY + 1;
    int xLen = endX - startX + 1;
    bool blockMap[yLen][xLen];
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int trX = centerX + x * trMatrix11 + y * trMatrix21 + startZ * trMatrix31;
            int trY = centerY + x * trMatrix12 + y * trMatrix22 + startZ * trMatrix32;
            int trZ = centerZ + x * trMatrix13 + y * trMatrix23 + startZ * trMatrix33;

            bool localBlocked = request.blockWorld.getBlock(trX, trY, trZ).id != 0 && startZ != 0;
            if (endSlopeX == 1 && x +1 == endX && blockMap[y - startY][x-1 - startX]) localBlocked = true;
            if (endSlopeY == 1 && y +1 == endY && blockMap[y-1 - startY][x - startX]) localBlocked = true;

            blockMap[y - startY][x - startX] = localBlocked;

        }
    }
    for (int y = startY * 2 + 1; y < endY * 2; y ++) {
        float currentSlopeY = ((y-yOffset) / 2.0f) / (realZ-0.5f);
        float currentSlopeYP = ((y-yOffset) / 2.0f) / (realZ);
        for (int x = startX * 2 + 1; x < endX * 2; x++) {
            float currentSlopeX = ((x-xOffset) / 2.0f) / (realZ-0.5f) ;
            float currentSlopeXP = ((x-xOffset) / 2.0f) / (realZ);

            bool localBlocked = blockMap[y/2 - startY][x/2 - startX];
            if (x%2 != 0) localBlocked &= blockMap[y/2 - startY][x/2 - startX+1];
            if (y%2 != 0) localBlocked &= blockMap[y/2 - startY +1][x/2 - startX];
            if (x%2 != 0 && y%2 != 0) localBlocked &= blockMap[y/2 - startY + 1][x/2 - startX+1];
            if (localBlocked) continue;

            if (!(currentSlopeY < startSlopeY || currentSlopeY > endSlopeY || currentSlopeX < startSlopeX || currentSlopeX > endSlopeX)) [[likely]] {
                int trX = centerX * 2 + 1+ (x) * trMatrix11 + (y) * trMatrix21 + (startZ*2-1 ) * trMatrix31;
                int trY = centerY * 2 + 1+ (x) * trMatrix12 + (y) * trMatrix22 + (startZ*2 -1) * trMatrix32;
                int trZ = centerZ * 2 + 1+ (x) * trMatrix13 + (y) * trMatrix23 + (startZ*2-1) * trMatrix33;
                f(trX, trY, trZ);
            }
            if (!(currentSlopeYP < startSlopeY || currentSlopeYP > endSlopeY || currentSlopeXP < startSlopeX || currentSlopeXP > endSlopeX)) [[likely]]  {
                int trX = centerX * 2 + 1+ (x) * trMatrix11 + (y) * trMatrix21 + (startZ*2 ) * trMatrix31;
                int trY = centerY * 2 + 1+ (x) * trMatrix12 + (y) * trMatrix22 + (startZ*2 ) * trMatrix32;
                int trZ = centerZ * 2 + 1+ (x) * trMatrix13 + (y) * trMatrix23 + (startZ*2 ) * trMatrix33;
                f(trX, trY, trZ);
            }
        }
    }
    bool yEdges[yLen];
    bool xEdges[xLen];
    for (int y = 0; y < yLen;y ++) yEdges[y] = false;
    for (int x = 0; x < xLen; x++) xEdges[x] = false;

    for (int y = 0; y < yLen ; y++) {
        for (int x = 0; x < xLen; x++) {
            if (y < yLen -1 && blockMap[y][x] != blockMap[y+1][x])
                yEdges[y] = true;
            if (x < xLen - 1 && blockMap[y][x] != blockMap[y][x+1])
                xEdges[x] = true;
        }
    }
    yEdges[yLen - 1] = true;
    xEdges[xLen - 1] = true;


    int prevY = -1;
    float leeway = request.settings.etherwarpLeeway;
    for (int y = 0; y < yLen; y++) {
        if (!yEdges[y]) [[likely]] continue;
        int prevX = -1;
        for (int x = 0; x < xLen; x++) {
            if (!xEdges[x]) [[likely]] continue;
            if (!blockMap[y][x]) {
                bool yGood = prevY != -1 && !blockMap[prevY][x];
                bool xGood = prevX != -1 && !blockMap[y][prevX];
                bool diagonalGood = prevY != -1 && prevX != -1 && !blockMap[prevY][prevX];


                if (diagonalGood && yGood && xGood) {
                    float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = max(startSlopeX, (prevX  + startX - xOffset+ 0.5f - leeway) / (realZ + 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset  + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                                ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                           startSlopeYY ,endSlopeYY , radius,
                                           xOffset, yOffset, zOffset, trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, f);
                    }
                } else if ((xGood && !yGood) || (yGood && !xGood)) {
                    float startSlopeYY = yGood ? max(startSlopeY, (prevY + startY - yOffset +0.5f - leeway) / (realZ + 0.5f)) : max(startSlopeY, (prevY + startY - yOffset +0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = xGood ? max(startSlopeX, (prevX  + startX - xOffset+0.5f - leeway) / (realZ + 0.5f)) : max(startSlopeX, (prevX  + startX - xOffset+ 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset  + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                                ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                           startSlopeYY ,endSlopeYY , radius,
                                           xOffset, yOffset, zOffset, trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, f);
                    }
                } else if (!diagonalGood && yGood && xGood) {
                    {
                        float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset +0.5f + leeway) / (realZ - 0.5f));
                        float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                        float startSlopeXX = max(startSlopeX, (prevX + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                            [[likely]]
                                    ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                               startSlopeYY, endSlopeYY, radius,
                                               xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, f);
                        }
                    }
                    {
                        float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset +0.5f - leeway) / (realZ + 0.5f));
                        float endSlopeYY = min(endSlopeY, (prevY + startY - yOffset + 0.5f + leeway) / (realZ - 0.5f));
                        float startSlopeXX  =  max(startSlopeX, (prevX  + startX - xOffset+0.5f + leeway) / (realZ - 0.5f));
                        float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                            [[likely]]
                                    ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                               startSlopeYY ,endSlopeYY , radius,
                                               xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, f);
                        }
                    }
                } else {
                    float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset + 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = max(startSlopeX, (prevX  + startX - xOffset+ 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                                ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                           startSlopeYY ,endSlopeYY , radius,
                                           xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, f);
                    }
                    // normal case
                }
            }
            prevX = x;
        }
        prevY = y;
    }
}

Coordinate coordinates[GPU_RETURN_SIZE];
void ShadowCaster::RealShadowCast(const std::function <void (int, int, int)>& f, Coordinate start, int radius) {
#ifdef PATHFINDER_USE_CUDA
    shadowCasts++;

    int len = radius + radius + 6;
    int fromX = max(0, start.x - len/2);
    int fromY = max(minY, start.y - len/2);
    int fromZ = max(0, start.z - len/2);

    int toX = min(xLen, start.x + len/2);
    int toY = min(maxY, start.y + len/2);
    int toZ = min(zLen, start.z + len/2);

    int cnt = callShadowCast(this->cudaBlockMap, xLen, yLen, zLen,
                   fromX, fromY, fromZ,
                   toX, toY, toZ,
                   start.x, start.y, start.z,  request.settings.etherwarpOffset, radius + 3, coordinates);

    for (int i = 0; i < cnt; i++) {
        Coordinate coordinate = coordinates[i];
        f(coordinate.x, coordinate.y, coordinate.z);
    }
//    std::cout << cnt << "umm" << std::endl;

#else
    float offset = request.settings.etherwarpOffset;
    for (auto & i : TRANSFORM_MATRICES) {
        if (request.blockWorld.getBlock(start.x + 1 * i[0], start.y + 1 * i[3], start.z + 1 * i[6]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       offset, 0, 0,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], f
            );
        if (request.blockWorld.getBlock(start.x + 1 * i[1], start.y + 1 * i[4], start.z + 1 * i[7]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       0, offset, 0,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], f
            );
        if (request.blockWorld.getBlock(start.x + 1 * i[2], start.y + 1 * i[5], start.z + 1 * i[8]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       0, 0, offset,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], f
            );
    }
#endif
}