//
// Created by syeyoung on 2024/02/26.
//

#ifndef PATHFINDER2_PATHFINDER_H
#define PATHFINDER2_PATHFINDER_H

#include "PathfindRequest.h"
#include <limits>

enum ConnectionType : uint8_t {
    ConnectionType_STONK_EXIT = 0,
    ConnectionType_STONK_WALK,
    ConnectionType_WALK,
    ConnectionType_ETHERWARP,
    ConnectionType_DIG_DOWN,
    ConnectionType_ECHEST,
    ConnectionType_DIG_UP,
    ConnectionType_TELEPORT_INTO,
    ConnectionType_SUPERBOOM,
    ConnectionType_TNTPEARL,
    ConnectionType_ENDERPEARL,
    ConnectionType_TARGET,
    ConnectionType_UNPOPULATED
} ;

struct PathfindNode {
    Coordinate parent = {0,0,0};
    float gScore = std::numeric_limits<float>::max();
    uint8_t stonkLen = 0;
    ConnectionType type = ConnectionType_UNPOPULATED;
} ;

class Pathfinder {
private:

    void ShadowCast(int centerX, int centerY, int centerZ, int startZ, float startSlopeX,
                    float endSlopeX, float startSlopeY, float endSlopeY, int radius,
                    float xOffset, float yOffset, float zOffset,
                    int trMatrix11, int  trMatrix21, int  trMatrix31, int  trMatrix12, int  trMatrix22,
                    int  trMatrix32, int  trMatrix13, int  trMatrix23, int  trMatrix33,
                    std::vector<Coordinate> &result);
public:
    PathfindRequest& request;
    std::vector<std::vector<std::vector<PathfindNode> > > nodes;
    std::vector<std::vector<std::vector<std::vector<Coordinate>>>> etherwarps;
    int minY;
    int maxY;
    int shadowcasts;

    void Populate();
    PathfindNode& GetNode(int x, int  y, int  z);
    PathfindNode& GetNode(const Coordinate& coord);

    void RealShadowCast(std::vector<Coordinate>&, Coordinate start, int radius);
    Pathfinder(PathfindRequest& request);
};


#endif //PATHFINDER2_PATHFINDER_H
