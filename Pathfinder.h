//
// Created by syeyoung on 2024/02/26.
//

#ifndef PATHFINDER2_PATHFINDER_H
#define PATHFINDER2_PATHFINDER_H

#include "PathfindRequest.h"

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

    void ShadowCast(uint32_t centerX, uint32_t centerY, uint32_t centerZ, uint32_t startZ, double startSlopeX, double endSlopeX, double startSlopeY, double endSlopeY, uint32_t radius,
                    uint8_t trMatrix11, uint8_t trMatrix21, uint8_t trMatrix31,
                    uint8_t trMatrix12, uint8_t trMatrix22, uint8_t trMatrix32,
                    uint8_t trMatrix13, uint8_t trMatrix23, uint8_t trMatrix33, std::vector<Coordinate> &result);
    std::vector<Coordinate> RealShadowCast(Coordinate start, uint32_t radius);
public:
    PathfindRequest& request;
    std::vector<std::vector<std::vector<PathfindNode> > > nodes;
    int minY;
    int maxY;

    void Populate();
    PathfindNode& GetNode(uint32_t x, uint32_t  y, uint32_t  z);
    PathfindNode& GetNode(const Coordinate& coord);

    Pathfinder(PathfindRequest& request);
};


#endif //PATHFINDER2_PATHFINDER_H
