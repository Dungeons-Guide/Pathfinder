//
// Created by syeyoung on 6/25/24.
//

#ifndef PATHFINDER_RAYCAST_H
#define PATHFINDER_RAYCAST_H

#define GPU_RETURN_SIZE 1000000

#include "PathfindRequest.h"

int callShadowCast(bool *req, int lenX, int lenY, int lenZ,
                    int fromX, int fromY, int fromZ, int toX, int toY, int toZ,
                    short targetX, short  targetY, short  targetZ, float offset, int rad, Coordinate* coordinates);

void setupCudaMemory();

int setupPotentialShadowcasts(bool *req, int lenX, int lenY, int lenZ, int fromY, int toY);

#endif //PATHFINDER_RAYCAST_H
