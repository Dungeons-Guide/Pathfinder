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
    double gScore = std::numeric_limits<double>::max();
    uint8_t stonkLen = 0;
    ConnectionType type = ConnectionType_UNPOPULATED;
} ;

class Pathfinder {
private:

    void ShadowCast(int centerX, int centerY, int centerZ, int startZ, double startSlopeX,
                    double endSlopeX, double startSlopeY, double endSlopeY, uint32_t radius, int8_t trMatrix11,
                    int8_t  trMatrix21, int8_t  trMatrix31, int8_t  trMatrix12, int8_t  trMatrix22,
                    int8_t  trMatrix32, int8_t  trMatrix13, int8_t  trMatrix23, int8_t  trMatrix33,
                    std::vector<Coordinate> &result);
    void RealShadowCast(std::vector<Coordinate>&, Coordinate start, int radius);
public:
    PathfindRequest& request;
    std::vector<std::vector<std::vector<PathfindNode> > > nodes;
    std::vector<std::vector<std::vector<std::vector<Coordinate>>>> etherwarps;
    int minY;
    int maxY;

    void Populate();
    PathfindNode& GetNode(int x, int  y, int  z);
    PathfindNode& GetNode(const Coordinate& coord);

    Pathfinder(PathfindRequest& request);
};


#endif //PATHFINDER2_PATHFINDER_H
