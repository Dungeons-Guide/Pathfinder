//
// Created by syeyoung on 6/25/24.
//


#ifndef PATHFINDER_SHADOWCASTER_H
#define PATHFINDER_SHADOWCASTER_H

#include "PathfindRequest.h"
#include <functional>
class PathfindRequest;


class ShadowCaster {
private:
    void ShadowCast(int centerX, int centerY, int centerZ, int startZ, float startSlopeX, float endSlopeX, float startSlopeY,
                    float endSlopeY, int radius, float xOffset, float yOffset, float zOffset, int trMatrix11, int trMatrix21,
                    int trMatrix31, int trMatrix12, int trMatrix22, int trMatrix32, int trMatrix13, int trMatrix23,
                    int trMatrix33, std::vector<Coordinate> &result);
public:
    PathfindRequest& request;
    bool* blockMap;
    bool* cudaBlockMap;

    int shadowCasts;
    int xLen, yLen, zLen;

    void RealShadowCast(const std::function <void (int, int, int)>& f, Coordinate start, int radius);

    explicit ShadowCaster(PathfindRequest& request);
};

#endif //PATHFINDER_SHADOWCASTER_H
